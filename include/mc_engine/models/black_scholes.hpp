// Black-Scholes analytical formulas for validation

#ifndef MC_ENGINE_MODELS_BLACK_SCHOLES_HPP
#define MC_ENGINE_MODELS_BLACK_SCHOLES_HPP

#include "mc_engine/core/types.hpp"
#include <cmath>

namespace mc_engine {

class BlackScholes {
public:
    // Standard Normal CDF
    static Real cdf_normal(Real x) {
        return 0.5 * (1.0 + std::erf(x / constants::SQRT_2));
    }
    
    // Standard Normal PDF
    static Real pdf_normal(Real x) {
        return constants::INV_SQRT_2PI * std::exp(-0.5 * x * x);
    }
    
    // European Call/Put Price
    static Real european_price(const MarketParams& params, OptionType type) {
        Real S = params.spot;
        Real K = params.strike;
        Real r = params.rate;
        Real q = params.dividend;
        Real sigma = params.volatility;
        Real T = params.maturity;
        
        if (T <= 0.0) return 0.0;
        if (sigma <= 0.0) {
            Real forward = S * std::exp((r - q) * T);
            if (type == OptionType::Call) {
                return std::exp(-r * T) * std::max(forward - K, 0.0);
            } else {
                return std::exp(-r * T) * std::max(K - forward, 0.0);
            }
        }
        
        Real d1 = compute_d1(S, K, r, q, sigma, T);
        Real d2 = d1 - sigma * std::sqrt(T);
        
        if (type == OptionType::Call) {
            return S * std::exp(-q * T) * cdf_normal(d1) - 
                   K * std::exp(-r * T) * cdf_normal(d2);
        } else {
            return K * std::exp(-r * T) * cdf_normal(-d2) - 
                   S * std::exp(-q * T) * cdf_normal(-d1);
        }
    }
    
    // Delta
    static Real delta(const MarketParams& params, OptionType type) {
        Real S = params.spot;
        Real K = params.strike;
        Real r = params.rate;
        Real q = params.dividend;
        Real sigma = params.volatility;
        Real T = params.maturity;
        
        if (T <= 0.0 || sigma <= 0.0) return 0.0;
        
        Real d1 = compute_d1(S, K, r, q, sigma, T);
        
        if (type == OptionType::Call) {
            return std::exp(-q * T) * cdf_normal(d1);
        } else {
            return -std::exp(-q * T) * cdf_normal(-d1);
        }
    }
    
    // Gamma
    static Real gamma(const MarketParams& params) {
        Real S = params.spot;
        Real K = params.strike;
        Real r = params.rate;
        Real q = params.dividend;
        Real sigma = params.volatility;
        Real T = params.maturity;
        
        if (T <= 0.0 || sigma <= 0.0 || S <= 0.0) return 0.0;
        
        Real d1 = compute_d1(S, K, r, q, sigma, T);
        return std::exp(-q * T) * pdf_normal(d1) / (S * sigma * std::sqrt(T));
    }
    
    // Vega
    static Real vega(const MarketParams& params) {
        Real S = params.spot;
        Real K = params.strike;
        Real r = params.rate;
        Real q = params.dividend;
        Real sigma = params.volatility;
        Real T = params.maturity;
        
        if (T <= 0.0 || sigma <= 0.0) return 0.0;
        
        Real d1 = compute_d1(S, K, r, q, sigma, T);
        return S * std::exp(-q * T) * pdf_normal(d1) * std::sqrt(T);
    }
    
    // Theta
    static Real theta(const MarketParams& params, OptionType type) {
        Real S = params.spot;
        Real K = params.strike;
        Real r = params.rate;
        Real q = params.dividend;
        Real sigma = params.volatility;
        Real T = params.maturity;
        
        if (T <= 0.0 || sigma <= 0.0) return 0.0;
        
        Real d1 = compute_d1(S, K, r, q, sigma, T);
        Real d2 = d1 - sigma * std::sqrt(T);
        
        Real term1 = -S * std::exp(-q * T) * pdf_normal(d1) * sigma / (2.0 * std::sqrt(T));
        
        if (type == OptionType::Call) {
            Real term2 = q * S * std::exp(-q * T) * cdf_normal(d1);
            Real term3 = -r * K * std::exp(-r * T) * cdf_normal(d2);
            return term1 + term2 + term3;
        } else {
            Real term2 = -q * S * std::exp(-q * T) * cdf_normal(-d1);
            Real term3 = r * K * std::exp(-r * T) * cdf_normal(-d2);
            return term1 + term2 + term3;
        }
    }
    
    // Rho
    static Real rho(const MarketParams& params, OptionType type) {
        Real S = params.spot;
        Real K = params.strike;
        Real r = params.rate;
        Real q = params.dividend;
        Real sigma = params.volatility;
        Real T = params.maturity;
        
        if (T <= 0.0 || sigma <= 0.0) return 0.0;
        
        Real d1 = compute_d1(S, K, r, q, sigma, T);
        Real d2 = d1 - sigma * std::sqrt(T);
        
        if (type == OptionType::Call) {
            return K * T * std::exp(-r * T) * cdf_normal(d2);
        } else {
            return -K * T * std::exp(-r * T) * cdf_normal(-d2);
        }
    }
    
private:
    static Real compute_d1(Real S, Real K, Real r, Real q, Real sigma, Real T) {
        return (std::log(S / K) + (r - q + 0.5 * sigma * sigma) * T) / (sigma * std::sqrt(T));
    }
};

} 

#endif