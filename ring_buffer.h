#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <array>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>

/**
 * @brief Thread-safe ring buffer with blocking/non-blocking operations
 * @tparam T Element type (should be trivially copyable)
 * @tparam Capacity Buffer capacity (default 1024)
 * 
 * Producer: push() - blocks if full
 * Consumer: pop() - blocks if empty, try_pop() - returns immediately
 */
template <typename T, size_t Capacity = 1024>
class RingBuffer {
public:
    RingBuffer() : head_(0), tail_(0), shutdown_(false) {
        static_assert(std::is_trivially_copyable_v<T>, 
                      "RingBuffer element type must be trivially copyable");
    }

    ~RingBuffer() {
        shutdown();
    }

    // No copy/move semantics (thread synchronization primitives)
    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    /**
     * @brief Push element to buffer (blocks if full)
     * @param item Element to push
     * @return false if shutdown, true otherwise
     */
    bool push(const T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        // Wait until buffer is not full or shutdown
        cv_not_full_.wait(lock, [this]() {
            return !is_full_unsafe() || shutdown_.load(std::memory_order_acquire);
        });

        if (shutdown_.load(std::memory_order_acquire)) {
            return false;
        }

        buffer_[head_] = item;
        head_ = (head_ + 1) % Capacity;
        
        lock.unlock();
        cv_not_empty_.notify_one();
        return true;
    }

    /**
     * @brief Pop element from buffer (blocks if empty)
     * @param item Output parameter for popped element
     * @return false if shutdown and buffer empty, true otherwise
     */
    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        // Wait until buffer is not empty or shutdown
        cv_not_empty_.wait(lock, [this]() {
            return head_ != tail_ || shutdown_.load(std::memory_order_acquire);
        });

        // If shutdown and buffer is empty, return false
        if (shutdown_.load(std::memory_order_acquire) && head_ == tail_) {
            return false;
        }

        item = buffer_[tail_];
        tail_ = (tail_ + 1) % Capacity;
        
        lock.unlock();
        cv_not_full_.notify_one();
        return true;
    }

    /**
     * @brief Try to pop element without blocking
     * @return std::optional with element if available, std::nullopt otherwise
     */
    std::optional<T> try_pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (head_ == tail_) {
            return std::nullopt;  // Buffer empty
        }

        T item = buffer_[tail_];
        tail_ = (tail_ + 1) % Capacity;
        
        lock.unlock();
        cv_not_full_.notify_one();
        return item;
    }

    /**
     * @brief Signal shutdown and wake all waiting threads
     */
    void shutdown() {
        shutdown_.store(true, std::memory_order_release);
        cv_not_empty_.notify_all();
        cv_not_full_.notify_all();
    }

    /**
     * @brief Check if buffer is empty (racy - for diagnostics only)
     */
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return head_ == tail_;
    }

    /**
     * @brief Get current buffer size (racy - for diagnostics only)
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (head_ >= tail_) {
            return head_ - tail_;
        } else {
            return Capacity - tail_ + head_;
        }
    }

private:
    bool is_full_unsafe() const {
        return (head_ + 1) % Capacity == tail_;
    }

    std::array<T, Capacity> buffer_;
    size_t head_;  // Write position
    size_t tail_;  // Read position
    
    mutable std::mutex mutex_;
    std::condition_variable cv_not_empty_;
    std::condition_variable cv_not_full_;
    std::atomic<bool> shutdown_;
};

#endif // RING_BUFFER_H
