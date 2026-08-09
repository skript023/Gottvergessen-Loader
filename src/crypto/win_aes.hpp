#pragma once

#include <vector>
#include <string>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#endif

namespace gottvergessen::crypto
{
    class win_aes
    {
    public:
        static constexpr size_t KEY_SIZE = 32;
        static constexpr size_t IV_SIZE = 12;
        static constexpr size_t TAG_SIZE = 16;

        static bool decrypt_gcm(
            const std::vector<uint8_t>& encryptedData,
            const std::vector<uint8_t>& key,
            const std::vector<uint8_t>& iv,
            const std::vector<uint8_t>& tag,
            std::vector<uint8_t>& outPlaintext);

        static std::vector<uint8_t> hex_to_bytes(const std::string& hex);
    };
}
