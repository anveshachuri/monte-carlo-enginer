#!/usr/bin/env python3
"""
plot_results.py
Comprehensive visualization script for Monte Carlo convergence and analysis
Place in: monte_carlo_engine/scripts/plot_results.py
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os
import sys

# Use a nice style
plt.style.use('seaborn-v0_8-darkgrid')

def plot_convergence_paths():
    """Plot price convergence vs number of paths"""
    if not os.path.exists('convergence_paths.csv'):
        print("Warning: convergence_paths.csv not found. Run convergence_study first.")
        return
    
    df = pd.read_csv('convergence_paths.csv')
    
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('Convergence vs Number of Paths', fontsize=16, fontweight='bold')
    
    # Plot 1: Price with confidence intervals
    ax = axes[0, 0]
    ax.plot(df['num_paths'], df['mc_price'], 'b-o', label='MC Price', linewidth=2)
    ax.fill_between(df['num_paths'], df['ci_lower'], df['ci_upper'], 
                     alpha=0.3, label='95% CI')
    ax.set_xlabel('Number of Paths', fontsize=12)
    ax.set_ylabel('Option Price', fontsize=12)
    ax.set_xscale('log')
    ax.legend()
    ax.grid(True, alpha=0.3)
    
    # Plot 2: Standard error vs sqrt(N)
    ax = axes[0, 1]
    ax.loglog(df['num_paths'], df['std_error'], 'g-o', linewidth=2, label='Std Error')
    # Theoretical O(1/sqrt(N)) line
    x = df['num_paths'].values
    theoretical = df['std_error'].iloc[0] * np.sqrt(x[0] / x)
    ax.loglog(x, theoretical, 'r--', linewidth=2, label='O(1/√N)')
    ax.set_xlabel('Number of Paths', fontsize=12)
    ax.set_ylabel('Standard Error', fontsize=12)
    ax.legend()
    ax.grid(True, alpha=0.3)
    
    # Plot 3: Relative error
    ax = axes[1, 0]
    ax.semilogx(df['num_paths'], df['rel_error'], 'r-o', linewidth=2)
    ax.set_xlabel('Number of Paths', fontsize=12)
    ax.set_ylabel('Relative Error (%)', fontsize=12)
    ax.grid(True, alpha=0.3)
    
    # Plot 4: Computation time
    ax = axes[1, 1]
    ax.loglog(df['num_paths'], df['time'], 'm-o', linewidth=2, label='Actual')
    # Linear scaling reference
    linear = df['time'].iloc[0] * (x / x[0])
    ax.loglog(x, linear, 'k--', linewidth=2, label='O(N)')
    ax.set_xlabel('Number of Paths', fontsize=12)
    ax.set_ylabel('Time (seconds)', fontsize=12)
    ax.legend()
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig('convergence_paths.png', dpi=300, bbox_inches='tight')
    print("✓ Saved: convergence_paths.png")
    plt.close()

def plot_convergence_timesteps():
    """Plot price convergence vs time discretization"""
    if not os.path.exists('convergence_timesteps.csv'):
        print("Warning: convergence_timesteps.csv not found. Run convergence_study first.")
        return
    
    df = pd.read_csv('convergence_timesteps.csv')
    
    fig, axes = plt.subplots(1, 2, figsize=(14, 5))
    fig.suptitle('Convergence vs Time Discretization (Asian Option)', 
                 fontsize=16, fontweight='bold')
    
    # Plot 1: Price vs dt
    ax = axes[0]
    ax.plot(df['dt'], df['price'], 'b-o', linewidth=2, markersize=8)
    ax.errorbar(df['dt'], df['price'], yerr=1.96*df['std_error'], 
                fmt='none', ecolor='gray', alpha=0.5, capsize=5)
    ax.set_xlabel('Time Step (dt)', fontsize=12)
    ax.set_ylabel('Asian Option Price', fontsize=12)
    ax.set_xscale('log')
    ax.grid(True, alpha=0.3)
    ax.invert_xaxis()  # Smaller dt is better
    
    # Plot 2: Price vs number of steps
    ax = axes[1]
    ax.plot(df['num_steps'], df['price'], 'g-o', linewidth=2, markersize=8)
    ax.errorbar(df['num_steps'], df['price'], yerr=1.96*df['std_error'],
                fmt='none', ecolor='gray', alpha=0.5, capsize=5)
    ax.set_xlabel('Number of Time Steps', fontsize=12)
    ax.set_ylabel('Asian Option Price', fontsize=12)
    ax.set_xscale('log')
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig('convergence_timesteps.png', dpi=300, bbox_inches='tight')
    print("✓ Saved: convergence_timesteps.png")
    plt.close()

def plot_variance_reduction():
    """Plot variance reduction effectiveness"""
    if not os.path.exists('variance_reduction.csv'):
        print("Warning: variance_reduction.csv not found. Run convergence_study first.")
        return
    
    df = pd.read_csv('variance_reduction.csv')
    
    fig, axes = plt.subplots(1, 3, figsize=(18, 5))
    fig.suptitle('Variance Reduction: Antithetic Variates', 
                 fontsize=16, fontweight='bold')
    
    # Plot 1: Variance comparison
    ax = axes[0]
    x = np.arange(len(df))
    width = 0.35
    ax.bar(x - width/2, df['standard_var'], width, label='Standard MC', alpha=0.8)
    ax.bar(x + width/2, df['anti_var'], width, label='Antithetic', alpha=0.8)
    ax.set_xlabel('Number of Paths', fontsize=12)
    ax.set_ylabel('Variance', fontsize=12)
    ax.set_xticks(x)
    ax.set_xticklabels(df['num_paths'], rotation=45)
    ax.legend()
    ax.grid(True, alpha=0.3, axis='y')
    
    # Plot 2: Variance Reduction Factor
    ax = axes[1]
    ax.plot(df['num_paths'], df['vrf'], 'r-o', linewidth=2, markersize=10)
    ax.axhline(y=1.0, color='k', linestyle='--', alpha=0.5)
    ax.set_xlabel('Number of Paths', fontsize=12)
    ax.set_ylabel('Variance Reduction Factor', fontsize=12)
    ax.set_xscale('log')
    ax.grid(True, alpha=0.3)
    ax.set_ylim(bottom=0.9)
    
    # Plot 3: Efficiency gain
    ax = axes[2]
    ax.plot(df['num_paths'], df['efficiency'], 'g-o', linewidth=2, markersize=10)
    ax.axhline(y=1.0, color='k', linestyle='--', alpha=0.5, label='Break-even')
    ax.set_xlabel('Number of Paths', fontsize=12)
    ax.set_ylabel('Efficiency Gain', fontsize=12)
    ax.set_xscale('log')
    ax.legend()
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig('variance_reduction.png', dpi=300, bbox_inches='tight')
    print("✓ Saved: variance_reduction.png")
    plt.close()

def plot_greeks_comparison():
    """Plot Greeks comparison across methods with error bars"""
    if not os.path.exists('greeks_comparison.csv'):
        print("Warning: greeks_comparison.csv not found. Run Greeks analysis first.")
        return
    
    df = pd.read_csv('greeks_comparison.csv')
    
    fig, axes = plt.subplots(1, 2, figsize=(14, 6))
    fig.suptitle('Greek Estimation: Method Comparison', fontsize=16, fontweight='bold')
    
    # Plot 1: Delta estimates with error bars
    ax = axes[0]
    methods = df['method'].unique()
    x_pos = np.arange(len(methods))
    
    deltas = [df[df['method'] == m]['delta'].iloc[0] for m in methods]
    errors = [df[df['method'] == m]['std_error'].iloc[0] for m in methods]
    
    bars = ax.bar(x_pos, deltas, yerr=[1.96*e for e in errors], 
                  capsize=10, alpha=0.7, edgecolor='black', linewidth=1.5)
    
    # Color bars differently
    colors = ['#2E86AB', '#A23B72', '#F18F01']
    for bar, color in zip(bars, colors):
        bar.set_color(color)
    
    if 'true_delta' in df.columns and not pd.isna(df['true_delta'].iloc[0]):
        true_delta = df['true_delta'].iloc[0]
        ax.axhline(y=true_delta, color='red', linestyle='--', 
                   linewidth=2, label='Black-Scholes', zorder=0)
        ax.legend()
    
    ax.set_ylabel('Delta', fontsize=12)
    ax.set_xlabel('Method', fontsize=12)
    ax.set_xticks(x_pos)
    ax.set_xticklabels(methods, rotation=0)
    ax.grid(True, alpha=0.3, axis='y')
    ax.set_title('Delta Estimates (±1.96σ)', fontsize=12)
    
    # Plot 2: Variance comparison (log scale)
    ax = axes[1]
    variances = [df[df['method'] == m]['variance'].iloc[0] for m in methods]
    bars = ax.bar(x_pos, variances, alpha=0.7, edgecolor='black', linewidth=1.5)
    
    for bar, color in zip(bars, colors):
        bar.set_color(color)
    
    ax.set_ylabel('Variance (log scale)', fontsize=12)
    ax.set_xlabel('Method', fontsize=12)
    ax.set_xticks(x_pos)
    ax.set_xticklabels(methods, rotation=0)
    ax.set_yscale('log')
    ax.grid(True, alpha=0.3, axis='y')
    ax.set_title('Variance Comparison', fontsize=12)
    
    # Add variance ratios as text
    baseline_var = variances[0]
    for i, (m, v) in enumerate(zip(methods, variances)):
        ratio = v / baseline_var
        ax.text(i, v * 1.2, f'{ratio:.2f}×', ha='center', fontsize=10, fontweight='bold')
    
    plt.tight_layout()
    plt.savefig('greeks_comparison.png', dpi=300, bbox_inches='tight')
    print("✓ Saved: greeks_comparison.png")
    plt.close()

def plot_lr_variance_explosion():
    """Plot LR variance vs strike (OTM explosion)"""
    if not os.path.exists('greeks_variance_vs_strike.csv'):
        print("Warning: greeks_variance_vs_strike.csv not found.")
        return
    
    df = pd.read_csv('greeks_variance_vs_strike.csv')
    
    fig, axes = plt.subplots(1, 2, figsize=(14, 6))
    fig.suptitle('Likelihood Ratio Variance: OTM Explosion', 
                 fontsize=16, fontweight='bold')
    
    # Plot 1: Variance vs Strike
    ax = axes[0]
    for method in df['method'].unique():
        method_data = df[df['method'] == method]
        ax.plot(method_data['strike'], method_data['variance'], 
                'o-', linewidth=2, markersize=8, label=method)
    
    ax.axvline(x=100, color='gray', linestyle='--', alpha=0.5, label='ATM (S=100)')
    ax.set_xlabel('Strike', fontsize=12)
    ax.set_ylabel('Variance', fontsize=12)
    ax.set_yscale('log')
    ax.legend()
    ax.grid(True, alpha=0.3)
    ax.set_title('Variance vs Moneyness', fontsize=12)
    
    # Plot 2: Variance ratio (LR/Pathwise)
    ax = axes[1]
    pw_data = df[df['method'] == 'Pathwise']
    lr_data = df[df['method'] == 'LikelihoodRatio']
    
    if len(pw_data) > 0 and len(lr_data) > 0:
        ratio = lr_data['variance'].values / pw_data['variance'].values
        ax.plot(pw_data['strike'], ratio, 'ro-', linewidth=2, markersize=8)
        ax.axvline(x=100, color='gray', linestyle='--', alpha=0.5)
        ax.axhline(y=1.0, color='k', linestyle='--', alpha=0.5)
        ax.set_xlabel('Strike', fontsize=12)
        ax.set_ylabel('Variance Ratio (LR / Pathwise)', fontsize=12)
        ax.set_yscale('log')
        ax.grid(True, alpha=0.3)
        ax.set_title('LR Disadvantage for OTM Options', fontsize=12)
    
    plt.tight_layout()
    plt.savefig('lr_variance_explosion.png', dpi=300, bbox_inches='tight')
    print("✓ Saved: lr_variance_explosion.png")
    plt.close()

def plot_asian_vs_european():
    """Plot Asian vs European price comparison"""
    if not os.path.exists('asian_european_comparison.csv'):
        print("Warning: asian_european_comparison.csv not found.")
        return
    
    df = pd.read_csv('asian_european_comparison.csv')
    
    fig, axes = plt.subplots(1, 2, figsize=(14, 6))
    fig.suptitle('Asian vs European Options: Monte Carlo Pricing', 
                 fontsize=16, fontweight='bold')
    
    # Plot 1: Price convergence
    ax = axes[0]
    for option_type in df['option_type'].unique():
        data = df[df['option_type'] == option_type]
        ax.plot(data['num_paths'], data['price'], 'o-', 
                linewidth=2, markersize=8, label=option_type)
        ax.fill_between(data['num_paths'], 
                        data['price'] - 1.96*data['std_error'],
                        data['price'] + 1.96*data['std_error'],
                        alpha=0.2)
    
    ax.set_xlabel('Number of Paths', fontsize=12)
    ax.set_ylabel('Option Price', fontsize=12)
    ax.set_xscale('log')
    ax.legend()
    ax.grid(True, alpha=0.3)
    ax.set_title('Price Convergence (95% CI)', fontsize=12)
    
    # Plot 2: Boxplot comparison at final N
    ax = axes[1]
    if 'samples' in df.columns:
        # If we have sample data, create boxplots
        european_samples = df[df['option_type'] == 'European']['samples'].iloc[0]
        asian_samples = df[df['option_type'] == 'Asian']['samples'].iloc[0]
        
        if isinstance(european_samples, str):
            european_samples = [float(x) for x in european_samples.split(',')]
            asian_samples = [float(x) for x in asian_samples.split(',')]
        
        ax.boxplot([european_samples, asian_samples], 
                   labels=['European', 'Asian'],
                   patch_artist=True)
    else:
        # Simple bar plot with error bars
        final_data = df.groupby('option_type').last()
        x_pos = np.arange(len(final_data))
        ax.bar(x_pos, final_data['price'], 
               yerr=1.96*final_data['std_error'],
               capsize=10, alpha=0.7, edgecolor='black', linewidth=1.5)
        ax.set_xticks(x_pos)
        ax.set_xticklabels(final_data.index)
    
    ax.set_ylabel('Option Price', fontsize=12)
    ax.grid(True, alpha=0.3, axis='y')
    ax.set_title('Final Price Distribution', fontsize=12)
    
    plt.tight_layout()
    plt.savefig('asian_european_comparison.png', dpi=300, bbox_inches='tight')
    print("✓ Saved: asian_european_comparison.png")
    plt.close()

def plot_performance_scaling():
    """Plot performance scaling with paths and threads"""
    if not os.path.exists('performance_results.csv'):
        print("Warning: performance_results.csv not found. Run performance_benchmark first.")
        return
    
    df = pd.read_csv('performance_results.csv')
    
    fig, axes = plt.subplots(1, 3, figsize=(18, 5))
    fig.suptitle('Performance Scaling Analysis', fontsize=16, fontweight='bold')
    
    # Plot 1: Speedup vs threads
    ax = axes[0]
    threaded_data = df[df['threads'] > 1] if 'threads' in df.columns else df
    
    if len(threaded_data) > 0 and 'threads' in df.columns:
        ax.plot(threaded_data['threads'], threaded_data['speedup'], 
                'bo-', linewidth=2, markersize=10, label='Actual')
        # Ideal linear speedup
        max_threads = threaded_data['threads'].max()
        ideal_x = np.linspace(1, max_threads, 100)
        ax.plot(ideal_x, ideal_x, 'r--', linewidth=2, label='Ideal (linear)')
        
        ax.set_xlabel('Number of Threads', fontsize=12)
        ax.set_ylabel('Speedup', fontsize=12)
        ax.legend()
        ax.grid(True, alpha=0.3)
        ax.set_title('Strong Scaling', fontsize=12)
    
    # Plot 2: Efficiency vs threads
    ax = axes[1]
    if len(threaded_data) > 0 and 'efficiency' in df.columns:
        ax.plot(threaded_data['threads'], threaded_data['efficiency'], 
                'go-', linewidth=2, markersize=10)
        ax.axhline(y=1.0, color='r', linestyle='--', linewidth=2, label='Perfect efficiency')
        ax.axhline(y=0.8, color='orange', linestyle=':', linewidth=2, label='80% threshold')
        
        ax.set_xlabel('Number of Threads', fontsize=12)
        ax.set_ylabel('Efficiency (Speedup/Threads)', fontsize=12)
        ax.legend()
        ax.grid(True, alpha=0.3)
        ax.set_ylim([0, 1.1])
        ax.set_title('Parallel Efficiency', fontsize=12)
    
    # Plot 3: Throughput (paths/sec)
    ax = axes[2]
    if 'paths_per_sec' in df.columns:
        methods = df['method'].unique() if 'method' in df.columns else ['Single']
        x_pos = np.arange(len(methods))
        
        throughputs = [df[df['method'] == m]['paths_per_sec'].iloc[0] if 'method' in df.columns 
                      else df['paths_per_sec'].iloc[0] for m in methods]
        
        bars = ax.bar(x_pos, throughputs, alpha=0.7, edgecolor='black', linewidth=1.5)
        
        # Color bars
        colors = ['#2E86AB', '#A23B72', '#F18F01', '#06A77D']
        for bar, color in zip(bars, colors[:len(bars)]):
            bar.set_color(color)
        
        ax.set_ylabel('Throughput (paths/sec)', fontsize=12)
        ax.set_xlabel('Method', fontsize=12)
        ax.set_xticks(x_pos)
        ax.set_xticklabels(methods, rotation=45, ha='right')
        ax.set_yscale('log')
        ax.grid(True, alpha=0.3, axis='y')
        ax.set_title('Throughput Comparison', fontsize=12)
        
        # Add speedup factors
        baseline = throughputs[0]
        for i, t in enumerate(throughputs):
            speedup = t / baseline
            ax.text(i, t * 1.2, f'{speedup:.1f}×', ha='center', 
                   fontsize=10, fontweight='bold')
    
    plt.tight_layout()
    plt.savefig('performance_scaling.png', dpi=300, bbox_inches='tight')
    print("✓ Saved: performance_scaling.png")
    plt.close()

def generate_summary_report():
    """Generate text summary of results"""
    print("\n" + "="*70)
    print("CONVERGENCE STUDY SUMMARY")
    print("="*70)
    
    # Paths convergence
    if os.path.exists('convergence_paths.csv'):
        df_paths = pd.read_csv('convergence_paths.csv')
        print("\n1. CONVERGENCE VS NUMBER OF PATHS:")
        print(f"   - Final price (max paths): {df_paths['mc_price'].iloc[-1]:.6f}")
        print(f"   - Final std error: {df_paths['std_error'].iloc[-1]:.6f}")
        print(f"   - Error reduction: {df_paths['std_error'].iloc[0] / df_paths['std_error'].iloc[-1]:.2f}×")
    
    # Timestep convergence
    if os.path.exists('convergence_timesteps.csv'):
        df_steps = pd.read_csv('convergence_timesteps.csv')
        print("\n2. CONVERGENCE VS TIME STEPS:")
        print(f"   - Price (min steps): {df_steps['price'].iloc[0]:.6f}")
        print(f"   - Price (max steps): {df_steps['price'].iloc[-1]:.6f}")
        print(f"   - Difference: {abs(df_steps['price'].iloc[-1] - df_steps['price'].iloc[0]):.6f}")
    
    # Variance reduction
    if os.path.exists('variance_reduction.csv'):
        df_var = pd.read_csv('variance_reduction.csv')
        print("\n3. VARIANCE REDUCTION (Antithetic Variates):")
        print(f"   - Average VRF: {df_var['vrf'].mean():.3f}×")
        print(f"   - Best VRF: {df_var['vrf'].max():.3f}×")
        print(f"   - Average efficiency: {df_var['efficiency'].mean():.3f}×")
    
    # Greeks
    if os.path.exists('greeks_comparison.csv'):
        df_greeks = pd.read_csv('greeks_comparison.csv')
        print("\n4. GREEK ESTIMATION:")
        for method in df_greeks['method'].unique():
            data = df_greeks[df_greeks['method'] == method].iloc[0]
            print(f"   - {method}: Δ={data['delta']:.6f}, σ={data['std_error']:.6f}")
    
    # Performance
    if os.path.exists('performance_results.csv'):
        df_perf = pd.read_csv('performance_results.csv')
        print("\n5. PERFORMANCE:")
        if 'speedup' in df_perf.columns:
            max_speedup = df_perf['speedup'].max()
            print(f"   - Maximum speedup: {max_speedup:.2f}×")
        if 'paths_per_sec' in df_perf.columns:
            max_throughput = df_perf['paths_per_sec'].max()
            print(f"   - Peak throughput: {max_throughput:.0f} paths/sec")
    
    print("\n" + "="*70)

if __name__ == "__main__":
    print("="*70)
    print("Monte Carlo Analysis - Comprehensive Visualization")
    print("="*70)
    print("\nGenerating plots from CSV data...\n")
    
    plots_generated = 0
    
    try:
        # Original plots
        plot_convergence_paths()
        plots_generated += 1
        
        plot_convergence_timesteps()
        plots_generated += 1
        
        plot_variance_reduction()
        plots_generated += 1
        
        # New advanced plots
        plot_greeks_comparison()
        plots_generated += 1
        
        plot_lr_variance_explosion()
        plots_generated += 1
        
        plot_asian_vs_european()
        plots_generated += 1
        
        plot_performance_scaling()
        plots_generated += 1
        
        # Summary report
        generate_summary_report()
        
        print(f"\n✓ Successfully generated {plots_generated} plots!")
        print("\nGenerated files:")
        print("  - convergence_paths.png")
        print("  - convergence_timesteps.png")
        print("  - variance_reduction.png")
        print("  - greeks_comparison.png")
        print("  - lr_variance_explosion.png")
        print("  - asian_european_comparison.png")
        print("  - performance_scaling.png")
        
    except Exception as e:
        print(f"\n✗ Error generating plots: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)