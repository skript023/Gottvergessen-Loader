#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <functional>

namespace gottvergessen
{
    class encrypted_downloader
    {
    public:
        static bool download_and_decrypt_to_memory(
            const std::string& baseUrl,
            const std::string& binaryId,
            const std::string& authToken,
            std::vector<uint8_t>& outDecryptedBytes,
            std::function<void(float)> progressCb = nullptr);
    };
}
