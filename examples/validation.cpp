// examples/validation.cpp
// Validate Monte Carlo against Black-Scholes analytical formulas

#include "mc_engine/core/types.hpp"
#include "mc_engine/models/gbm.hpp"
#include "mc_engine/models/black_scholes.hpp"
#include "mc_engine/payoffs/european.hpp"
#include "mc_engine/pricing/monte_carlo.hpp"
#include "mc_engine/core/statistics.hpp"
#include <iostream>
#include <iomanip>

using namespace mc_engine;

void print_comparison(const std::string& scenario, Real mc_price, Real mc_stderr, Real bs_price) {
    Real error = mc_price - bs_price;
    Real rel_error = std::abs(error) / bs_price * 100.0;
    Real z_score = error / mc_stderr;
    
    std::cout << "\n" << scenario << ":\n";
    std::cout << std::string(60, '-') << "\n";
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "  Monte Carlo:     " << mc_price << " ± " << mc_stderr << "\n";
    std::cout << "  Black-Scholes:   " << bs_price << "\n";
    std::cout << "  Error:           " << error << "\n";
    std::cout << "  Relative Error:  " << rel_error << "%\n";
    std::cout << "  Z-score:         " << z_score << " σ\n";
    
    if (std::abs(z_score) < 2.0) {
        std::cout << "  ✓ Within 95% confidence interval\n";
    } else if (std::abs(z_score) < 3.0) {
        std::cout << "  ⚠ Outside 95% CI but within 99.7%\n";
    } else {
        std::cout << "  ✗ Outside 99.7% confidence interval!\n";
    }
}

int main() {
    std::cout << "==========================================================\n";
    std::cout << "Monte Carlo vs Black-Scholes Validation\n";
    std::cout << "==========================================================\n";
    
    Size num_paths = 500000;
    Size num_steps = 100;
    SimulationConfig config(num_paths, num_steps, 12345);
    
    std::cout << "\nSimulation: " << num_paths << " paths, " << num_steps << " steps\n";
    
    // Test 1: ATM Call
    {
        MarketParams params(100.0, 100.0, 0.05, 0.2, 1.0);
        EuropeanPayoff call(100.0, OptionType::Call);
        
        MonteCarloEngine mc(params, config, call);
        auto result = mc.price();
        
        Real bs_price = BlackScholes::european_price(params, OptionType::Call);
        
        print_comparison("ATM Call (S=K=100)", result.price, result.std_error, bs_price);
    }
    
    // Test 2: OTM Call
    {
        MarketParams params(100.0, 110.0, 0.05, 0.2, 1.0);
        EuropeanPayoff call(110.0, OptionType::Call);
        
        MonteCarloEngine mc(params, config, call);
        auto result = mc.price();
        
        Real bs_price = BlackScholes::european_price(params, OptionType::Call);
        
        print_comparison("OTM Call (S=100, K=110)", result.price, result.std_error, bs_price);
    }
    
    // Test 3: ITM Call
    {
        MarketParams params(100.0, 90.0, 0.05, 0.2, 1.0);
        EuropeanPayoff call(90.0, OptionType::Call);
        
        MonteCarloEngine mc(params, config, call);
        auto result = mc.price();
        
        Real bs_price = BlackScholes::european_price(params, OptionType::Call);
        
        print_comparison("ITM Call (S=100, K=90)", result.price, result.std_error, bs_price);
    }
    
    // Test 4: ATM Put
    {
        MarketParams params(100.0, 100.0, 0.05, 0.2, 1.0);
        EuropeanPayoff put(100.0, OptionType::Put);
        
        MonteCarloEngine mc(params, config, put);
        auto result = mc.price();
        
        Real bs_price = BlackScholes::european_price(params, OptionType::Put);
        
        print_comparison("ATM Put (S=K=100)", result.price, result.std_error, bs_price);
    }
    
    // Test 5: High Volatility
    {
        MarketParams params(100.0, 100.0, 0.05, 0.5, 1.0);
        EuropeanPayoff call(100.0, OptionType::Call);
        
        MonteCarloEngine mc(params, config, call);
        auto result = mc.price();
        
        Real bs_price = BlackScholes::european_price(params, OptionType::Call);
        
        print_comparison("High Vol Call (σ=50%)", result.price, result.std_error, bs_price);
    }
    
    // Test 6: Low Volatility
    {
        MarketParams params(100.0, 100.0, 0.05, 0.1, 1.0);
        EuropeanPayoff call(100.0, OptionType::Call);
        
        MonteCarloEngine mc(params, config, call);
        auto result = mc.price();
        
        Real bs_price = BlackScholes::european_price(params, OptionType::Call);
        
        print_comparison("Low Vol Call (σ=10%)", result.price, result.std_error, bs_price);
    }
    
    // Test 7: Long Maturity
    {
        MarketParams params(100.0, 100.0, 0.05, 0.2, 5.0);
        EuropeanPayoff call(100.0, OptionType::Call);
        
        MonteCarloEngine mc(params, config, call);
        auto result = mc.price();
        
        Real bs_price = BlackScholes::european_price(params, OptionType::Call);
        
        print_comparison("Long Maturity Call (T=5y)", result.price, result.std_error, bs_price);
    }
    
    // Test 8: With Dividends
    {
        MarketParams params(100.0, 100.0, 0.05, 0.2, 1.0, 0.02);
        EuropeanPayoff call(100.0, OptionType::Call);
        
        MonteCarloEngine mc(params, config, call);
        auto result = mc.price();
        
        Real bs_price = BlackScholes::european_price(params, OptionType::Call);
        
        print_comparison("Call with Dividends (q=2%)", result.price, result.std_error, bs_price);
    }
    
    std::cout << "\n==========================================================\n";
    std::cout << "Validation Complete!\n";
    std::cout << "==========================================================\n";
    
    return 0;
}