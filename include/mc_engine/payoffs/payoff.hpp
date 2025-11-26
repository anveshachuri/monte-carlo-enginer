// include/mc_engine/payoffs/payoff.hpp
// Abstract payoff interface

#ifndef MC_ENGINE_PAYOFFS_PAYOFF_HPP
#define MC_ENGINE_PAYOFFS_PAYOFF_HPP

#include "mc_engine/core/types.hpp"
#include <vector>
#include <memory>

namespace mc_engine {

class Payoff {
public:
    virtual ~Payoff() = default;
    
    // Compute payoff from a complete path
    virtual Real operator()(const std::vector<Real>& path) const = 0;
    
    // Compute payoff from terminal value only (for path-independent options)
    virtual Real operator()(Real terminal_value) const {
        return (*this)(std::vector<Real>{terminal_value});
    }
    
    // check path-dependent option?
    virtual bool is_path_dependent() const = 0;
    
    // Get strike
    virtual Real get_strike() const = 0;
    
    // Get option type
    virtual OptionType get_type() const = 0;
};

}

#endif