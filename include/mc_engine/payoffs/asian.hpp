// include/mc_engine/payoffs/asian.hpp
// Asian option payoffs (arithmetic and geometric)

#ifndef MC_ENGINE_PAYOFFS_ASIAN_HPP
#define MC_ENGINE_PAYOFFS_ASIAN_HPP

#include "payoff.hpp"
#include <numeric>
#include <algorithm>
#include <cmath>

namespace mc_engine {

// Asian Arithmetic Average
class AsianArithmeticPayoff : public Payoff {
private:
    Real strike_;
    OptionType type_;
    bool include_spot_;
    
public:
    AsianArithmeticPayoff(Real strike, OptionType type = OptionType::Call, bool include_spot = true)
        : strike_(strike), type_(type), include_spot_(include_spot) {
        if (strike <= 0.0) {
            throw std::invalid_argument("Strike must be positive");
        }
    }
    
    Real operator()(const std::vector<Real>& path) const override {
        if (path.empty()) {
            throw std::invalid_argument("Path cannot be empty");
        }
        
        Real average = compute_average(path);
        
        if (type_ == OptionType::Call) {
            return std::max(average - strike_, 0.0);
        } else {
            return std::max(strike_ - average, 0.0);
        }
    }
    
    Real operator()(Real terminal_value) const override {
        throw std::runtime_error("Asian option requires full path");
    }
    
    bool is_path_dependent() const override {
        return true;
    }
    
    Real get_strike() const override {
        return strike_;
    }
    
    OptionType get_type() const override {
        return type_;
    }
    
    Real compute_average(const std::vector<Real>& path) const {
        Size start_idx = include_spot_ ? 0 : 1;
        Size count = path.size() - start_idx;
        
        if (count == 0) {
            throw std::invalid_argument("No points to average");
        }
        
        // Kahan summation for numerical stability
        Real sum = 0.0;
        Real compensation = 0.0;
        
        for (Size i = start_idx; i < path.size(); ++i) {
            Real y = path[i] - compensation;
            Real t = sum + y;
            compensation = (t - sum) - y;
            sum = t;
        }
        
        return sum / static_cast<Real>(count);
    }
    
    bool includes_spot() const {
        return include_spot_;
    }
};

// Asian Geometric Average
class AsianGeometricPayoff : public Payoff {
private:
    Real strike_;
    OptionType type_;
    bool include_spot_;
    
public:
    AsianGeometricPayoff(Real strike, OptionType type = OptionType::Call, bool include_spot = true)
        : strike_(strike), type_(type), include_spot_(include_spot) {
        if (strike <= 0.0) {
            throw std::invalid_argument("Strike must be positive");
        }
    }
    
    Real operator()(const std::vector<Real>& path) const override {
        if (path.empty()) {
            throw std::invalid_argument("Path cannot be empty");
        }
        
        Real average = compute_geometric_average(path);
        
        if (type_ == OptionType::Call) {
            return std::max(average - strike_, 0.0);
        } else {
            return std::max(strike_ - average, 0.0);
        }
    }
    
    Real operator()(Real terminal_value) const override {
        throw std::runtime_error("Asian option requires full path");
    }
    
    bool is_path_dependent() const override {
        return true;
    }
    
    Real get_strike() const override {
        return strike_;
    }
    
    OptionType get_type() const override {
        return type_;
    }
    
    Real compute_geometric_average(const std::vector<Real>& path) const {
        Size start_idx = include_spot_ ? 0 : 1;
        Size count = path.size() - start_idx;
        
        if (count == 0) {
            throw std::invalid_argument("No points to average");
        }
        
        // Compute in log space for stability
        Real log_sum = 0.0;
        for (Size i = start_idx; i < path.size(); ++i) {
            log_sum += std::log(path[i]);
        }
        
        return std::exp(log_sum / static_cast<Real>(count));
    }
    
    bool includes_spot() const {
        return include_spot_;
    }
};

} // namespace mc_engine

#endif // MC_ENGINE_PAYOFFS_ASIAN_HPP