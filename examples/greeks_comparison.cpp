// Compare three Delta estimation methods

#include "mc_engine/core/types.hpp"
#include "mc_engine/models/gbm.hpp"
#include "mc_engine/models/black_scholes.hpp"
#include "mc_engine/payoffs/european.hpp"
#include "mc_engine/payoffs/asian.hpp"
#include "mc_engine/pricing/greeks.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>

using namespace mc_engine;

std::string method_name(DeltaMethod method) {
    switch (method) {
        case DeltaMethod::Pathwise: return "Pathwise";
        case DeltaMethod::LikelihoodRatio: return "Likelihood Ratio";
        case DeltaMethod::BumpRevalue: return "Bump-and-Revalue";
        default: return "Unknown";
    }
}

void print_delta_result(const DeltaResult& result, Real true_delta = -1.0) {
    std::cout << "\n" << method_name(result.method) << " Method:\n";
    std::cout << std::string(60, '-') << "\n";
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "  Delta:       " << result.delta << "\n";
    std::cout << "  Std Error:   " << result.std_error << "\n";
    std::cout << "  95% CI:      [" << result.confidence_interval_lower()
              << ", " << result.confidence_interval_upper() << "]\n";
    std::cout << "  Variance:    " << result.variance << "\n";
    std::cout << "  Time:        " << result.compute_time << " seconds\n";
    
    if (true_delta > 0.0) {
        Real error = result.delta - true_delta;
        Real rel_error = std::abs(error) / true_delta * 100.0;
        Real z_score = error / result.std_error;
        
        std::cout << "  True Delta:  " << true_delta << "\n";
        std::cout << "  Error:       " << error << "\n";
        std::cout << "  Rel Error:   " << rel_error << "%\n";
        std::cout << "  Z-score:     " << z_score << " σ\n";
        
        if (std::abs(z_score) < 2.0) {
            std::cout << "  ✓ Within 95% confidence interval\n";
        } else {
            std::cout << "  ⚠ Outside 95% confidence interval\n";
        }
    }
}
void export_greeks_csv() {
    // Run Greeks for European call
    MarketParams params(100.0, 100.0, 0.05, 0.2, 1.0);
    SimulationConfig config(100000, 100, 42);
    EuropeanPayoff call(100.0, OptionType::Call);
    
    Real true_delta = BlackScholes::delta(params, OptionType::Call);
    
    PathwiseDeltaEstimator pw(params, config, call);
    LikelihoodRatioDeltaEstimator lr(params, config, call);
    BumpRevalueDeltaEstimator bump(params, config, call, 0.01);
    
    auto pw_result = pw.estimate();
    auto lr_result = lr.estimate();
    auto bump_result = bump.estimate();
    
    // Export CSV
    std::ofstream file("greeks_comparison.csv");
    file << "method,delta,std_error,variance,true_delta\n";
    file << "Pathwise," << pw_result.delta << "," << pw_result.std_error 
         << "," << pw_result.variance << "," << true_delta << "\n";
    file << "LikelihoodRatio," << lr_result.delta << "," << lr_result.std_error 
         << "," << lr_result.variance << "," << true_delta << "\n";
    file << "BumpRevalue," << bump_result.delta << "," << bump_result.std_error 
         << "," << bump_result.variance << "," << true_delta << "\n";
    file.close();
}

void export_variance_vs_strike() {
    std::ofstream file("greeks_variance_vs_strike.csv");
    file << "strike,method,variance,moneyness\n";
    
    std::vector<Real> strikes = {80.0, 90.0, 100.0, 110.0, 120.0};
    
    for (Real K : strikes) {
        MarketParams params(100.0, K, 0.05, 0.2, 1.0);
        SimulationConfig config(50000, 252, 42);
        AsianArithmeticPayoff asian(K, OptionType::Call);
        
        PathwiseDeltaEstimator pw(params, config, asian);
        LikelihoodRatioDeltaEstimator lr(params, config, asian);
        BumpRevalueDeltaEstimator bump(params, config, asian, 0.01);
        
        auto pw_result = pw.estimate();
        auto lr_result = lr.estimate();
        auto bump_result = bump.estimate();
        
        Real moneyness = 100.0 / K;
        
        file << K << ",Pathwise," << pw_result.variance << "," << moneyness << "\n";
        file << K << ",LikelihoodRatio," << lr_result.variance << "," << moneyness << "\n";
        file << K << ",BumpRevalue," << bump_result.variance << "," << moneyness << "\n";
    }
    
    file.close();
}

