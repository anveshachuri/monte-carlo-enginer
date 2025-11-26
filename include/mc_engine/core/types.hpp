// Core type definitions and data structures

#ifndef MC_ENGINE_CORE_TYPES_HPP
#define MC_ENGINE_CORE_TYPES_HPP

#include <cstddef>
#include <vector>
#include <cmath>
#include <stdexcept>

namespace mc_engine {

// Fundamental Types
using Real = double;
using Size = std::size_t;

enum class OptionType {
    Call,
    Put
};

// Mathematical constants
namespace constants {
    constexpr Real PI = 3.14159265358979323846;
    constexpr Real SQRT_2 = 1.41421356237309504880;
    constexpr Real INV_SQRT_2PI = 0.39894228040143267794;
}

// Market Parameters

struct MarketParams {
    Real spot;           // S₀: Initial spot price
    Real strike;         // K: Strike price
    Real rate;           // r: Risk-free rate
    Real volatility;     // σ: Volatility
    Real maturity;       // T: Time to maturity (years)
    Real dividend;       // q: Dividend yield (default = 0)
    
    MarketParams(Real S0, Real K, Real r, Real sigma, Real T, Real q = 0.0): spot(S0), strike(K), rate(r), volatility(sigma), maturity(T), dividend(q) {
        validate();
    }
    
    void validate() const {
        if (spot <= 0.0) throw std::invalid_argument("Spot must be positive");
        if (strike <= 0.0) throw std::invalid_argument("Strike must be positive");
        if (volatility < 0.0) throw std::invalid_argument("Volatility must be non-negative");
        if (maturity <= 0.0) throw std::invalid_argument("Maturity must be positive");
        if (dividend < 0.0) throw std::invalid_argument("Dividend yield must be non-negative");
    }
};

// Simulation Configuration

struct SimulationConfig {
    Size num_paths;      // N: Number of Monte Carlo paths
    Size num_steps;      // Number of time steps per path
    Size seed;           // Random seed
    bool antithetic;     // antithetic variates
    
    SimulationConfig(Size N = 100000, Size steps = 252, Size s = 42, bool anti = false): num_paths(N), num_steps(steps), seed(s), antithetic(anti) {
        validate();
    }
    
    void validate() const {
        if (num_paths == 0) throw std::invalid_argument("Number of paths must be positive");
        if (num_steps == 0) throw std::invalid_argument("Number of steps must be positive");
    }
    
    Real dt(Real maturity) const {
        return maturity / static_cast<Real>(num_steps);
    }
};

// Results Structures

struct PricingResult {
    Real price;
    Real std_error;
    Real variance;
    Size num_paths;
    Real compute_time;
    
    Real confidence_interval_lower() const {
        return price - 1.96 * std_error;
    }
    
    Real confidence_interval_upper() const {
        return price + 1.96 * std_error;
    }
};

struct GreeksResult {
    Real delta;
    Real delta_std_error;
};

// Path Storage (Structure of Arrays)

struct PathData {
    std::vector<Real> spots;
    Size num_paths;
    Size num_steps;
    
    PathData(Size n_paths, Size n_steps): spots(n_paths * (n_steps + 1)), num_paths(n_paths), num_steps(n_steps) {}
    
    Real& operator()(Size i, Size j) {
        return spots[i * (num_steps + 1) + j];
    }
    
    const Real& operator()(Size i, Size j) const {
        return spots[i * (num_steps + 1) + j];
    }
    
    std::vector<Real> get_path(Size i) const {
        std::vector<Real> path(num_steps + 1);
        for (Size j = 0; j <= num_steps; ++j) {
            path[j] = (*this)(i, j);
        }
        return path;
    }
};

}

#endif