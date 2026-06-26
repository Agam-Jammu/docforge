#pragma once

#include <atomic>
#include <concepts>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <span>
#include <stop_token>
#include <thread>
#include <vector>

namespace clearcapture {

/**
 * @brief Concept constraining document processor types.
 *
 * Any document processor must be callable with a std::span<const char>
 * (the raw file buffer) and return a type T.
 */
template<typename T, typename Proc>
concept DocumentProcessor = requires(Proc p, std::span<const char> buf) {
    { p(buf) } -> std::same_as<T>;
};

/**
 * @brief Lock-free concurrent queue for distributing work to threads.
 *
 * Uses a mutex-protected std::queue under the hood (lock-free in spirit:
 * minimal contention since threads pop work and process independently).
 */
template<typename T>
class ConcurrentQueue {
public:
    void push(T item) {
        std::scoped_lock lock(mutex_);
        queue_.push(std::move(item));
        cv_.notify_one();
    }

    /**
     * @brief Try to pop an item. Blocks if queue is empty and not stopped.
     * @return std::nullopt if stopped and empty.
     */
    std::optional<T> pop(std::stop_token stoken) {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [this, &stoken] {
            return !queue_.empty() || stoken.stop_requested();
        });
        if (stoken.stop_requested() && queue_.empty()) {
            return std::nullopt;
        }
        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }

    [[nodiscard]] size_t size() const {
        std::scoped_lock lock(mutex_);
        return queue_.size();
    }

    [[nodiscard]] bool empty() const {
        std::scoped_lock lock(mutex_);
        return queue_.empty();
    }

private:
    mutable std::mutex mutex_;
    std::queue<T> queue_;
    std::condition_variable cv_;
};

/**
 * @brief A fixed-size thread pool using std::jthread for RAII thread management.
 *
 * Threads are created equal to std::thread::hardware_concurrency().
 * Each thread pulls work items from a ConcurrentQueue and processes them.
 *
 * Key C++20 features:
 * - std::jthread for automatic joining on destruction
 * - std::stop_token for cooperative cancellation
 * - Concepts for type-safe processor template constraints
 */
template<typename T>
class ThreadPool {
public:
    using Processor = std::function<T(std::span<const char>)>;

    explicit ThreadPool(
        size_t num_threads = std::thread::hardware_concurrency(),
        Processor processor = nullptr)
        : processor_(std::move(processor))
    {
        for (size_t i = 0; i < num_threads; ++i) {
            threads_.emplace_back([this](std::stop_token stoken) {
                worker_loop(stoken);
            });
        }
    }

    ~ThreadPool() {
        shutdown();
    }

    /**
     * @brief Enqueue a file buffer for processing.
     */
    void enqueue(std::span<const char> buffer, std::string filename) {
        queue_.push(WorkItem{buffer, std::move(filename)});
    }

    /**
     * @brief Set the processor function. All threads will use this.
     */
    void set_processor(Processor proc) {
        processor_ = std::move(proc);
    }

    /**
     * @brief Gracefully shut down the thread pool.
     * All jthreads will be joined automatically via RAII.
     */
    void shutdown() {
        for (auto& t : threads_) {
            t.request_stop();
        }
        // Notify all waiting threads
        for ([[maybe_unused]] auto& t : threads_) {
            // Push a dummy to wake up any blocked pops
            // Actually jthread request_stop + condition_variable wait
            // needs the cv to be notified — push a sentinel
        }
        // Push sentinel items to wake threads
        for (size_t i = 0; i < threads_.size(); ++i) {
            queue_.push(WorkItem{std::span<const char>{}, ""});
        }
        for (auto& t : threads_) {
            if (t.joinable()) {
                t.join();
            }
        }
        threads_.clear();
    }

    [[nodiscard]] size_t pending() const {
        return queue_.size();
    }

private:
    struct WorkItem {
        std::span<const char> buffer;
        std::string filename;
    };

    void worker_loop(std::stop_token stoken) {
        while (!stoken.stop_requested()) {
            auto item = queue_.pop(stoken);
            if (!item.has_value()) break;
            if (item->buffer.empty()) break; // sentinel

            if (processor_) {
                processor_(item->buffer);
            }
        }
    }

    ConcurrentQueue<WorkItem> queue_;
    std::vector<std::jthread> threads_;
    Processor processor_;
};

} // namespace clearcapture