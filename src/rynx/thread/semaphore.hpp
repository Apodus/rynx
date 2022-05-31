
#pragma once

#include <rynx/system/assert.hpp>

#include <atomic>
#include <cstdint>

class semaphore {
public:
    
    constexpr explicit semaphore(const int32_t desired = 0) noexcept /* strengthened */
        : m_counter(desired) {
        rynx_assert(desired >= 0, "initial state must be non-negative");
    }

    semaphore(const semaphore&) = delete;
    semaphore& operator=(const semaphore&) = delete;

    void release() noexcept {
        m_counter.fetch_add(1);
        if (m_waiting.load() > 0) {
            m_counter.notify_one();
        }
    }

    void acquire() noexcept {
        int32_t current = m_counter.load(std::memory_order_relaxed);
        for (;;) {
            while (current == 0) {
                internal_wait();
                current = m_counter.load(std::memory_order_relaxed);
            }
            rynx_assert(current > 0, "should be at least one");
            if (m_counter.compare_exchange_weak(current, current - 1)) {
                return;
            }
        }
    }

    [[nodiscard]] bool try_acquire() noexcept {
        int32_t current = m_counter.load();
        if (current == 0) {
            return false;
        }
        rynx_assert(current > 0, "should be at least one");
        return m_counter.compare_exchange_weak(current, current - 1);
    }

    void signal() {
        release();
    }

    void wait() {
        acquire();
    }

private:
    void internal_wait() noexcept {
        m_waiting.fetch_add(1);
        int32_t current = m_counter.load();
        if (current == 0) {
            m_counter.wait(current);
        }
        m_waiting.fetch_sub(1, std::memory_order_relaxed);
    }

    std::atomic<int32_t> m_counter;
    std::atomic<int32_t> m_waiting;
};

class semaphore_binary {
public:
    constexpr explicit semaphore_binary(const int32_t desired = 0) noexcept /* strengthened */
        : m_counter(static_cast<unsigned char>(desired)) {
        rynx_assert(desired >= 0 && desired <= 1, "binary state only allowed");
    }

    semaphore_binary(const semaphore_binary&) = delete;
    semaphore_binary& operator=(const semaphore_binary&) = delete;

    void release() noexcept {
        m_counter.store(1);
        m_counter.notify_one();
    }

    void acquire() noexcept {
        for (;;) {
            unsigned char prev = m_counter.exchange(0);
            if (prev == 1) {
                break;
            }
            rynx_assert(prev == 0, "must be zero");
            m_counter.wait(0, std::memory_order_relaxed);
        }
    }

    _NODISCARD bool try_acquire() noexcept {
        unsigned char prev = m_counter.exchange(0);
        rynx_assert((prev & ~1) == 0, "only first bit is allowed to be set");
        return reinterpret_cast<const bool&>(prev);
    }

    void signal() {
        release();
    }

    void wait() {
        acquire();
    }

private:
    std::atomic<unsigned char> m_counter;
};
