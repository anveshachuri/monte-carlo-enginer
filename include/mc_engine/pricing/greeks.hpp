// Greek estimation: Pathwise, Likelihood Ratio, Bump-and-Revalue

#ifndef MC_ENGINE_PRICING_GREEKS_HPP
#define MC_ENGINE_PRICING_GREEKS_HPP

#include "mc_engine/core/types.hpp"
#include "mc_engine/core/random.hpp"
#include "mc_engine/models/gbm.hpp"
#include "mc_engine/payoffs/payoff.hpp"
#include "mc_engine/payoffs/asian.hpp"
#include <vector>
#include <cmath>
#include <chrono>

namespace mc_engine {

enum class DeltaMethod {
    Pathwise,
    LikelihoodRatio,
    BumpRevalue
};

struct DeltaResult {
    Real delta;
    Real std_error;
    Real variance;
    DeltaMethod method;
    Size num_paths;
    Real compute_time;
    Real mean_payoff;
    Real mean_weight;
    
    Real confidence_interval_lower() const {
        return delta - 1.96 * std_error;
    }
    
    Real confidence_interval_upper() const {
        return delta + 1.96 * std_error;
    }
};

class DeltaEstimator {
protected:
    GBMPathGenerator path_gen_;
    const Payoff& payoff_;
    Real discount_factor_;
    MarketParams params_;
    
public:
    DeltaEstimator(const MarketParams& params, const SimulationConfig& config, const Payoff& payoff)
        : path_gen_(params, config), payoff_(payoff),
          discount_factor_(std::exp(-params.rate * params.maturity)),
          params_(params) {}
    
    virtual ~DeltaEstimator() = default;
    virtual DeltaResult estimate() const = 0;
    
protected:
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
};

// Pathwise Estimator
class PathwiseDeltaEstimator : public DeltaEstimator {
public:
    using DeltaEstimator::DeltaEstimator;
    
    DeltaResult estimate() const override {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        const auto& config = path_gen_.get_config();
        auto rng = make_rng(config.seed);
        
        std::vector<Real> delta_estimates(config.num_paths);
        std::vector<Real> payoffs(config.num_paths);
        std::vector<Real> path;
        
        Real S0 = params_.spot;
        OptionType opt_type = payoff_.get_type();
        
        for (Size i = 0; i < config.num_paths; ++i) {
            if (payoff_.is_path_dependent()) {
                // Path-dependent (e.g., Asian)
                path_gen_.generate_path(path, *rng);
                
                // Check if it's an Asian option to use specialized computation
                const AsianArithmeticPayoff* asian = dynamic_cast<const AsianArithmeticPayoff*>(&payoff_);
                if (asian) {
                    Real average = asian->compute_average(path);
                    
                    // Asian call or put
                    Real payoff_val, derivative;
                    if (opt_type == OptionType::Call) {
                        payoff_val = std::max(average - params_.strike, 0.0);
                        // ∂payoff/∂S_0 = 1{A > K} × (∂A/∂S_0)
                        if (average > params_.strike) {
                            Real sum_spots = 0.0;
                            for (const Real& spot : path) {
                                sum_spots += spot;
                            }
                            derivative = sum_spots / (S0 * path.size());
                        } else {
                            derivative = 0.0;
                        }
                    } else {
                        // Asian Put
                        payoff_val = std::max(params_.strike - average, 0.0);
                        // ∂payoff/∂S_0 = -1{K > A} × (∂A/∂S_0)
                        if (params_.strike > average) {
                            Real sum_spots = 0.0;
                            for (const Real& spot : path) {
                                sum_spots += spot;
                            }
                            derivative = -sum_spots / (S0 * path.size());
                        } else {
                            derivative = 0.0;
                        }
                    }
                    
                    payoffs[i] = payoff_val;
                    delta_estimates[i] = derivative;
                } else {
                    // Generic path-dependent option (not implemented)
                    payoffs[i] = payoff_(path);
                    delta_estimates[i] = 0.0;  // Would need specific implementation
                }
            } else {
                // Path-independent (e.g., European)
                path_gen_.generate_path(path, *rng);
                Real terminal = path.back();
                Real payoff_val = payoff_(terminal);
                payoffs[i] = payoff_val;
                
                // European derivatives
                Real derivative;
                if (opt_type == OptionType::Call) {
                    // Call: ∂max(S_T - K, 0)/∂S_0 = 1{S_T > K} × (S_T / S_0)
                    if (terminal > params_.strike) {
                        derivative = terminal / S0;
                    } else {
                        derivative = 0.0;
                    }
                } else {
                    // Put: ∂max(K - S_T, 0)/∂S_0 = -1{K > S_T} × (S_T / S_0)
                    if (params_.strike > terminal) {
                        derivative = -terminal / S0;
                    } else {
                        derivative = 0.0;
                    }
                }
                
                delta_estimates[i] = derivative;
            }
        }
        
        auto [mean_delta, var_delta] = compute_statistics(delta_estimates);
        auto [mean_payoff, var_payoff] = compute_statistics(payoffs);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<Real> elapsed = end_time - start_time;
        
        DeltaResult result;
        result.delta = discount_factor_ * mean_delta;
        result.std_error = discount_factor_ * std::sqrt(var_delta / config.num_paths);
        result.variance = var_delta;
        result.method = DeltaMethod::Pathwise;
        result.num_paths = config.num_paths;
        result.compute_time = elapsed.count();
        result.mean_payoff = mean_payoff;
        result.mean_weight = 0.0;
        
        return result;
    }
};

// Likelihood Ratio Estimator
class LikelihoodRatioDeltaEstimator : public DeltaEstimator {
public:
    using DeltaEstimator::DeltaEstimator;
    
