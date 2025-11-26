// First example: Basic Monte Carlo pricing of European and Asian options

#include "mc_engine/core/types.hpp"
#include "mc_engine/core/random.hpp"
#include "mc_engine/models/gbm.hpp"
#include "mc_engine/payoffs/european.hpp"
#include "mc_engine/payoffs/asian.hpp"
#include "mc_engine/pricing/monte_carlo.hpp"
#include <iostream>
#include <iomanip>

using namespace mc_engine;

void print_result(const std::string& name, const PricingResult& result) {
    std::cout << "\n" << name << " Results:\n";
    std::cout << std::string(50, '=') << "\n";
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Price:          " << result.price << "\n";
    std::cout << "Std Error:      " << result.std_error << "\n";
    std::cout << "95% CI:         [" << result.confidence_interval_lower()<< ", " << result.confidence_interval_upper() << "]\n";
    std::cout << "Variance:       " << result.variance << "\n";
    std::cout << "Num Paths:      " << result.num_paths << "\n";
    std::cout << "Compute Time:   " << result.compute_time << " seconds\n";
}

int main() {
    std::cout << "Monte Carlo Option Pricing - Basic Example\n";
    std::cout << std::string(50, '=') << "\n\n";
    
    // Market parameters
    Real S0 = 100.0;
    Real K = 100.0;
    Real r = 0.05;
    Real sigma = 0.2;
    Real T = 1.0;
    
    MarketParams params(S0, K, r, sigma, T);
    
    std::cout << "Market Parameters:\n";
    std::cout << "  S0 = " << S0 << "\n";
    std::cout << "  K = " << K << "\n";
    std::cout << "  r = " << r << "\n";
    std::cout << "  σ = " << sigma << "\n";
    std::cout << "  T = " << T << "\n\n";
    
    // Simulation configuration
    Size num_paths = 100000;
    Size num_steps = 252;
    
    SimulationConfig config(num_paths, num_steps, 42);
    
    std::cout << "Simulation Configuration:\n";
    std::cout << "  Paths: " << num_paths << "\n";
    std::cout << "  Steps: " << num_steps << "\n";
    std::cout << "  dt = " << config.dt(T) << "\n\n";
    
    // European Call Option
    std::cout << std::string(50, '=') << "\n";
    std::cout << "EUROPEAN CALL OPTION\n";
    std::cout << std::string(50, '=') << "\n";
    
    EuropeanPayoff european_call(K, OptionType::Call);
    MonteCarloEngine mc_european(params, config, european_call);
    
    auto euro_result = mc_european.price();
    print_result("European Call (Standard MC)", euro_result);
    
    auto euro_anti_result = mc_european.price_antithetic();
    print_result("European Call (Antithetic Variates)", euro_anti_result);
    
    Real variance_reduction = euro_result.variance / euro_anti_result.variance;
    std::cout << "\nVariance Reduction Factor: " << variance_reduction << "x\n";
    
    // Asian Call Option
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << "ASIAN ARITHMETIC AVERAGE CALL OPTION\n";
    std::cout << std::string(50, '=') << "\n";
    
    AsianArithmeticPayoff asian_call(K, OptionType::Call, true);
    MonteCarloEngine mc_asian(params, config, asian_call);
    
    auto asian_result = mc_asian.price();
    print_result("Asian Arithmetic Call (Standard MC)", asian_result);
    
    auto asian_anti_result = mc_asian.price_antithetic();
    print_result("Asian Arithmetic Call (Antithetic Variates)", asian_anti_result);
    
    Real asian_variance_reduction = asian_result.variance / asian_anti_result.variance;
    std::cout << "\nVariance Reduction Factor: " << asian_variance_reduction << "x\n";
    
    // Comparison
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << "COMPARISON\n";
    std::cout << std::string(50, '=') << "\n";
    std::cout << "  European Call Price: " << euro_result.price << "\n";
    std::cout << "  Asian Call Price:    " << asian_result.price << "\n";
    std::cout << "  Difference:          " << euro_result.price - asian_result.price << "\n";
    std::cout << "  Asian/European:      " << asian_result.price / euro_result.price << "\n";
    
    std::cout << "\nKey Observations:\n";
    std::cout << "  • Asian options are cheaper due to averaging effect\n";
    std::cout << "  • Antithetic variates reduce variance effectively\n";
    std::cout << "  • Asian options benefit more from variance reduction\n";
    
    return 0;
}