# Host regression tests

Run with Python 3 and a Clang toolchain supporting C++20, AddressSanitizer and
UndefinedBehaviorSanitizer:

```sh
python3 tests/run_host_tests.py
```

The lifecycle test compiles the actual FAAD implementation and pointer owner.
It requires no outstanding tracked allocation after every sequential or
interleaved two-instance Open/Init2/Close lifecycle, in floating-point and
fixed-point profiles, with PSRAM-present and internal-RAM fallback adapters.
Repeated cleanup follows the wrapper's contract: clear the handle after Close;
a second reset closes NULL, never a dangling handle.

The host overlay replaces Arduino/platform logging only. The FAAD `.cpp` is
unchanged. The `NeAACDecInit` declaration is adapted from `long` to `int32_t` to
match its existing definition on LP64 hosts. Clang's delayed template parsing
avoids an unrelated, unused pointer-template method. ASAN/UBSAN check memory
access; an allocation ledger checks leaks and rejects unknown/double frees.
LeakSanitizer is disabled for macOS compatibility. This is not a device, task
scheduling, audio-output or allocation-failure test.

To reproduce the floating-point FFT table leak on a pre-fix revision:

```sh
python3 tests/run_host_tests.py --source-ref <pre-fix-commit> --expect-leak
```

That control must find 100 allocations of each 512-, 4096- and 2048-byte table
(665600 bytes total), then discards the intentionally leaked tables. The
fixed-point control must remain leak-free because its tables are static.

The AAC output-bound test compiles the real wrapper and decodes a small stereo
AAC-LC silence fixture through two interleaved decoder instances. A sentinel
after the reported scalar-sample count catches writes beyond produced output.
Its overlay uses the real `Decoder` interface with unused base fallback methods
and platform logging stubbed; all AAC wrapper and FAAD implementation is real.
The fixture was generated from silence using:

```sh
ffmpeg -f lavfi -i anullsrc=r=44100:cl=stereo -t 0.1 -c:a aac -b:a 64000 -f adts tests/fixtures/silence-stereo-44100.aac
```

The PCM test checks real output delivery with zero/partial consumption, retry,
independent sinks, reset and an invalid over-reporting consumer. The event queue
test floods 10000 move-only entries, verifies bounded FIFO ownership and checks
that continuous producer replenishment cannot extend the fixed dispatch budget.

The suite also runs the [MP3 leading-zero regression](MP3_CLZ.md) with full
ASAN/UBSAN and no arithmetic sanitizer exclusions.