    DeltaResult estimate() const override {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        const auto& config = path_gen_.get_config();
        auto rng = make_rng(config.seed);
        
        std::vector<Real> delta_estimates(config.num_paths);
        std::vector<Real> payoffs(config.num_paths);
        std::vector<Real> scores(config.num_paths);
        std::vector<Real> path;
        std::vector<Real> randoms(config.num_steps);
        
        Real S0 = params_.spot;
        Real sigma = params_.volatility;
        Real T = params_.maturity;
        Real r = params_.rate;
        Real q = params_.dividend;
        
        for (Size i = 0; i < config.num_paths; ++i) {
            rng->fill_gaussian(randoms);
            path_gen_.generate_path(path, randoms);
            
            // Compute payoff using the actual payoff function
            Real payoff_val;
            if (payoff_.is_path_dependent()) {
                payoff_val = payoff_(path);
            } else {
                payoff_val = payoff_(path.back());
            }
            payoffs[i] = payoff_val;
            
            Real score;
            if (payoff_.is_path_dependent()) {
                // Path-dependent: use sum over increments (high variance but unbiased for path)
                Real log_sum = 0.0;
                Real dt = T / config.num_steps;
                Real mu = r - q - 0.5 * sigma * sigma;

                for (Size j = 1; j < path.size(); ++j) {
                    Real increment = std::log(path[j] / path[j-1]);
                    log_sum += (increment - mu * dt);
                }

                score = log_sum / (sigma * sigma * dt * S0);

            } else {
                // Path-independent: use terminal-based approximation
                // Note: This is still approximate - exact formula depends on discretization scheme
                Real ST = path.back();
                Real mu = r - q - 0.5 * sigma * sigma;
                Real log_return = std::log(ST / S0);
                score = (log_return - mu * T) / (sigma * sigma * T * S0);
            }
            scores[i] = score;
            
            // Likelihood ratio estimate
            delta_estimates[i] = payoff_val * score;
        }
        
        auto [mean_delta, var_delta] = compute_statistics(delta_estimates);
        auto [mean_payoff, var_payoff] = compute_statistics(payoffs);
        auto [mean_score, var_score] = compute_statistics(scores);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<Real> elapsed = end_time - start_time;
        
        DeltaResult result;
        result.delta = discount_factor_ * mean_delta;
        result.std_error = discount_factor_ * std::sqrt(var_delta / config.num_paths);
        result.variance = var_delta;
        result.method = DeltaMethod::LikelihoodRatio;
        result.num_paths = config.num_paths;
        result.compute_time = elapsed.count();
        result.mean_payoff = mean_payoff;
        result.mean_weight = mean_score;
        
        return result;
    }
};

// Bump-and-Revalue Estimator
class BumpRevalueDeltaEstimator : public DeltaEstimator {
private:
    Real bump_size_;
    
public:
    BumpRevalueDeltaEstimator(const MarketParams& params, const SimulationConfig& config,
                              const Payoff& payoff, Real bump_size = 0.01)
        : DeltaEstimator(params, config, payoff), bump_size_(bump_size) {}
    
    DeltaResult estimate() const override {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        const auto& config = path_gen_.get_config();
        auto rng = make_rng(config.seed);
        
        Real S0 = params_.spot;
        Real h = bump_size_ * S0;
        
        std::vector<Real> delta_estimates(config.num_paths);
        std::vector<Real> path;
        std::vector<Real> randoms(config.num_steps);
        
        for (Size i = 0; i < config.num_paths; ++i) {
            rng->fill_gaussian(randoms);
            
            // Price at S₀ + h
            MarketParams params_up = params_;
            params_up.spot = S0 + h;
            GBMPathGenerator gen_up(params_up, config);
            gen_up.generate_path(path, randoms);
            Real payoff_up = payoff_.is_path_dependent() ? payoff_(path) : payoff_(path.back());
            
            // Price at S₀ - h
            MarketParams params_down = params_;
            params_down.spot = S0 - h;
            GBMPathGenerator gen_down(params_down, config);
            gen_down.generate_path(path, randoms);
            Real payoff_down = payoff_.is_path_dependent() ? payoff_(path) : payoff_(path.back());
            
            // Central difference
            delta_estimates[i] = (payoff_up - payoff_down) / (2.0 * h);
        }
        
        auto [mean_delta, var_delta] = compute_statistics(delta_estimates);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<Real> elapsed = end_time - start_time;
        
        DeltaResult result;
        result.delta = discount_factor_ * mean_delta;
        result.std_error = discount_factor_ * std::sqrt(var_delta / config.num_paths);
        result.variance = var_delta;
        result.method = DeltaMethod::BumpRevalue;
        result.num_paths = config.num_paths;
        result.compute_time = elapsed.count();
        result.mean_payoff = 0.0;
        result.mean_weight = 0.0;
        
        return result;
    }
};

} 

#endif 