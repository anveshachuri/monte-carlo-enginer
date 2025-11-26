// include/mc_engine/performance/benchmark.hpp
// Comprehensive benchmarking suite

#ifndef MC_ENGINE_PERFORMANCE_BENCHMARK_HPP
#define MC_ENGINE_PERFORMANCE_BENCHMARK_HPP

#include "mc_engine/core/types.hpp"
#include "mc_engine/core/statistics.hpp"
#include "mc_engine/models/gbm.hpp"
#include "mc_engine/payoffs/asian.hpp"
#include "mc_engine/pricing/monte_carlo.hpp"
#include "mc_engine/performance/threading.hpp"
#include "mc_engine/performance/simd.hpp"
#include <chrono>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <thread>

namespace mc_engine {

enum class OptimizationLevel {
    Baseline,
    Threading,
    SIMD,
    Combined
};

struct BenchmarkResult {
    std::string name;
    OptimizationLevel level;
    Real price;
    Real std_error;
    Real compute_time;
    Size num_paths;
    Size num_threads;
    Real speedup;
    Real efficiency;
    Real paths_per_second;
    
    void print() const {
        std::cout << std::fixed << std::setprecision(6);
        std::cout << std::setw(20) << name
                  << std::setw(15) << price
                  << std::setw(15) << std_error
                  << std::setw(12) << compute_time
                  << std::setw(10) << speedup << "x"
                  << std::setw(12) << efficiency
                  << std::setw(15) << paths_per_second << "\n";
    }
};

class BenchmarkSuite {
private:
    MarketParams params_;
    std::vector<BenchmarkResult> results_;
    
public:
    BenchmarkSuite(const MarketParams& params) : params_(params) {}
    
    BenchmarkResult run_baseline(Size num_paths, Size num_steps) {
        SimulationConfig config(num_paths, num_steps, 42);
        AsianArithmeticPayoff asian_call(params_.strike, OptionType::Call);
        MonteCarloEngine mc(params_, config, asian_call);
        
        auto result = mc.price();
        
        BenchmarkResult bench;
        bench.name = "Baseline";
        bench.level = OptimizationLevel::Baseline;
        bench.price = result.price;
        bench.std_error = result.std_error;
        bench.compute_time = result.compute_time;
        bench.num_paths = num_paths;
        bench.num_threads = 1;
        bench.speedup = 1.0;
        bench.efficiency = 1.0;
        bench.paths_per_second = num_paths / result.compute_time;
        
        results_.push_back(bench);
        return bench;
    }
    
    BenchmarkResult run_threaded(Size num_paths, Size num_steps, 
                                 Size num_threads, Real baseline_time) {
        SimulationConfig config(num_paths, num_steps, 42);
        GBMPathGenerator path_gen(params_, config);
        AsianArithmeticPayoff asian_call(params_.strike, OptionType::Call);
        
        auto payoff_func = [&](const std::vector<Real>& path) {
            return asian_call(path);
        };
        
        Real discount = std::exp(-params_.rate * params_.maturity);
        auto result = parallel_monte_carlo(path_gen, payoff_func, 
                                          discount, num_threads);
        
        BenchmarkResult bench;
        bench.name = "Multi-threaded";
        bench.level = OptimizationLevel::Threading;
        bench.price = result.price;
        bench.std_error = result.std_error;
        bench.compute_time = result.compute_time;
        bench.num_paths = num_paths;
        bench.num_threads = num_threads;
        bench.speedup = baseline_time / result.compute_time;
        bench.efficiency = bench.speedup / num_threads;
        bench.paths_per_second = num_paths / result.compute_time;
        
        results_.push_back(bench);
        return bench;
    }
    
    BenchmarkResult run_simd(Size num_paths, Size num_steps, Real baseline_time) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        Size aligned_paths = (num_paths / SIMD_WIDTH) * SIMD_WIDTH;
        
        SimulationConfig config(aligned_paths, num_steps, 42);
        SIMDPathGenerator path_gen(params_, config);
        
        PathData paths(aligned_paths, num_steps);
        auto rng = make_rng(config.seed);
        
        path_gen.generate_paths_simd(paths, *rng);
        
        std::vector<Real> payoffs(aligned_paths);
        SIMDPayoffComputer::compute_asian_payoffs(paths, payoffs, params_.strike);
        
        auto stats = Statistics::compute_full(payoffs);
        
