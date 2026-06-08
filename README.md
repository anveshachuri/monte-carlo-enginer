# High-Performance C++20 Monte Carlo Engine for Equity Option Pricing

A high-performance C++20 Monte Carlo simulation engine for pricing equity derivatives under Geometric Brownian Motion (GBM). Features rigorous numerical analysis, variance reduction techniques, Greek estimation, Black-Scholes validation, and parallel computing optimizations.

---

## Project Highlights

* **Deep Numerical Focus**: Exact log-Euler discretization, convergence analysis, and bias-variance tradeoffs
* **Variance Reduction**: Antithetic variates and control variates with statistical validation
* **Greek Estimation**: Three methods (Pathwise, Likelihood Ratio, and Bump-and-Revalue)
* **Parallel Computing**: Thread-pool based Monte Carlo acceleration with strong scaling behavior
* **Modern C++20**: Concepts, ranges, RAII, and zero-cost abstractions
* **Validation Framework**: Comprehensive testing against analytical Black-Scholes solutions

---

## Key Results

| Feature                | Result                                                                |
| ---------------------- | --------------------------------------------------------------------- |
| **Validation**         | All 8 test scenarios within 95% confidence intervals of Black-Scholes |
| **Multi-threading**    | 10.63× speedup observed on a 24-thread system                         |
| **Scalability**        | 7.19× speedup at 16 threads                                           |
| **Variance Reduction** | Antithetic and control variates implemented                           |
| **Greek Estimation**   | Pathwise, Likelihood Ratio, and Bump-and-Revalue methods              |
| **SIMD Support**       | AVX2 vectorization implementation included                            |

---

## Architecture

```text
monte_carlo_engine/
├── include/mc_engine/
│   ├── core/           # Types, RNG (Box-Muller), statistics
│   ├── models/         # GBM, Black-Scholes formulas
│   ├── payoffs/        # European, Asian (arithmetic/geometric)
│   ├── pricing/        # MC engine, variance reduction, Greeks
│   └── performance/    # Threading, SIMD, benchmarking
├── examples/
├── scripts/
└── docs/
```

---

## Mathematical Foundation

### Geometric Brownian Motion

```math
dS_t = \mu S_t dt + \sigma S_t dW_t
```

Exact discretization:

```math
S(t+\Delta t)
=
S(t)\exp\left((\mu-\sigma^2/2)\Delta t+\sigma\sqrt{\Delta t}Z\right)
```

where:

```math
Z \sim N(0,1)
```

The implementation uses exact log-Euler discretization, eliminating weak discretization bias for GBM paths.

---

## Implemented Features

### Pricing Engines

* European Call/Put Options
* Arithmetic Asian Options
* Geometric Asian Options

### Variance Reduction

* Antithetic Variates
* Control Variates using analytically priced European options

### Greek Estimation

| Method           | Characteristics                     |
| ---------------- | ----------------------------------- |
| Pathwise         | Low variance for smooth payoffs     |
| Likelihood Ratio | Applicable to discontinuous payoffs |
| Bump-and-Revalue | Simple finite-difference benchmark  |

### Statistical Infrastructure

* Box-Muller Gaussian RNG with spare caching
* Welford's online variance estimation
* Kahan summation for numerical stability
* Confidence intervals and Z-score validation

---

## Validation

The engine is validated against analytical Black-Scholes prices across eight scenarios:

* ATM Calls
* OTM Calls
* ITM Calls
* ATM Puts
* High Volatility Calls
* Low Volatility Calls
* Long-Maturity Calls
* Dividend-Paying Calls

Example validation result:

```text
ATM Call (S=K=100)

Monte Carlo:   10.452711 ± 0.021884
Black-Scholes: 10.450584

Error:         0.002127
Z-score:       0.097

✓ Within 95% confidence interval
```

All tested scenarios fall within their expected confidence intervals.

---

## Performance Benchmark

Benchmark configuration:

```text
Paths:              1,000,000
Timesteps:          252
Compiler:           MSVC 2022 (Release)
Hardware Threads:   24
```

Observed performance:

| Method         | Time (s) | Speedup |
| -------------- | -------- | ------- |
| Baseline       | 15.16    | 1.00×   |
| Multi-threaded | 1.43     | 10.63×  |

Thread scalability study:

| Threads | Speedup |
| ------- | ------- |
| 1       | 0.97×   |
| 2       | 1.35×   |
| 4       | 3.18×   |
| 8       | 5.33×   |
| 16      | 7.19×   |

The implementation demonstrates strong scaling characteristics with increasing thread count.

---

## Technical Details

### Numerical Methods

* Exact log-Euler discretization
* Box-Muller Gaussian random number generation
* Welford variance estimation
* Kahan summation for path-dependent payoffs

### Performance Optimizations

* Thread pool parallelization
* Structure-of-Arrays memory layout
* AVX2 SIMD implementation
* Memory pre-allocation and alignment

### Modern C++20 Features

* Concepts
* Structured bindings
* Ranges
* RAII resource management
* Const-correct design

---

## Build

### Requirements

* C++20 compatible compiler
* CMake 3.20+
* GCC 10+, Clang 11+, or MSVC 2019+

### Build Instructions

```bash
mkdir build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

### Example Programs

```bash
basic_pricing
validation
convergence_study
control_variates_demo
greeks_comparison
performance_benchmark
asian_vs_european
```

---

## Visualization

```bash
./convergence_study
python3 scripts/plot_results.py
```

Generates:

* Convergence vs number of paths
* Error vs timestep size
* Variance reduction effectiveness

---

## Author

Anvesha Churi

---

## References

* Black-Scholes-Merton Option Pricing Model
* Paul Glasserman, *Monte Carlo Methods in Financial Engineering*
* Effective Modern C++
