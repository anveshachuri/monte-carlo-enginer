// include/mc_engine/performance/threading.hpp
// Multi-threading infrastructure with thread pool

#ifndef MC_ENGINE_PERFORMANCE_THREADING_HPP
#define MC_ENGINE_PERFORMANCE_THREADING_HPP

#include "mc_engine/core/types.hpp"
#include "mc_engine/models/gbm.hpp"
#include <thread>
#include <vector>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <chrono>

namespace mc_engine {

// Thread Pool
class ThreadPool {
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;
    
public:
    explicit ThreadPool(Size num_threads) : stop_(false) {
        for (Size i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        condition_.wait(lock, [this] {
                            return stop_.load() || !tasks_.empty();
                        });
                        
                        if (stop_.load() && tasks_.empty()) {
                            return;
                        }
                        
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    
                    task();
                }
            });
        }
    }
    
    ~ThreadPool() {
        stop_.store(true);
        condition_.notify_all();
        
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
    
    template<typename F>
    auto enqueue(F&& f) -> std::future<typename std::invoke_result<F>::type> {
        using return_type = typename std::invoke_result<F>::type;
        
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::forward<F>(f)
        );
        
        std::future<return_type> result = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            
            if (stop_.load()) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            
            tasks_.emplace([task]() { (*task)(); });
        }
        
        condition_.notify_one();
        return result;
    }
    
    Size num_threads() const {
        return workers_.size();
    }
};

// Parallel Monte Carlo
template<typename PayoffFunc>
PricingResult parallel_monte_carlo(
    const GBMPathGenerator& path_gen,
    const PayoffFunc& payoff_func,
    Real discount_factor,
    Size num_threads = std::thread::hardware_concurrency())
{
    auto start_time = std::chrono::high_resolution_clock::now();
    
    const auto& config = path_gen.get_config();
    Size paths_per_thread = config.num_paths / num_threads;
    Size remainder = config.num_paths % num_threads;
    
    ThreadPool pool(num_threads);
    std::vector<std::future<std::pair<Real, Real>>> futures;
    
    auto worker = [&](Size thread_id, Size start_path, Size end_path) 
        -> std::pair<Real, Real> {
        
        auto rng = make_rng(config.seed + thread_id);
        
        std::vector<Real> path;
        Real sum = 0.0;
        Real sum_sq = 0.0;
        
        for (Size i = start_path; i < end_path; ++i) {
            path_gen.generate_path(path, *rng);
            Real payoff = payoff_func(path);
            sum += payoff;
            sum_sq += payoff * payoff;
        }
        
        return {sum, sum_sq};
    };
    
    Size current_path = 0;
    for (Size t = 0; t < num_threads; ++t) {
        Size extra = (t < remainder) ? 1 : 0;
        Size paths_this_thread = paths_per_thread + extra;
        
        futures.push_back(pool.enqueue(
            [&worker, t, current_path, paths_this_thread]() {
                return worker(t, current_path, current_path + paths_this_thread);
            }
        ));
        
        current_path += paths_this_thread;
    }
    
    Real total_sum = 0.0;
    Real total_sum_sq = 0.0;
    
    for (auto& future : futures) {
        auto [sum, sum_sq] = future.get();
        total_sum += sum;
        total_sum_sq += sum_sq;
    }
    
    Real mean = total_sum / config.num_paths;
    Real variance = (total_sum_sq / config.num_paths - mean * mean) * 
                    config.num_paths / (config.num_paths - 1);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<Real> elapsed = end_time - start_time;
    
    PricingResult result;
    result.price = discount_factor * mean;
    result.variance = variance;
    result.std_error = std::sqrt(variance / config.num_paths);
    result.num_paths = config.num_paths;
    result.compute_time = elapsed.count();
    
    return result;
}

}

#endif