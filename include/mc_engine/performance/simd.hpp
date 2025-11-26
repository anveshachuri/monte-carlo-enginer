// include/mc_engine/performance/simd.hpp
// SIMD vectorization using AVX2 intrinsics

#ifndef MC_ENGINE_PERFORMANCE_SIMD_HPP
#define MC_ENGINE_PERFORMANCE_SIMD_HPP

#include "mc_engine/core/types.hpp"
#include "mc_engine/core/random.hpp"
#include "mc_engine/models/gbm.hpp"
#include <vector>
#include <cmath>

// Check for AVX2 support
#if defined(__AVX2__)
#include <immintrin.h>
#define HAS_AVX2 1
#else
#define HAS_AVX2 0
#endif

namespace mc_engine {

#if HAS_AVX2
constexpr Size SIMD_WIDTH = 4;  // 4 doubles per AVX2 vector
#else
constexpr Size SIMD_WIDTH = 1;  // Scalar fallback
#endif

// SIMD-Optimized Path Generator
class SIMDPathGenerator {
private:
    MarketParams params_;
    SimulationConfig config_;
    Real dt_;
    Real drift_;
    Real diffusion_;
    
public:
    SIMDPathGenerator(const MarketParams& params, const SimulationConfig& config)
        : params_(params), config_(config) {
        
        dt_ = config_.dt(params_.maturity);
        drift_ = (params_.rate - params_.dividend - 
                 0.5 * params_.volatility * params_.volatility) * dt_;
        diffusion_ = params_.volatility * std::sqrt(dt_);
    }
    
    void generate_paths_simd(PathData& paths, RandomGenerator& rng) const {
#if HAS_AVX2
        generate_paths_avx2(paths, rng);
#else
        generate_paths_scalar(paths, rng);
#endif
    }
    
private:
    
#if HAS_AVX2
    void generate_paths_avx2(PathData& paths, RandomGenerator& rng) const {
        Size num_paths = paths.num_paths;
        Size num_steps = paths.num_steps;
        
        // Initialize with spot
        __m256d spot_vec = _mm256_set1_pd(params_.spot);
        for (Size i = 0; i < num_paths; i += SIMD_WIDTH) {
            _mm256_storeu_pd(&paths(i, 0), spot_vec);
        }
        
        __m256d drift_vec = _mm256_set1_pd(drift_);
        __m256d diffusion_vec = _mm256_set1_pd(diffusion_);
        
        for (Size i = 0; i < num_paths; i += SIMD_WIDTH) {
            __m256d current = _mm256_loadu_pd(&paths(i, 0));
            
            for (Size j = 1; j <= num_steps; ++j) {
                // Generate random normals
                alignas(32) Real randoms[SIMD_WIDTH];
                for (Size k = 0; k < SIMD_WIDTH; ++k) {
                    randoms[k] = rng.next_gaussian();
                }
                __m256d z = _mm256_load_pd(randoms);
                
                // Compute: drift + diffusion * z
                __m256d exponent = _mm256_fmadd_pd(diffusion_vec, z, drift_vec);
                
                // exp() - use scalar (vectorized exp requires SVML)
                alignas(32) Real exp_vals[SIMD_WIDTH];
                _mm256_store_pd(exp_vals, exponent);
                for (Size k = 0; k < SIMD_WIDTH; ++k) {
                    exp_vals[k] = std::exp(exp_vals[k]);
                }
                __m256d exp_vec = _mm256_load_pd(exp_vals);
                
                // Update: current *= exp
                current = _mm256_mul_pd(current, exp_vec);
                
                // Store
                _mm256_storeu_pd(&paths(i, j), current);
            }
        }
    }
#endif
    
    void generate_paths_scalar(PathData& paths, RandomGenerator& rng) const {
        for (Size i = 0; i < paths.num_paths; ++i) {
            paths(i, 0) = params_.spot;
            
            for (Size j = 1; j <= paths.num_steps; ++j) {
                Real z = rng.next_gaussian();
                paths(i, j) = paths(i, j-1) * std::exp(drift_ + diffusion_ * z);
            }
        }
    }
};

// SIMD-Optimized Payoff Computer
class SIMDPayoffComputer {
public:
    static void compute_asian_payoffs(const PathData& paths,
                                     std::vector<Real>& payoffs,
                                     Real strike) {
#if HAS_AVX2
        compute_asian_payoffs_avx2(paths, payoffs, strike);
#else
        compute_asian_payoffs_scalar(paths, payoffs, strike);
#endif
    }
    
private:
    
#if HAS_AVX2
    static void compute_asian_payoffs_avx2(const PathData& paths,
                                          std::vector<Real>& payoffs,
                                          Real strike) {
        Size num_paths = paths.num_paths;
        Size num_steps = paths.num_steps;
        Real inv_n = 1.0 / (num_steps + 1);
        
        __m256d inv_n_vec = _mm256_set1_pd(inv_n);
        __m256d strike_vec = _mm256_set1_pd(strike);
        __m256d zero_vec = _mm256_setzero_pd();
        
        for (Size i = 0; i < num_paths; i += SIMD_WIDTH) {
            __m256d sum = _mm256_setzero_pd();
            
            for (Size j = 0; j <= num_steps; ++j) {
                __m256d spot = _mm256_loadu_pd(&paths(i, j));
                sum = _mm256_add_pd(sum, spot);
            }
            
            __m256d average = _mm256_mul_pd(sum, inv_n_vec);
            __m256d diff = _mm256_sub_pd(average, strike_vec);
            __m256d payoff = _mm256_max_pd(diff, zero_vec);
            
            _mm256_storeu_pd(&payoffs[i], payoff);
        }
    }
#endif
    
    static void compute_asian_payoffs_scalar(const PathData& paths,
                                            std::vector<Real>& payoffs,
                                            Real strike) {
        Size num_paths = paths.num_paths;
        Size num_steps = paths.num_steps;
        
        for (Size i = 0; i < num_paths; ++i) {
            Real sum = 0.0;
            for (Size j = 0; j <= num_steps; ++j) {
                sum += paths(i, j);
            }
            Real average = sum / (num_steps + 1);
            payoffs[i] = std::max(average - strike, 0.0);
        }
    }
};

} 
#endif