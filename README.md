# ESP32-audioI2S

:warning: **This library only works on multi-core chips like ESP32, ESP32-S3 and ESP32-P4. Your board must have PSRAM! It does not work on the ESP32-S2, ESP32-C3 etc** :warning:

Plays mp3, m4a and wav files from SD card via I2S with external hardware.
HELIX-mp3 and faad2-aac decoder is included. There is also an OPUS decoder for Fullband, an VORBIS decoder and a FLAC decoder.
Works with MAX98357A (3 Watt amplifier with DAC), connected three lines (DOUT, BLCK, LRC) to I2S. The I2S output frequency is always 48kHz, regardless of the input source, so Bluetooth devices can also be connected without any problems.
For stereo are two MAX98357A necessary. AudioI2S works with UDA1334A (Adafruit I2S Stereo Decoder Breakout Board), PCM5102A and CS4344.
Other HW may work but not tested. Plays also icy-streams, GoogleTTS and OpenAIspeech. Can be compiled with Arduino IDE. [WIKI](https://github.com/schreibfaul1/ESP32-audioI2S/wiki)

```` c++
#include "Arduino.h"
#include "WiFi.h"
#include "Audio.h"

// Digital I/O used
#define I2S_DOUT      25
#define I2S_BCLK      27
#define I2S_LRC       26

String ssid =     "*******";
String password = "*******";

Audio audio;

// callbacks
void my_audio_info(Audio::msg_t m) {
    Serial.printf("%s: %s\n", m.s, m.msg);
}

void setup() {
    Audio::audio_info_callback = my_audio_info; // optional
    Serial.begin(115200);
    WiFi.begin(ssid.c_str(), password.c_str());
    while (WiFi.status() != WL_CONNECTED) delay(1500);
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(21); // default 0...21
    audio.connecttohost("http://stream.antennethueringen.de/live/aac-64/stream.antennethueringen.de/");
}

void loop(){
    audio.loop();
    vTaskDelay(1);
}

````

## Continuous stream buffering

The input ring buffer is stored in PSRAM. Its capacity and the preload threshold for continuous web streams can be configured before the first connection:

````c++
audio.setInputBufferSize(2 * 1024 * 1024);
audio.settings.BUFFER_THRESHOLD_WEBSTREAM_INITIAL = 64 * 1024;
audio.settings.BUFFER_THRESHOLD_WEBSTREAM = 256 * 1024;
audio.settings.BUFFER_INITIAL_PRELOAD_TIMEOUT_MS = 5000;
audio.settings.BUFFER_PRELOAD_TIMEOUT_MS = 45000;
````

The initial threshold can be smaller for a fast first start while the ring buffer continues filling in the background. When the decoder exhausts the input data, playback waits for `BUFFER_THRESHOLD_WEBSTREAM` before it resumes. Setting either initial option to `0` makes it inherit the corresponding rebuffer option, preserving the previous behavior. `isBuffering()`, `inBufferFilled()`, `getInBufferSize()` and `getRebufferCount()` expose the current state for diagnostics. A rebuffer threshold of `0` keeps the legacy continuous-stream behavior.

## HTTPS verification

HTTPS connections verify the server certificate with the ESP-IDF public CA bundle when `CONFIG_MBEDTLS_CERTIFICATE_BUNDLE` is enabled. Set the device wall clock before connecting; an unset clock causes HTTPS to fail. Certificate validity dates and the hostname are checked by the TLS stack.

For a private server, call `audio.setCACert(pem)` before connecting. The method copies the PEM into library-owned memory and stops current playback; it returns `false` if allocation fails. `audio.setCACert(nullptr)` restores the public bundle. Without a configured bundle or a custom CA, HTTPS fails closed.

Once a stream uses HTTPS, HTTP redirects, HLS playlists, segments, range requests, and internal reconnects cannot downgrade that stream to HTTP. Protocol-relative URLs preserve the current scheme. A new explicit `connecttohost("http://...")` remains available for legacy HTTP stations. Basic authentication on reconnect and the buffering APIs described above remain supported.

## Independent PCM output

`ESP32_AUDIO_I2S_PCM_OUTPUT` identifies the optional PCM output API. Initialize it
instead of calling `setPinout()` when the application owns the output device:

```cpp
Audio source;
source.setInfoCallback([](Audio::msg_t message) {
    // Handle this source's events. Copy message strings if retaining them.
});
bool ready = source.initPcmOutput([](const int32_t* samples, size_t frames) -> size_t {
    // Copy up to `frames` into an application-owned queue; return frames copied.
    return copyToOutputQueue(samples, frames);
});
```

The callback receives interleaved stereo `int32_t` PCM at 48 kHz before volume,
gain, EQ and mono processing. The legacy global PCM hooks are skipped in this
mode. A zero or partial return preserves the remaining frames for a later call;
the pointer is valid only during the callback. The callback runs on the source's
decoder task and must return promptly: it must neither block nor call `Audio`
methods. `isPcmOutputReady()` reports successful initialization and a running
decoder task; it does not mean that a stream has produced PCM yet.

Each `Audio` instance owns its decoder task and pending PCM. Two PCM instances
can feed a mixer without allocating or controlling I2S channels, VU or spectrum
buffers. The application must provide output pacing, queue capacity, mixing and
the physical I2S owner. Normal `setPinout()` playback remains available.

Use one control task per instance for `loop()`, connection, stop and destruction.
Control methods are not a general concurrent API. `stopSong()` waits for any
in-flight PCM callback; do not hold an application queue lock needed by that
callback while stopping or deleting a source. Destruction joins the decoder
task. Concurrent `stopSong()`/destruction from multiple owners is unsupported.

`setInfoCallback()` selects an instance callback, falling back to the legacy
global callback when unset. Stream events are queued per instance, with at most
32 pending events and eight deliveries per `loop()`. Excess events, text over
4096 bytes and image descriptors over 1024 integers are rejected. Callbacks run
outside the queue lock. Low-level static decoder logging still uses the global
logging callback. Host regression commands and limitations are in
[tests/README.md](tests/README.md).

You can find more examples here: https://github.com/schreibfaul1/ESP32-audioI2S/tree/master/examples

````c++
// detailed cb output
void my_audio_info(Audio::msg_t m) {
    switch(m.e){
        case Audio::evt_info:           Serial.printf("info: ....... %s\n", m.msg); break;
        case Audio::evt_eof:            Serial.printf("end of file:  %s\n", m.msg); break;
        case Audio::evt_bitrate:        Serial.printf("bitrate: .... %s\n", m.msg); break; // icy-bitrate or bitrate from metadata
        case Audio::evt_icyurl:         Serial.printf("icy URL: .... %s\n", m.msg); break;
        case Audio::evt_id3data:        Serial.printf("ID3 data: ... %s\n", m.msg); break; // id3-data or metadata
        case Audio::evt_lasthost:       Serial.printf("last URL: ... %s\n", m.msg); break;
        case Audio::evt_name:           Serial.printf("station name: %s\n", m.msg); break; // station name or icy-name
        case Audio::evt_streamtitle:    Serial.printf("stream title: %s\n", m.msg); break;
        case Audio::evt_icylogo:        Serial.printf("icy logo: ... %s\n", m.msg); break;
        case Audio::evt_icydescription: Serial.printf("icy descr: .. %s\n", m.msg); break;
        case Audio::evt_image: for(int i = 0; i < m.vec.size(); i += 2){
                                        Serial.printf("cover image:  segment %02i, pos %07lu, len %05lu\n", i / 2, m.vec[i], m.vec[i + 1]);} break; // APIC
        case Audio::evt_lyrics:         Serial.printf("sync lyrics:  %s\n", m.msg); break;
        case Audio::evt_log   :         Serial.printf("audio_logs:   %s\n", m.msg); break;
        default:                        Serial.printf("message:..... %s\n", m.msg); break;
    }
}
````
<br>

|Codec       | ESP32       |ESP32-S3 or ESP32-P4         |                          |
|------------|-------------|-----------------------------|--------------------------|
| mp3        | y           | y                           |                          |
| aac        | y           | y                           |                          |
| aacp       | y (mono)    | y (+SBR, +Parametric Stereo)|                          |
| wav        | y           | y                           |                          |
| flac       | y           | y                           |blocksize max 24576 bytes |
| vorbis     | y           | y                           | <=196Kbit/s              |
| m4a        | y           | y                           |                          |
| opus       | y           | y                           |                          |

<br>

***
Wiring
![schematic](https://github.com/user-attachments/assets/77ce30d2-acb1-4b5d-a9d6-4f1e3d56e385)

***
Impulse diagram
![Impulse diagram](https://github.com/schreibfaul1/ESP32-audioI2S/blob/master/additional_info/Impulsdiagramm.jpg)
***
Yellobyte has developed an all-in-one board. It includes an ESP32-S3 N8R2, 2x MAX98357 and an SD card adapter.
Documentation, circuit diagrams and examples can be found here: https://github.com/yellobyte/ESP32-DevBoards-Getting-Started
![image](https://github.com/user-attachments/assets/4002d09e-8e76-4e08-9265-188fed7628d3)
