// include/mc_engine/core/statistics.hpp
// Statistical utilities for Monte Carlo analysis

#ifndef MC_ENGINE_CORE_STATISTICS_HPP
#define MC_ENGINE_CORE_STATISTICS_HPP

#include "types.hpp"
#include <vector>
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace mc_engine {

class Statistics {
public:
    // Compute mean and standard error
    static std::pair<Real, Real> mean_and_stderr(const std::vector<Real>& data) {
        if (data.empty()) {
            throw std::invalid_argument("Cannot compute statistics of empty data");
        }
        
        Size n = data.size();
        Real mean = 0.0;
        Real M2 = 0.0;
        
        // Welford's online algorithm
        for (Size i = 0; i < n; ++i) {
            Real delta = data[i] - mean;
            mean += delta / static_cast<Real>(i + 1);
            Real delta2 = data[i] - mean;
            M2 += delta * delta2;
        }
        
        Real variance = (n > 1) ? M2 / static_cast<Real>(n - 1) : 0.0;
        Real std_error = std::sqrt(variance / n);
        
        return {mean, std_error};
    }
    
    // Full statistics structure
    struct FullStatistics {
        Real mean;
        Real variance;
        Real std_dev;
        Real std_error;
        Real min;
        Real max;
        Size count;
    };
    
    // Compute full statistics
    static FullStatistics compute_full(const std::vector<Real>& data) {
        if (data.empty()) {
            throw std::invalid_argument("Cannot compute statistics of empty data");
        }
        
        Size n = data.size();
        Real mean = 0.0;
        Real M2 = 0.0;
        Real min_val = data[0];
        Real max_val = data[0];
        
        for (Size i = 0; i < n; ++i) {
            Real delta = data[i] - mean;
            mean += delta / static_cast<Real>(i + 1);
            Real delta2 = data[i] - mean;
            M2 += delta * delta2;
            
            min_val = std::min(min_val, data[i]);
            max_val = std::max(max_val, data[i]);
        }
        
        Real variance = (n > 1) ? M2 / static_cast<Real>(n - 1) : 0.0;
        Real std_dev = std::sqrt(variance);
        Real std_error = std_dev / std::sqrt(static_cast<Real>(n));
        
        return {mean, variance, std_dev, std_error, min_val, max_val, n};
    }
    
    // Confidence interval
    static std::pair<Real, Real> confidence_interval(Real mean, Real std_error, Real z_score = 1.96) {
        Real margin = z_score * std_error;
        return {mean - margin, mean + margin};
    }
    
    // Variance Reduction Factor
    static Real variance_reduction_factor(Real var_standard, Real var_reduced) {
        if (var_reduced <= 0.0) {
            throw std::invalid_argument("Reduced variance must be positive");
        }
        return var_standard / var_reduced;
    }
    
    // Efficiency gain
    static Real efficiency_gain(Real var_standard, Real time_standard,
                               Real var_reduced, Real time_reduced) {
        Real eff_standard = 1.0 / (var_standard * time_standard);
        Real eff_reduced = 1.0 / (var_reduced * time_reduced);
        return eff_reduced / eff_standard;
    }
    
    // Mean Squared Error
    static Real mse(const std::vector<Real>& estimates, Real true_value) {
        if (estimates.empty()) {
            throw std::invalid_argument("Cannot compute MSE of empty data");
        }
        
        Real sum_squared_error = 0.0;
        for (Real estimate : estimates) {
            Real error = estimate - true_value;
            sum_squared_error += error * error;
        }
        
        return sum_squared_error / static_cast<Real>(estimates.size());
    }
    
    // Relative error
    static Real relative_error(Real estimate, Real true_value) {
        if (std::abs(true_value) < 1e-10) {
            throw std::invalid_argument("True value too close to zero");
        }
        return std::abs(estimate - true_value) / std::abs(true_value);
    }
    
    // Percentile
    static Real percentile(std::vector<Real> data, Real p) {
        if (data.empty()) {
            throw std::invalid_argument("Cannot compute percentile of empty data");
        }
        if (p < 0.0 || p > 100.0) {
            throw std::invalid_argument("Percentile must be between 0 and 100");
        }
        
        std::sort(data.begin(), data.end());
        
        Real index = (p / 100.0) * (data.size() - 1);
        Size lower = static_cast<Size>(std::floor(index));
        Size upper = static_cast<Size>(std::ceil(index));
        
        if (lower == upper) {
            return data[lower];
        }
        
        Real weight = index - lower;
        return data[lower] * (1.0 - weight) + data[upper] * weight;
    }
};

} // namespace mc_engine

#endif // MC_ENGINE_CORE_STATISTICS_HPP