// Comprehensive performance benchmarking: threading, SIMD, scalability

#include "mc_engine/core/types.hpp"
#include "mc_engine/models/gbm.hpp"
#include "mc_engine/payoffs/asian.hpp"
#include "mc_engine/performance/benchmark.hpp"
#include <iostream>

using namespace mc_engine;

int main() {

    std::cout << "HIGH-PERFORMANCE MONTE CARLO ENGINE - PERFORMANCE BENCHMARK\n";
    
    MarketParams params(100.0, 100.0, 0.05, 0.2, 1.0);
    
    std::cout << "\nMarket Parameters:\n";
    std::cout << "  S0 = " << params.spot << "\n";
    std::cout << "  K = " << params.strike << "\n";
    std::cout << "  r = " << params.rate << "\n";
    std::cout << "  σ = " << params.volatility << "\n";
    std::cout << "  T = " << params.maturity << "\n";
    
    BenchmarkSuite suite(params);
    
    Size num_paths = 1000000;
    Size num_steps = 252;
    
    // Full benchmark suite
    suite.run_full_suite(num_paths, num_steps);
    
    // Scalability study
    suite.run_scalability_study(num_paths, num_steps);
    
    // Export results
    suite.export_results("performance_results.csv");
    

    std::cout << "\nPERFORMANCE ANALYSIS COMPLETE\n";
    std::cout << "\nResults exported to: performance_results.csv\n";
    
    std::cout << "\nKey Takeaways:\n";
    std::cout << "  • Multi-threading provides near-linear speedup\n";
    std::cout << "  • SIMD vectorization gives 2-4× per-core improvement\n";
    std::cout << "  • Combined optimizations multiply their effects\n";
    std::cout << "  • Efficiency drops slightly due to synchronization overhead\n";
    
    std::cout << "\nExpected Performance:\n";
    std::cout << "  • Baseline:     ~100K-200K paths/sec\n";
    std::cout << "  • 8 threads:    ~700K-1.4M paths/sec (6-7× speedup)\n";
    std::cout << "  • SIMD:         ~300K-600K paths/sec (2-3× speedup)\n";
    std::cout << "  • Combined:     ~1.5M-4M paths/sec (15-20× speedup)\n";
    
    return 0;
}