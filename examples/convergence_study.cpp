// Study convergence behavior: price vs N, price vs dt, variance reduction

#include "mc_engine/core/types.hpp"
#include "mc_engine/models/gbm.hpp"
#include "mc_engine/models/black_scholes.hpp"
#include "mc_engine/payoffs/european.hpp"
#include "mc_engine/payoffs/asian.hpp"
#include "mc_engine/pricing/monte_carlo.hpp"
#include "mc_engine/core/statistics.hpp"
#include <iostream>
#include <iomanip>
#include <vector>
#include <fstream>

using namespace mc_engine;

void convergence_vs_paths(const std::string& filename) {

    std::cout << "\nConvergence Study: Price vs Number of Paths\n";
    
    MarketParams params(100.0, 100.0, 0.05, 0.2, 1.0);
    Real bs_price = BlackScholes::european_price(params, OptionType::Call);
    
    std::cout << "True price (Black-Scholes): " << bs_price << "\n\n";
    
    EuropeanPayoff call(100.0, OptionType::Call);
    
    std::vector<Size> path_counts = {
        1000, 2000, 5000, 10000, 20000, 50000, 
        100000, 200000, 500000, 1000000
    };
    
    std::ofstream file(filename);
    file << "num_paths,mc_price,std_error,rel_error,ci_lower,ci_upper,time\n";
    
    std::cout << std::setw(12) << "Paths"
              << std::setw(12) << "MC Price"
              << std::setw(12) << "Std Error"
              << std::setw(12) << "Rel Error"
              << std::setw(12) << "Time (s)\n";
    std::cout << std::string(60, '-') << "\n";
    
    for (Size n : path_counts) {
        SimulationConfig config(n, 100, 42);
        MonteCarloEngine mc(params, config, call);
        
        auto result = mc.price();
        Real rel_error = std::abs(result.price - bs_price) / bs_price * 100.0;
        
        std::cout << std::fixed << std::setprecision(6)
                  << std::setw(12) << n
                  << std::setw(12) << result.price
                  << std::setw(12) << result.std_error
                  << std::setw(12) << rel_error << "%"
                  << std::setw(12) << result.compute_time << "\n";
        
        file << n << "," << result.price << "," << result.std_error << ","
             << rel_error << "," << result.confidence_interval_lower() << ","
             << result.confidence_interval_upper() << "," << result.compute_time << "\n";
    }
    
    file.close();
    std::cout << "\nResults saved to: " << filename << "\n";
}

void convergence_vs_timesteps(const std::string& filename) {

    std::cout << "\nConvergence Study: Price vs Number of Time Steps\n";
    
    MarketParams params(100.0, 100.0, 0.05, 0.2, 1.0);
    AsianArithmeticPayoff asian_call(100.0, OptionType::Call);
    
    std::vector<Size> step_counts = {10, 20, 50, 100, 200, 252, 500, 1000};
    Size num_paths = 100000;
    
    std::ofstream file(filename);
    file << "num_steps,dt,price,std_error,time\n";
    
    std::cout << std::setw(12) << "Steps"
              << std::setw(12) << "dt"
              << std::setw(12) << "Price"
              << std::setw(12) << "Std Error"
              << std::setw(12) << "Time (s)\n";
    std::cout << std::string(60, '-') << "\n";
    
    for (Size steps : step_counts) {
        SimulationConfig config(num_paths, steps, 42);
        MonteCarloEngine mc(params, config, asian_call);
        
        auto result = mc.price();
        Real dt = config.dt(params.maturity);
        
        std::cout << std::fixed << std::setprecision(6)
                  << std::setw(12) << steps
                  << std::setw(12) << dt
                  << std::setw(12) << result.price
                  << std::setw(12) << result.std_error
                  << std::setw(12) << result.compute_time << "\n";
        
        file << steps << "," << dt << "," << result.price << ","
             << result.std_error << "," << result.compute_time << "\n";
    }
    
    file.close();
    std::cout << "\nResults saved to: " << filename << "\n";
}

void variance_reduction_comparison(const std::string& filename) {
    std::cout << "\n==========================================================\n";
    std::cout << "Variance Reduction: Standard vs Antithetic\n";
    
    MarketParams params(100.0, 100.0, 0.05, 0.2, 1.0);
    AsianArithmeticPayoff asian_call(100.0, OptionType::Call);
    
    std::vector<Size> path_counts = {10000, 20000, 50000, 100000, 200000};
    
    std::ofstream file(filename);
    file << "num_paths,standard_var,anti_var,vrf,standard_time,anti_time,efficiency\n";
    
    std::cout << std::setw(12) << "Paths"
              << std::setw(15) << "Std Var"
              << std::setw(15) << "Anti Var"
              << std::setw(12) << "VRF"
              << std::setw(15) << "Efficiency\n";
    std::cout << std::string(70, '-') << "\n";
    
    for (Size n : path_counts) {
        SimulationConfig config(n, 252, 42);
        MonteCarloEngine mc(params, config, asian_call);
        
        auto std_result = mc.price();
        auto anti_result = mc.price_antithetic();
        
        Real vrf = std_result.variance / anti_result.variance;
        Real efficiency = Statistics::efficiency_gain(
            std_result.variance, std_result.compute_time,
            anti_result.variance, anti_result.compute_time
        );
        
        std::cout << std::fixed << std::setprecision(6)
                  << std::setw(12) << n
                  << std::setw(15) << std_result.variance
                  << std::setw(15) << anti_result.variance
                  << std::setw(12) << vrf
                  << std::setw(15) << efficiency << "\n";
        
        file << n << "," << std_result.variance << "," << anti_result.variance << ","
             << vrf << "," << std_result.compute_time << "," << anti_result.compute_time << ","
             << efficiency << "\n";
    }
    
    file.close();
    std::cout << "\nResults saved to: " << filename << "\n";
}

int main() {
    std::cout << "Phase 2: Convergence & Validation Studies\n";
    
    std::cout << "\nGenerating convergence data...\n";
    
    convergence_vs_paths("convergence_paths.csv");
    convergence_vs_timesteps("convergence_timesteps.csv");
    variance_reduction_comparison("variance_reduction.csv");
    
    std::cout << "All studies complete!\n";
    std::cout << "\nGenerated files:\n";
    std::cout << "  - convergence_paths.csv\n";
    std::cout << "  - convergence_timesteps.csv\n";
    std::cout << "  - variance_reduction.csv\n";
    std::cout << "\nUse plot_results.py to visualize the data.\n";
    
    return 0;
}