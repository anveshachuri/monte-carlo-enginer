// include/mc_engine/pricing/variance_reduction.hpp
// Control variates implementation

#ifndef MC_ENGINE_PRICING_VARIANCE_REDUCTION_HPP
#define MC_ENGINE_PRICING_VARIANCE_REDUCTION_HPP

#include "mc_engine/core/types.hpp"
#include "mc_engine/core/random.hpp"
#include "mc_engine/models/gbm.hpp"
#include "mc_engine/payoffs/payoff.hpp"
#include <vector>
#include <cmath>
#include <chrono>

namespace mc_engine {

struct ControlVariateResult {
    Real price_standard;
    Real price_cv;
    Real std_error_standard;
    Real std_error_cv;
    Real variance_standard;
    Real variance_cv;
    Real beta;
    Real correlation;
    Real vrf;
    Size num_paths;
    Real compute_time;
    
    Real confidence_interval_lower() const {
        return price_cv - 1.96 * std_error_cv;
    }
    
    Real confidence_interval_upper() const {
        return price_cv + 1.96 * std_error_cv;
    }
};

class ControlVariatePricer {
private:
    GBMPathGenerator path_gen_;
    const Payoff& target_payoff_;
    const Payoff& control_payoff_;
    Real discount_factor_;
    Real control_true_value_;
    
public:
    ControlVariatePricer(const MarketParams& params, const SimulationConfig& config,
                        const Payoff& target_payoff, const Payoff& control_payoff,
                        Real control_true_value)
        : path_gen_(params, config), target_payoff_(target_payoff),
          control_payoff_(control_payoff),
          discount_factor_(std::exp(-params.rate * params.maturity)),
          control_true_value_(control_true_value) {}
    
    ControlVariateResult price() const {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        const auto& config = path_gen_.get_config();
        auto rng = make_rng(config.seed);
        
        std::vector<Real> target_payoffs(config.num_paths);
        std::vector<Real> control_payoffs(config.num_paths);
        std::vector<Real> path;
        
        for (Size i = 0; i < config.num_paths; ++i) {
            path_gen_.generate_path(path, *rng);
            target_payoffs[i] = target_payoff_(path);
            control_payoffs[i] = control_payoff_(path);
        }
        
        auto [mean_target, var_target] = compute_statistics(target_payoffs);
        auto [mean_control, var_control] = compute_statistics(control_payoffs);
        
        Real cov = compute_covariance(target_payoffs, control_payoffs, mean_target, mean_control);
        
        Real beta = 0.0;
        if (var_control > 1e-10) {
            beta = cov / var_control;
        }
        
        std::vector<Real> cv_payoffs(config.num_paths);
        for (Size i = 0; i < config.num_paths; ++i) {
            cv_payoffs[i] = target_payoffs[i] - 
                           beta * (control_payoffs[i] - control_true_value_ / discount_factor_);
        }
        
        auto [mean_cv, var_cv] = compute_statistics(cv_payoffs);
        
        Real correlation = 0.0;
        if (var_target > 1e-10 && var_control > 1e-10) {
            correlation = cov / std::sqrt(var_target * var_control);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<Real> elapsed = end_time - start_time;
        
        ControlVariateResult result;
        result.price_standard = discount_factor_ * mean_target;
        result.price_cv = discount_factor_ * mean_cv;
        result.std_error_standard = discount_factor_ * std::sqrt(var_target / config.num_paths);
        result.std_error_cv = discount_factor_ * std::sqrt(var_cv / config.num_paths);
        result.variance_standard = var_target;
        result.variance_cv = var_cv;
        result.beta = beta;
        result.correlation = correlation;
        result.vrf = var_target / var_cv;
        result.num_paths = config.num_paths;
        result.compute_time = elapsed.count();
        
        return result;
    }
    
private:
    std::pair<Real, Real> compute_statistics(const std::vector<Real>& values) const {
        Size n = values.size();
        Real mean = 0.0;
        Real M2 = 0.0;
        
        for (Size i = 0; i < n; ++i) {
            Real delta = values[i] - mean;
            mean += delta / static_cast<Real>(i + 1);
            Real delta2 = values[i] - mean;
            M2 += delta * delta2;
        }
        
        Real variance = (n > 1) ? M2 / static_cast<Real>(n - 1) : 0.0;
        return {mean, variance};
    }
    
    Real compute_covariance(const std::vector<Real>& x, const std::vector<Real>& y,
                           Real mean_x, Real mean_y) const {
        Size n = x.size();
        Real cov = 0.0;
        for (Size i = 0; i < n; ++i) {
            cov += (x[i] - mean_x) * (y[i] - mean_y);
        }
        return (n > 1) ? cov / static_cast<Real>(n - 1) : 0.0;
    }
};

} // namespace mc_engine

#endif // MC_ENGINE_PRICING_VARIANCE_REDUCTION_HPP