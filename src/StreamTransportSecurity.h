#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace audiolib {
    namespace StreamTransportSecurity {
        inline constexpr int64_t MINIMUM_TLS_UNIX_TIME = 1704067200;

        inline bool isHttps(std::string_view url) { return url.compare(0, 8, "https://") == 0; }

        inline bool canConnectTls(int64_t unixTime, bool hasCustomCA, bool hasCertificateBundle) {
            return unixTime >= MINIMUM_TLS_UNIX_TIME && (hasCustomCA || hasCertificateBundle);
        }

        inline bool canFollow(bool requireTls, std::string_view target) {
            return !target.empty() && (!requireTls || isHttps(target));
        }

        inline std::string resolveUrl(std::string_view base, std::string_view reference) {
            if (reference.empty()) return {};
            for (unsigned char value : reference) {
                if (value <= 0x20 || value == 0x7f) return {};
            }

            if (reference.compare(0, 8, "https://") == 0 || reference.compare(0, 7, "http://") == 0) return std::string(reference);
            if (reference.find("://") != std::string_view::npos) return {};

            const auto schemeEnd = base.find("://");
            if (schemeEnd == std::string_view::npos) return {};
            if (reference.compare(0, 2, "//") == 0) return std::string(base.substr(0, schemeEnd + 1)) + std::string(reference);

            const auto authorityEnd = base.find_first_of("/?#", schemeEnd + 3);
            const auto origin = base.substr(0, authorityEnd);
            if (reference.front() == '/') return std::string(origin) + std::string(reference);

            const auto queryStart = base.find_first_of("?#");
            const auto basePath = base.substr(0, queryStart);
            if (reference.front() == '?') return std::string(basePath) + std::string(reference);
            const auto lastSlash = basePath.rfind('/');
            const auto directory = lastSlash > schemeEnd + 2 ? basePath.substr(0, lastSlash + 1) : origin;
            std::string result(directory);
            if (result.empty() || result.back() != '/') result += '/';
            result += reference;

            const auto pathStart = result.find('/', schemeEnd + 3);
            auto cursor = pathStart;
            while (cursor != std::string::npos && cursor < result.size()) {
                const auto queryStart = result.find_first_of("?#", pathStart);
                if (queryStart != std::string::npos && cursor >= queryStart) break;
                if (result.compare(cursor, 3, "/./") == 0) {
                    result.erase(cursor, 2);
                } else if (result.compare(cursor, 4, "/../") == 0) {
                    const auto previousSlash = cursor > pathStart ? result.rfind('/', cursor - 1) : pathStart;
                    result.erase(previousSlash, cursor + 3 - previousSlash);
                    cursor = previousSlash;
                } else {
                    cursor = result.find('/', cursor + 1);
                }
            }
            return result;
        }
    }
}
