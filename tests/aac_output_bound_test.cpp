#include "aac_decoder/aac_decoder.h"

#include <array>
#include <cassert>
#include <fstream>
#include <iterator>
#include <vector>

int main(int argc, char** argv) {
    assert(argc == 2);
    std::ifstream file(argv[1], std::ios::binary);
    assert(file.good());
    std::vector<uint8_t> encoded(std::istreambuf_iterator<char>(file), {});
    assert(!encoded.empty());
    {
        Audio owner;
        AACDecoder first(owner);
        AACDecoder second(owner);
        assert(first.init() && second.init());
        size_t offset[2] = {};
        size_t decodedFrames[2] = {};
        AACDecoder* decoders[] = {&first, &second};
        for (size_t turn = 0; turn < 4; ++turn) {
            for (size_t source = 0; source < 2; ++source) {
                constexpr int32_t sentinel = 0x12345678;
                std::array<int32_t, 8192> output;
                output.fill(sentinel);
                int32_t bytesLeft = encoded.size() - offset[source];
                const int32_t before = bytesLeft;
                assert(decoders[source]->decode(encoded.data() + offset[source], &bytesLeft, output.data()) == 0);
                assert(bytesLeft >= 0 && bytesLeft < before);
                offset[source] += before - bytesLeft;
                const size_t samples = decoders[source]->getOutputSamples();
                assert(decoders[source]->getChannels() == 2);
                assert(samples <= output.size());
                if (samples) ++decodedFrames[source];
                // FAAD reports the total number of interleaved scalar samples,
                // not the number of stereo frames. No tail may be overwritten.
                for (size_t i = samples; i < output.size(); ++i) assert(output[i] == sentinel);
            }
        }
        assert(decodedFrames[0] > 0 && decodedFrames[1] > 0);
        first.reset();
        first.reset();
        second.reset();
    }
    assert(host_live_allocation_count() == 0);
    std::puts("PASS: actual stereo AAC decode leaves every sample beyond the produced output untouched");
}
