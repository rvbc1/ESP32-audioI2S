#include "BoundedQueue.h"

#include <cassert>
#include <memory>

int main() {
    audiolib::BoundedQueue<std::unique_ptr<int>, 32, 8> events;
    for (int i = 0; i < 10000; ++i) {
        auto value = std::make_unique<int>(i);
        const bool accepted = events.tryPush(std::move(value));
        assert(accepted == (i < 32));
        assert(static_cast<bool>(value) == !accepted);
    }
    assert(events.size() == 32);
    size_t delivered = 0;
    const size_t budget = events.drainBudget();
    assert(budget == 8);
    // A producer that replenishes every consumed event cannot extend this
    // dispatch batch indefinitely, and entries retain FIFO ownership.
    for (size_t i = 0; i < budget; ++i) {
        std::unique_ptr<int> value;
        assert(events.tryPop(value) && *value == static_cast<int>(i));
        ++delivered;
        assert(events.tryPush(std::make_unique<int>(32 + i)));
    }
    assert(delivered == 8 && events.size() == 32);
    for (int i = 8; i < 40; ++i) {
        std::unique_ptr<int> value;
        assert(events.tryPop(value) && *value == i);
    }
    assert(events.size() == 0 && events.drainBudget() == 0);
    std::unique_ptr<int> untouched = std::make_unique<int>(123);
    assert(!events.tryPop(untouched) && *untouched == 123);
}
