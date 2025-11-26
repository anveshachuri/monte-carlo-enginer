// include/mc_engine/payoffs/european.hpp
// European option payoffs

#ifndef MC_ENGINE_PAYOFFS_EUROPEAN_HPP
#define MC_ENGINE_PAYOFFS_EUROPEAN_HPP

#include "payoff.hpp"
#include <algorithm>

namespace mc_engine {

class EuropeanPayoff : public Payoff {
private:
    Real strike_;
    OptionType type_;
    
public:
    EuropeanPayoff(Real strike, OptionType type = OptionType::Call)
        : strike_(strike), type_(type) {
        if (strike <= 0.0) {
            throw std::invalid_argument("Strike must be positive");
        }
    }
    
    Real operator()(const std::vector<Real>& path) const override {
        return (*this)(path.back());
    }
    
    Real operator()(Real terminal_value) const override {
        if (type_ == OptionType::Call) {
            return std::max(terminal_value - strike_, 0.0);
        } else {
            return std::max(strike_ - terminal_value, 0.0);
        }
    }
    
    bool is_path_dependent() const override {
        return false;
    }
    
    Real get_strike() const override {
        return strike_;
    }
    
    OptionType get_type() const override {
        return type_;
    }
};

} 

#endif 