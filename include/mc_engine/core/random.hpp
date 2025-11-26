// Random number generation with Box-Muller transform

#ifndef MC_ENGINE_CORE_RANDOM_HPP
#define MC_ENGINE_CORE_RANDOM_HPP

#include "types.hpp"
#include <random>
#include <memory>

namespace mc_engine {

// Abstract RNG Interface

class RandomGenerator {
public:
    virtual ~RandomGenerator() = default;
    
    virtual Real next_gaussian() = 0;
    virtual Real next_uniform() = 0;
    virtual void fill_gaussian(std::vector<Real>& out) = 0;
    virtual void reset(Size seed) = 0;
};

// Box-Muller Transform Implementation

class BoxMullerGenerator : public RandomGenerator {
private:
    std::mt19937_64 engine_;
    std::uniform_real_distribution<Real> uniform_dist_;
    bool has_spare_;
    Real spare_;
    
public:
    explicit BoxMullerGenerator(Size seed = 42): engine_(seed),uniform_dist_(0.0, 1.0),has_spare_(false),spare_(0.0) {}
    
    Real next_gaussian() override {
        if (has_spare_) {
            has_spare_ = false;
            return spare_;
        }
        
        Real u1 = next_uniform();
        Real u2 = next_uniform();
        
        Real radius = std::sqrt(-2.0 * std::log(u1));
        Real theta = 2.0 * constants::PI * u2;
        
        spare_ = radius * std::sin(theta);
        has_spare_ = true;
        
        return radius * std::cos(theta);
    }
    
    Real next_uniform() override {
        return uniform_dist_(engine_);
    }
    
    void fill_gaussian(std::vector<Real>& out) override {
        for (auto& val : out) {
            val = next_gaussian();
        }
    }
    
    void reset(Size seed) override {
        engine_.seed(seed);
        has_spare_ = false;
        spare_ = 0.0;
    }
};

// Factory function

inline std::unique_ptr<RandomGenerator> make_rng(Size seed = 42) {
    return std::make_unique<BoxMullerGenerator>(seed);
}

}

#endif