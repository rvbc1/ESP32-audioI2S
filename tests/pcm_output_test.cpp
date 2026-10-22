#include "PcmOutput.h"

#include <cassert>
#include <limits>
#include <vector>

int main() {
    const int32_t first[] = {10, 11, 20, 21, 30, 31, 40, 41};
    const int32_t second[] = {-10, -11, -20, -21};
    std::vector<int32_t> firstReceived;
    std::vector<int32_t> secondReceived;
    size_t capacity = 0;
    audiolib::PcmOutput a;
    audiolib::PcmOutput b;

    a.setCallback([&](const int32_t* samples, size_t frames) {
        const size_t accepted = std::min(capacity, frames);
        firstReceived.insert(firstReceived.end(), samples, samples + accepted * 2);
        return accepted;
    });
    b.setCallback([&](const int32_t* samples, size_t frames) {
        secondReceived.insert(secondReceived.end(), samples, samples + frames * 2);
        return frames;
    });
    a.begin(first, 4);
    b.begin(second, 2);
    assert(a.drain() == 0 && a.pendingFrames() == 4);
    assert(a.drain() == 0 && a.pendingFrames() == 4);
    assert(b.drain() == 2 && b.pendingFrames() == 0);
    capacity = 1;
    assert(a.drain() == 1 && a.pendingFrames() == 3);
    capacity = 0;
    assert(a.drain() == 0 && a.pendingFrames() == 3);
    capacity = 2;
    assert(a.drain() == 2 && a.pendingFrames() == 1);
    assert(a.drain() == 1 && a.pendingFrames() == 0);
    assert(a.drain() == 0);
    assert(firstReceived == std::vector<int32_t>(first, first + 8));
    assert(secondReceived == std::vector<int32_t>(second, second + 4));

    // Stopping/replacing a source drops its pending tail and starts the new source at frame zero.
    a.begin(first, 4);
    capacity = 1;
    assert(a.drain() == 1);
    a.reset();
    assert(a.drain() == 0 && a.pendingFrames() == 0);
    a.begin(second, 2);
    capacity = 2;
    assert(a.drain() == 2);
    assert(firstReceived[firstReceived.size() - 4] == -10);

    // A faulty consumer cannot underflow the pending count or move beyond the submitted block.
    a.setCallback([](const int32_t*, size_t) { return std::numeric_limits<size_t>::max(); });
    a.begin(first, 4);
    assert(a.drain() == 4 && a.pendingFrames() == 0);
    a.begin(nullptr, 4);
    assert(a.drain() == 0);
    a.setCallback({});
    a.begin(first, 4);
    assert(a.drain() == 0 && a.pendingFrames() == 4);
}
