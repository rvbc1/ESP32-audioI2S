#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>

namespace audiolib {

class PcmOutput {
  public:
    using Callback = std::function<size_t(const int32_t*, size_t)>;

    void setCallback(Callback callback) { m_callback = std::move(callback); }
    bool configured() const { return static_cast<bool>(m_callback); }
    size_t pendingFrames() const { return m_frames; }

    void begin(const int32_t* samples, size_t frames) {
        m_samples = samples;
        m_frames = samples ? frames : 0;
    }

    size_t drain() {
        if (!m_callback || !m_frames) return 0;
        const size_t consumed = std::min(m_callback(m_samples, m_frames), m_frames);
        m_samples += consumed * 2;
        m_frames -= consumed;
        return consumed;
    }

    void reset() {
        m_samples = nullptr;
        m_frames = 0;
    }

  private:
    Callback       m_callback;
    const int32_t* m_samples = nullptr;
    size_t         m_frames = 0;
};

} // namespace audiolib
