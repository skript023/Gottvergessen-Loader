#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <filesystem>

namespace gottvergessen
{
    class encrypted_downloader
    {
    public:
        // Download raw encrypted payload from server directly to disk
        static bool download_encrypted_to_file(
            const std::string& baseUrl,
            const std::string& binaryId,
            const std::string& authToken,
            const std::filesystem::path& targetPath,
            std::function<void(float)> progressCb = nullptr);

        // Decrypt an encrypted payload on disk to a temporary location prior to loading/injecting
        static bool decrypt_file_to_temp(
            const std::string& baseUrl,
            const std::string& binaryId,
            const std::string& authToken,
            const std::filesystem::path& encPath,
            std::filesystem::path& outTempPath);

        // Download and decrypt payload directly into RAM
        static bool download_and_decrypt_to_memory(
            const std::string& baseUrl,
            const std::string& binaryId,
            const std::string& authToken,
            std::vector<uint8_t>& outDecryptedBytes,
            std::function<void(float)> progressCb = nullptr);
    };
}