int main() {

    std::cout << "Greeks Estimation: Comparing Three Delta Methods\n";
    
    Size num_paths = 100000;
    
    // Test 1: European call (validation)
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "TEST 1: European Call (Validation Against Black-Scholes)\n";
    std::cout << std::string(70, '=') << "\n";
    
    {
        MarketParams params(100.0, 100.0, 0.05, 0.2, 1.0);
        SimulationConfig config(num_paths, 100, 42);
        
        std::cout << "\nParameters: S=" << params.spot << ", K=" << params.strike
                  << ", r=" << params.rate << ", σ=" << params.volatility
                  << ", T=" << params.maturity << "\n";
        
        EuropeanPayoff call(params.strike, OptionType::Call);
        Real true_delta = BlackScholes::delta(params, OptionType::Call);
        
        std::cout << "\nBlack-Scholes Delta: " << true_delta << "\n";
        
        PathwiseDeltaEstimator pathwise(params, config, call);
        auto pw_result = pathwise.estimate();
        print_delta_result(pw_result, true_delta);
        
        LikelihoodRatioDeltaEstimator lr(params, config, call);
        auto lr_result = lr.estimate();
        print_delta_result(lr_result, true_delta);
        
        BumpRevalueDeltaEstimator bump(params, config, call, 0.01);
        auto bump_result = bump.estimate();
        print_delta_result(bump_result, true_delta);
    }
    
    // Test 2: Asian call
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "TEST 2: Asian Call (No Closed-Form Solution)\n";
    std::cout << std::string(70, '=') << "\n";
    
    {
        MarketParams params(100.0, 100.0, 0.05, 0.2, 1.0);
        SimulationConfig config(num_paths, 252, 42);
        
        std::cout << "\nParameters: S=" << params.spot << ", K=" << params.strike
                  << ", r=" << params.rate << ", σ=" << params.volatility
                  << ", T=" << params.maturity << "\n";
        
        AsianArithmeticPayoff asian_call(params.strike, OptionType::Call);
        
        std::cout << "\n(No analytical solution available for comparison)\n";
        
        PathwiseDeltaEstimator pathwise(params, config, asian_call);
        auto pw_result = pathwise.estimate();
        print_delta_result(pw_result);
        
        LikelihoodRatioDeltaEstimator lr(params, config, asian_call);
        auto lr_result = lr.estimate();
        print_delta_result(lr_result);
        
        BumpRevalueDeltaEstimator bump(params, config, asian_call, 0.01);
        auto bump_result = bump.estimate();
        print_delta_result(bump_result);
    }
    
    // Test 3: Variance comparison across moneyness
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "TEST 3: Variance Comparison Across Moneyness\n";
    std::cout << std::string(70, '=') << "\n";
    
    {
        Size test_paths = 50000;
        SimulationConfig config(test_paths, 252, 42);
        
        std::vector<Real> strikes = {80.0, 90.0, 100.0, 110.0, 120.0};
        
        std::cout << "\n" << std::setw(10) << "Strike"
                  << std::setw(15) << "Moneyness"
                  << std::setw(15) << "PW Var"
                  << std::setw(15) << "LR Var"
                  << std::setw(15) << "Bump Var"
                  << std::setw(12) << "PW/LR\n";
        std::cout << std::string(82, '-') << "\n";
        
        for (Real K : strikes) {
            MarketParams params(100.0, K, 0.05, 0.2, 1.0);
            AsianArithmeticPayoff asian_call(K, OptionType::Call);
            
            PathwiseDeltaEstimator pathwise(params, config, asian_call);
            LikelihoodRatioDeltaEstimator lr(params, config, asian_call);
            BumpRevalueDeltaEstimator bump(params, config, asian_call, 0.01);
            
            auto pw_result = pathwise.estimate();
            auto lr_result = lr.estimate();
            auto bump_result = bump.estimate();
            
            Real moneyness = 100.0 / K;
            Real var_ratio = pw_result.variance / lr_result.variance;
            
            std::cout << std::fixed << std::setprecision(4)
                      << std::setw(10) << K
                      << std::setw(15) << moneyness
                      << std::setw(15) << pw_result.variance
                      << std::setw(15) << lr_result.variance
                      << std::setw(15) << bump_result.variance
                      << std::setw(12) << var_ratio << "\n";
        }
    }
    
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "KEY FINDINGS\n";
    std::cout << std::string(70, '=') << "\n";
    
    std::cout << "\n1. PATHWISE METHOD:\n";
    std::cout << "   ✓ Lowest variance for smooth payoffs\n";
    std::cout << "   ✓ Most efficient for Asian options\n";
    std::cout << "   ✗ Requires differentiable payoff\n";
    std::cout << "   ✗ Fails for barriers/digitals\n";
    
    std::cout << "\n2. LIKELIHOOD RATIO METHOD:\n";
    std::cout << "   ✓ Works for ANY payoff (including discontinuous)\n";
    std::cout << "   ✓ Unbiased estimator\n";
    std::cout << "   ✗ Higher variance (especially OTM)\n";
    std::cout << "   ✗ Requires storing random numbers\n";
    
    std::cout << "\n3. BUMP-AND-REVALUE:\n";
    std::cout << "   ✓ Simplest to implement\n";
    std::cout << "   ✓ Works for any payoff\n";
    std::cout << "   ✗ 2-3× more paths required\n";
    std::cout << "   ✗ Numerical stability issues\n";
    std::cout << "   ✗ Choice of bump size matters\n";
    
    std::cout << "\n4. RECOMMENDATIONS:\n";
    std::cout << "   • Asian/European options → Use PATHWISE\n";
    std::cout << "   • Barrier/Digital options → Use LIKELIHOOD RATIO\n";
    std::cout << "   • Quick prototyping → Use BUMP-AND-REVALUE\n";
    std::cout << "   • Production systems → Use PATHWISE + variance reduction\n";
    
    export_greeks_csv();
    export_variance_vs_strike();

    std::cout << "\nGenerated greeks_comparison.csv\n";
    std::cout << "Generated greeks_variance_vs_strike.csv\n";
    return 0;
    
}