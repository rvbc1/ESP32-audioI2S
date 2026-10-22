#include "src/StreamTransportSecurity.h"

#include <cassert>
#include <initializer_list>

using namespace audiolib::StreamTransportSecurity;

int main() {
    const auto playlist = "https://radio.example/hls/master.m3u8?token=example";
    assert(resolveUrl(playlist, "//cdn.example/chunk.aac") == "https://cdn.example/chunk.aac");
    assert(resolveUrl("http://radio.example/list.m3u", "//cdn.example/live.mp3") == "http://cdn.example/live.mp3");
    assert(resolveUrl(playlist, "segments/0001.aac") == "https://radio.example/hls/segments/0001.aac");
    assert(resolveUrl(playlist, "../low/playlist.m3u8") == "https://radio.example/low/playlist.m3u8");
    assert(resolveUrl(playlist, "/radio/0001.aac") == "https://radio.example/radio/0001.aac");
    assert(resolveUrl(playlist, "?token=next") == "https://radio.example/hls/master.m3u8?token=next");
    assert(resolveUrl("https://radio.example", "0001.aac") == "https://radio.example/0001.aac");
    assert(resolveUrl(playlist, "0001.aac?path=/../keep") == "https://radio.example/hls/0001.aac?path=/../keep");
    assert(resolveUrl(playlist, "https://cdn.example/live.mp3") == "https://cdn.example/live.mp3");

    const auto nextPlaylist = resolveUrl(playlist, "//cdn.example/playlist.m3u8");
    assert(canFollow(true, nextPlaylist));
    assert(canFollow(true, resolveUrl(nextPlaylist, "0001.aac")));
    assert(!canFollow(true, resolveUrl(nextPlaylist, "http://attacker.example/0002.aac")));
    assert(!canFollow(true, resolveUrl(nextPlaylist, "http://cdn.example:443/0002.aac")));
    assert(canFollow(false, "http://legacy-radio.example/live.mp3"));
    assert(canFollow(false, "https://radio.example/live.mp3"));
    assert(!canFollow(true, "http://legacy-radio.example/live.mp3"));

    for (const auto* reference : {"", "ftp://attacker.example/file", "https://radio.example/live\r\nHost: attacker.example"}) {
        assert(resolveUrl(playlist, reference).empty());
        assert(!canFollow(true, resolveUrl(playlist, reference)));
    }

    assert(canConnectTls(MINIMUM_TLS_UNIX_TIME, false, true));
    assert(canConnectTls(MINIMUM_TLS_UNIX_TIME, true, false));
    assert(!canConnectTls(MINIMUM_TLS_UNIX_TIME, false, false));
    assert(!canConnectTls(0, true, true));
    assert(!canConnectTls(MINIMUM_TLS_UNIX_TIME - 1, false, true));
}
