// Basic Monte Carlo pricing engine

#ifndef MC_ENGINE_PRICING_MONTE_CARLO_HPP
#define MC_ENGINE_PRICING_MONTE_CARLO_HPP

#include "mc_engine/core/types.hpp"
#include "mc_engine/core/random.hpp"
#include "mc_engine/models/gbm.hpp"
#include "mc_engine/payoffs/payoff.hpp"
#include <vector>
#include <cmath>
#include <chrono>

namespace mc_engine {

class MonteCarloEngine {
private:
    GBMPathGenerator path_gen_;
    const Payoff& payoff_;
    Real discount_factor_;
    
public:
    MonteCarloEngine(const MarketParams& params, const SimulationConfig& config, const Payoff& payoff)
        : path_gen_(params, config),
          payoff_(payoff),
          discount_factor_(std::exp(-params.rate * params.maturity)) {}
    
    // Standard Monte Carlo
    PricingResult price() const {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        const auto& config = path_gen_.get_config();
        auto rng = make_rng(config.seed);
        
        std::vector<Real> payoffs(config.num_paths);
        std::vector<Real> path;
        
        for (Size i = 0; i < config.num_paths; ++i) {
            if (payoff_.is_path_dependent()) {
                path_gen_.generate_path(path, *rng);
                payoffs[i] = payoff_(path);
            } else {
                Real terminal = path_gen_.generate_terminal_value(*rng);
                payoffs[i] = payoff_(terminal);
            }
        }
        
        auto [mean, variance] = compute_statistics(payoffs);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<Real> elapsed = end_time - start_time;
        
        PricingResult result;
        result.price = discount_factor_ * mean;
        result.variance = variance;
        result.std_error = std::sqrt(variance / config.num_paths);
        result.num_paths = config.num_paths;
        result.compute_time = elapsed.count();
        
        return result;
    }
    
    // Monte Carlo with antithetic variates
    PricingResult price_antithetic() const {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        const auto& config = path_gen_.get_config();
        
        if (config.num_paths % 2 != 0) {
            throw std::invalid_argument("Number of paths must be even");
        }
        
        auto rng = make_rng(config.seed);
        Size half_paths = config.num_paths / 2;
        
        std::vector<Real> payoffs(config.num_paths);
        std::vector<Real> path1, path2;
        std::vector<Real> randoms(config.num_steps);
        
        for (Size i = 0; i < half_paths; ++i) {
            rng->fill_gaussian(randoms);
            
            path_gen_.generate_path(path1, randoms);
            path_gen_.generate_antithetic_path(path2, randoms);
            
            payoffs[2*i] = payoff_(path1);
            payoffs[2*i + 1] = payoff_(path2);
        }
        
        auto [mean, variance] = compute_statistics(payoffs);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<Real> elapsed = end_time - start_time;
        
        PricingResult result;
        result.price = discount_factor_ * mean;
        result.variance = variance;
        result.std_error = std::sqrt(variance / config.num_paths);
        result.num_paths = config.num_paths;
        result.compute_time = elapsed.count();
        
        return result;
    }
    
    Real get_discount_factor() const {
        return discount_factor_;
    }
    
private:
    std::pair<Real, Real> compute_statistics(const std::vector<Real>& values) const {
        Size n = values.size();
        if (n == 0) {
            throw std::invalid_argument("Cannot compute statistics of empty vector");
        }
        
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

}

#endif