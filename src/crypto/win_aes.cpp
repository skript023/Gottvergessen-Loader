#include "win_aes.hpp"
#include <sstream>
#include <iomanip>

#ifdef _WIN32

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

namespace gottvergessen::crypto
{
    bool win_aes::decrypt_gcm(
        const std::vector<uint8_t>& encryptedData,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& iv,
        const std::vector<uint8_t>& tag,
        std::vector<uint8_t>& outPlaintext)
    {
        if (key.size() != KEY_SIZE || iv.size() != IV_SIZE || tag.size() != TAG_SIZE)
        {
            return false;
        }

        BCRYPT_ALG_HANDLE hAlg = NULL;
        BCRYPT_KEY_HANDLE hKey = NULL;
        NTSTATUS status = 0;
        bool success = false;

        do
        {
            status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, NULL, 0);
            if (!NT_SUCCESS(status)) break;

            status = BCryptSetProperty(
                hAlg,
                BCRYPT_CHAINING_MODE,
                (PUCHAR)BCRYPT_CHAIN_MODE_GCM,
                sizeof(BCRYPT_CHAIN_MODE_GCM),
                0);
            if (!NT_SUCCESS(status)) break;

            status = BCryptGenerateSymmetricKey(
                hAlg,
                &hKey,
                NULL,
                0,
                (PUCHAR)key.data(),
                static_cast<ULONG>(key.size()),
                0);
            if (!NT_SUCCESS(status)) break;

            BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
            BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
            authInfo.pbNonce = (PUCHAR)iv.data();
            authInfo.cbNonce = static_cast<ULONG>(iv.size());
            authInfo.pbTag = (PUCHAR)tag.data();
            authInfo.cbTag = static_cast<ULONG>(tag.size());

            ULONG cbDecrypted = 0;
            status = BCryptDecrypt(
                hKey,
                (PUCHAR)encryptedData.data(),
                static_cast<ULONG>(encryptedData.size()),
                &authInfo,
                NULL,
                0,
                NULL,
                0,
                &cbDecrypted,
                0);
            if (!NT_SUCCESS(status)) break;

            outPlaintext.resize(cbDecrypted);

            status = BCryptDecrypt(
                hKey,
                (PUCHAR)encryptedData.data(),
                static_cast<ULONG>(encryptedData.size()),
                &authInfo,
                NULL,
                0,
                outPlaintext.data(),
                static_cast<ULONG>(outPlaintext.size()),
                &cbDecrypted,
                0);
            if (!NT_SUCCESS(status)) break;

            outPlaintext.resize(cbDecrypted);
            success = true;
        } while (false);

        if (hKey) BCryptDestroyKey(hKey);
        if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0);

        return success;
    }

    std::vector<uint8_t> win_aes::hex_to_bytes(const std::string& hex)
    {
        std::vector<uint8_t> bytes;
        for (size_t i = 0; i < hex.length(); i += 2)
        {
            std::string byteString = hex.substr(i, 2);
            uint8_t byte = static_cast<uint8_t>(strtol(byteString.c_str(), nullptr, 16));
            bytes.push_back(byte);
        }
        return bytes;
    }
}

#else

namespace gottvergessen::crypto
{
    bool win_aes::decrypt_gcm(
        const std::vector<uint8_t>& encryptedData,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& iv,
        const std::vector<uint8_t>& tag,
        std::vector<uint8_t>& outPlaintext)
    {
        (void)encryptedData; (void)key; (void)iv; (void)tag; (void)outPlaintext;
        return false;
    }

    std::vector<uint8_t> win_aes::hex_to_bytes(const std::string& hex)
    {
        std::vector<uint8_t> bytes;
        for (size_t i = 0; i < hex.length(); i += 2)
        {
            std::string byteString = hex.substr(i, 2);
            uint8_t byte = static_cast<uint8_t>(strtol(byteString.c_str(), nullptr, 16));
            bytes.push_back(byte);
        }
        return bytes;
    }
}

#endif
