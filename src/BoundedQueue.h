#pragma once

#include <algorithm>
#include <cstddef>
#include <deque>
#include <utility>

namespace audiolib {

// Synchronization belongs to the owner; no callback runs while it holds a lock.
template <typename T, size_t Capacity, size_t BatchSize> class BoundedQueue {
  public:
    static_assert(Capacity > 0 && BatchSize > 0);
    bool full() const { return m_items.size() >= Capacity; }
    size_t size() const { return m_items.size(); }
    size_t drainBudget() const { return std::min(m_items.size(), BatchSize); }

    bool tryPush(T&& item) {
        if (full()) return false;
        m_items.push_back(std::move(item));
        return true;
    }

    bool tryPop(T& item) {
        if (m_items.empty()) return false;
        item = std::move(m_items.front());
        m_items.pop_front();
        return true;
    }

  private:
    std::deque<T> m_items;
};

} // namespace audiolib