        Real discount = std::exp(-params_.rate * params_.maturity);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<Real> elapsed = end_time - start_time;
        
        BenchmarkResult bench;
        bench.name = "SIMD";
        bench.level = OptimizationLevel::SIMD;
        bench.price = discount * stats.mean;
        bench.std_error = discount * stats.std_error;
        bench.compute_time = elapsed.count();
        bench.num_paths = aligned_paths;
        bench.num_threads = 1;
        bench.speedup = baseline_time / elapsed.count();
        bench.efficiency = bench.speedup;
        bench.paths_per_second = aligned_paths / elapsed.count();
        
        results_.push_back(bench);
        return bench;
    }
    
    void run_scalability_study(Size num_paths, Size num_steps) {
        std::cout << "\n" << std::string(90, '=') << "\n";
        std::cout << "Scalability Study: Speedup vs Number of Threads\n";
        std::cout << std::string(90, '=') << "\n";
        
        auto baseline = run_baseline(num_paths, num_steps);
        
        std::cout << "\nBaseline: " << baseline.compute_time << " seconds\n\n";
        
        std::cout << std::setw(12) << "Threads"
                  << std::setw(15) << "Time (s)"
                  << std::setw(15) << "Speedup"
                  << std::setw(15) << "Efficiency"
                  << std::setw(20) << "Paths/sec\n";
        std::cout << std::string(77, '-') << "\n";
        
        Size max_threads = std::thread::hardware_concurrency();
        std::vector<Size> thread_counts = {1, 2, 4};
        if (max_threads >= 8) thread_counts.push_back(8);
        if (max_threads >= 16) thread_counts.push_back(16);
        
        for (Size nt : thread_counts) {
            if (nt > max_threads) break;
            
            auto result = run_threaded(num_paths, num_steps, nt, baseline.compute_time);
            
            std::cout << std::fixed << std::setprecision(4)
                      << std::setw(12) << nt
                      << std::setw(15) << result.compute_time
                      << std::setw(15) << result.speedup
                      << std::setw(15) << result.efficiency
                      << std::setw(20) << result.paths_per_second << "\n";
        }
    }
    
    void run_full_suite(Size num_paths, Size num_steps) {
        std::cout << "\n" << std::string(110, '=') << "\n";
        std::cout << "COMPREHENSIVE PERFORMANCE BENCHMARK\n";
        std::cout << std::string(110, '=') << "\n";
        
        std::cout << "\nConfiguration:\n";
        std::cout << "  Paths: " << num_paths << "\n";
        std::cout << "  Steps: " << num_steps << "\n";
        std::cout << "  Hardware threads: " << std::thread::hardware_concurrency() << "\n";
        std::cout << "  SIMD width: " << SIMD_WIDTH << " (AVX2: " 
                  << (HAS_AVX2 ? "enabled" : "disabled") << ")\n";
        
        std::cout << "\n" << std::setw(20) << "Method"
                  << std::setw(15) << "Price"
                  << std::setw(15) << "Std Error"
                  << std::setw(12) << "Time (s)"
                  << std::setw(10) << "Speedup"
                  << std::setw(12) << "Efficiency"
                  << std::setw(15) << "Paths/sec\n";
        std::cout << std::string(99, '-') << "\n";
        
        auto baseline = run_baseline(num_paths, num_steps);
        baseline.print();
        
        Size num_threads = std::min<Size>(std::thread::hardware_concurrency(), 8);
        auto threaded = run_threaded(num_paths, num_steps, num_threads, baseline.compute_time);
        threaded.print();
        
        if (HAS_AVX2) {
            auto simd = run_simd(num_paths, num_steps, baseline.compute_time);
            simd.print();
        }
        
        std::cout << std::string(99, '=') << "\n";
    }
    
    void export_results(const std::string& filename) const {
        std::ofstream file(filename);
        file << "method,price,std_error,time,speedup,efficiency,threads,paths_per_sec\n";
        
        for (const auto& result : results_) {
            file << result.name << ","
                 << result.price << ","
                 << result.std_error << ","
                 << result.compute_time << ","
                 << result.speedup << ","
                 << result.efficiency << ","
                 << result.num_threads << ","
                 << result.paths_per_second << "\n";
        }
        
        file.close();
    }
    
    const std::vector<BenchmarkResult>& get_results() const {
        return results_;
    }
};

} 
#endif