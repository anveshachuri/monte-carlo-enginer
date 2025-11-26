// Demonstrate control variates for variance reduction

#include "mc_engine/core/types.hpp"
#include "mc_engine/models/gbm.hpp"
#include "mc_engine/models/black_scholes.hpp"
#include "mc_engine/payoffs/european.hpp"
#include "mc_engine/payoffs/asian.hpp"
#include "mc_engine/pricing/monte_carlo.hpp"
#include "mc_engine/pricing/variance_reduction.hpp"
#include <iostream>
#include <iomanip>

using namespace mc_engine;

void print_cv_result(const std::string& name, const ControlVariateResult& result) {
    std::cout << "\n" << name << ":\n";
    std::cout << std::string(70, '=') << "\n";
    std::cout << std::fixed << std::setprecision(6);
    
    std::cout << "\nStandard MC:\n";
    std::cout << "  Price:       " << result.price_standard << "\n";
    std::cout << "  Std Error:   " << result.std_error_standard << "\n";
    std::cout << "  Variance:    " << result.variance_standard << "\n";
    
    std::cout << "\nControl Variate MC:\n";
    std::cout << "  Price:       " << result.price_cv << "\n";
    std::cout << "  Std Error:   " << result.std_error_cv << "\n";
    std::cout << "  Variance:    " << result.variance_cv << "\n";
    std::cout << "  95% CI:      [" << result.confidence_interval_lower()
              << ", " << result.confidence_interval_upper() << "]\n";
    
    std::cout << "\nControl Variate Details:\n";
    std::cout << "  Beta (β):         " << result.beta << "\n";
    std::cout << "  Correlation (ρ):  " << result.correlation << "\n";
    
    std::cout << "\nPerformance:\n";
    std::cout << "  VRF:              " << result.vrf << "x\n";
    std::cout << "  Stderr Reduction: " << result.std_error_standard / result.std_error_cv << "x\n";
    std::cout << "  Compute Time:     " << result.compute_time << " seconds\n";
    
    Real theoretical_vrf = 1.0 / (1.0 - result.correlation * result.correlation);
    std::cout << "  Theoretical VRF:  " << theoretical_vrf << "x\n";
}

int main() {
    std::cout << "Control Variates: Using European Call to Price Asian Call\n";
    
    MarketParams params(100.0, 100.0, 0.05, 0.2, 1.0);
    
    std::cout << "\nMarket Parameters:\n";
    std::cout << "  S0 = " << params.spot << "\n";
    std::cout << "  K = " << params.strike << "\n";
    std::cout << "  r = " << params.rate << "\n";
    std::cout << "  σ = " << params.volatility << "\n";
    std::cout << "  T = " << params.maturity << "\n";
    
    Size num_paths = 100000;
    Size num_steps = 252;
    SimulationConfig config(num_paths, num_steps, 42);
    
    AsianArithmeticPayoff asian_call(params.strike, OptionType::Call, true);
    EuropeanPayoff european_call(params.strike, OptionType::Call);
    
    Real european_true = BlackScholes::european_price(params, OptionType::Call);
    
    std::cout << "\nControl Variable:\n";
    std::cout << "  European Call Analytical Price: " << european_true << "\n";
    
    // Baseline
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "Baseline: Standard Monte Carlo\n";
    std::cout << std::string(70, '=') << "\n";
    
    MonteCarloEngine mc_baseline(params, config, asian_call);
    auto baseline_result = mc_baseline.price();
    
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "\nAsian Call (Standard MC):\n";
    std::cout << "  Price:       " << baseline_result.price << "\n";
    std::cout << "  Std Error:   " << baseline_result.std_error << "\n";
    std::cout << "  Variance:    " << baseline_result.variance << "\n";
    std::cout << "  95% CI:      [" << baseline_result.confidence_interval_lower()
              << ", " << baseline_result.confidence_interval_upper() << "]\n";
    std::cout << "  Time:        " << baseline_result.compute_time << " seconds\n";
    
    // Control Variates
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "Method: Control Variates\n";
    std::cout << std::string(70, '=') << "\n";
    
    ControlVariatePricer cv_pricer(params, config, asian_call, european_call, european_true);
    auto cv_result = cv_pricer.price();
    
    print_cv_result("Control Variate Results", cv_result);
    
    // Summary
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "SUMMARY\n";
    std::cout << std::string(70, '=') << "\n";
    
    std::cout << "\n" << std::setw(25) << "Method"
              << std::setw(15) << "Price"
              << std::setw(15) << "Std Error"
              << std::setw(15) << "VRF\n";
    std::cout << std::string(70, '-') << "\n";
    
    std::cout << std::setw(25) << "Standard MC"
              << std::setw(15) << baseline_result.price
              << std::setw(15) << baseline_result.std_error
              << std::setw(15) << "1.00x\n";
    
    std::cout << std::setw(25) << "Control Variates"
              << std::setw(15) << cv_result.price_cv
              << std::setw(15) << cv_result.std_error_cv
              << std::setw(15) << cv_result.vrf << "x\n";
    
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "Key Insights:\n";
    std::cout << std::string(70, '=') << "\n";
    
    Real cv_improvement = baseline_result.std_error / cv_result.std_error_cv;
    
    std::cout << "\n1. Control variates reduce stderr by " << cv_improvement << "x\n";
    std::cout << "2. Correlation (ρ) = " << cv_result.correlation 
              << " → High correlation = effective control!\n";
    std::cout << "3. Optimal β = " << cv_result.beta 
              << " → Large |β| means strong linear relationship\n";
    
    Real equivalent_paths = baseline_result.variance / cv_result.variance_cv;
    std::cout << "\n4. Control variates with " << num_paths << " paths ≈ Standard MC with "
              << static_cast<Size>(equivalent_paths * num_paths) << " paths!\n";
    
    return 0;
}