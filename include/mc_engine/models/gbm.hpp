// Geometric Brownian Motion path generator

#ifndef MC_ENGINE_MODELS_GBM_HPP
#define MC_ENGINE_MODELS_GBM_HPP

#include "mc_engine/core/types.hpp"
#include "mc_engine/core/random.hpp"
#include <vector>
#include <cmath>

namespace mc_engine {

class GBMPathGenerator {
private:
    MarketParams params_;
    SimulationConfig config_;
    Real dt_;
    Real drift_;
    Real diffusion_;
    
public:
    GBMPathGenerator(const MarketParams& params, const SimulationConfig& config): params_(params), config_(config) {
        dt_ = config_.dt(params_.maturity);
        drift_ = (params_.rate - params_.dividend - 0.5 * params_.volatility * params_.volatility) * dt_;
        diffusion_ = params_.volatility * std::sqrt(dt_);
    }
    
    void generate_path(std::vector<Real>& path, RandomGenerator& rng) const {
        path.resize(config_.num_steps + 1);
        path[0] = params_.spot;
        
        for (Size i = 1; i <= config_.num_steps; ++i) {
            Real z = rng.next_gaussian();
            path[i] = path[i-1] * std::exp(drift_ + diffusion_ * z);
        }
    }
    
    void generate_path(std::vector<Real>& path, const std::vector<Real>& randoms) const {
        if (randoms.size() != config_.num_steps) {
            throw std::invalid_argument("Random vector size must match num_steps");
        }
        
        path.resize(config_.num_steps + 1);
        path[0] = params_.spot;
        
        for (Size i = 1; i <= config_.num_steps; ++i) {
            path[i] = path[i-1] * std::exp(drift_ + diffusion_ * randoms[i-1]);
        }
    }
    
    void generate_antithetic_path(std::vector<Real>& path, const std::vector<Real>& randoms) const {
        if (randoms.size() != config_.num_steps) {
            throw std::invalid_argument("Random vector size must match num_steps");
        }
        
        path.resize(config_.num_steps + 1);
        path[0] = params_.spot;
        
        for (Size i = 1; i <= config_.num_steps; ++i) {
            path[i] = path[i-1] * std::exp(drift_ + diffusion_ * (-randoms[i-1]));
        }
    }
    
    void generate_paths(PathData& paths, RandomGenerator& rng) const {
        if (paths.num_paths != config_.num_paths || paths.num_steps != config_.num_steps) {
            throw std::invalid_argument("PathData dimensions must match config");
        }
        
        for (Size i = 0; i < config_.num_paths; ++i) {
            paths(i, 0) = params_.spot;
        }
        
        for (Size i = 0; i < config_.num_paths; ++i) {
            for (Size j = 1; j <= config_.num_steps; ++j) {
                Real z = rng.next_gaussian();
                paths(i, j) = paths(i, j-1) * std::exp(drift_ + diffusion_ * z);
            }
        }
    }
    
    Real generate_terminal_value(RandomGenerator& rng) const {
        Real total_drift = (params_.rate - params_.dividend - 
                           0.5 * params_.volatility * params_.volatility) * params_.maturity;
        Real total_diffusion = params_.volatility * std::sqrt(params_.maturity);
        Real z = rng.next_gaussian();
        
        return params_.spot * std::exp(total_drift + total_diffusion * z);
    }
    
    Real get_dt() const { return dt_; }
    Real get_drift() const { return drift_; }
    Real get_diffusion() const { return diffusion_; }
    const MarketParams& get_params() const { return params_; }
    const SimulationConfig& get_config() const { return config_; }
};

}

#endif