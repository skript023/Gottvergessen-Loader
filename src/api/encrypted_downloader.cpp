#include "common.hpp"
#include "encrypted_downloader.hpp"
#include "../crypto/win_aes.hpp"
#include "../logger.hpp"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

namespace gottvergessen
{
    bool encrypted_downloader::download_encrypted_to_file(
        const std::string& baseUrl,
        const std::string& binaryId,
        const std::string& authToken,
        const std::filesystem::path& targetPath,
        std::function<void(float)> progressCb)
    {
        if (binaryId.empty() || authToken.empty())
        {
            LOG(WARNING) << "Encrypted downloader: binary ID or auth token is empty";
            return false;
        }

        std::string authHeaderVal = "Bearer " + authToken;

        cpr::Url sessionUrl{ baseUrl + "/binary/session/" + binaryId };
        cpr::Header sessionHeader{
            { "Content-Type", "application/json" },
            { "Authorization", authHeaderVal }
        };

        auto sessionRes = cpr::Post(sessionUrl, sessionHeader);
        if (sessionRes.status_code != 200)
        {
            LOG(WARNING) << "Encrypted downloader: Failed to create session, HTTP status: " << sessionRes.status_code;
            return false;
        }

        auto sessionJson = nlohmann::json::parse(sessionRes.text, nullptr, false);
        if (sessionJson.is_discarded() || !sessionJson.contains("session_id") || !sessionJson.contains("key_hex") || !sessionJson.contains("iv_hex"))
        {
            LOG(WARNING) << "Encrypted downloader: Invalid session response JSON";
            return false;
        }

        std::string sessionId = sessionJson["session_id"].get<std::string>();
        std::string keyHex = sessionJson["key_hex"].get<std::string>();
        std::string ivHex = sessionJson["iv_hex"].get<std::string>();

        std::vector<uint8_t> aesKey = crypto::win_aes::hex_to_bytes(keyHex);
        std::vector<uint8_t> aesIv = crypto::win_aes::hex_to_bytes(ivHex);

        if (aesKey.size() != crypto::win_aes::KEY_SIZE || aesIv.size() != crypto::win_aes::IV_SIZE)
        {
            LOG(WARNING) << "Encrypted downloader: Invalid key or IV size from server";
            return false;
        }

        cpr::Url downloadUrl{ baseUrl + "/binary/download/encrypted/" + binaryId };
        cpr::Header downloadHeader{
            { "X-Session-ID", sessionId }
        };

        cpr::Response downloadRes;
        if (progressCb)
        {
            downloadRes = cpr::Get(downloadUrl, downloadHeader, cpr::ProgressCallback([&progressCb](cpr::cpr_off_t downloadTotal, cpr::cpr_off_t downloadedCurrent, cpr::cpr_off_t, cpr::cpr_off_t, intptr_t) -> bool {
                if (downloadTotal > 0)
                {
                    progressCb(static_cast<float>(downloadedCurrent) / static_cast<float>(downloadTotal));
                }
                return true;
            }));
        }
        else
        {
            downloadRes = cpr::Get(downloadUrl, downloadHeader);
        }

        if (downloadRes.status_code != 200 || downloadRes.text.size() <= crypto::win_aes::TAG_SIZE)
        {
            LOG(WARNING) << "Encrypted downloader: Download failed or payload too small, status: " << downloadRes.status_code;
            return false;
        }

        std::ofstream outFile(targetPath, std::ios::binary | std::ios::trunc);
        if (!outFile.is_open())
        {
            LOG(WARNING) << "Encrypted downloader: Failed to open target file for writing encrypted payload: " << targetPath.string();
            return false;
        }

        // Write AES Key (32 bytes) + IV (12 bytes) header, followed by encrypted payload (ciphertext + tag)
        outFile.write(reinterpret_cast<const char*>(aesKey.data()), aesKey.size());
        outFile.write(reinterpret_cast<const char*>(aesIv.data()), aesIv.size());
        outFile.write(downloadRes.text.data(), downloadRes.text.size());
        outFile.close();

        LOG(INFO) << "Encrypted downloader: Encrypted binary stored on disk: " << targetPath.string();
        return true;
    }

    bool encrypted_downloader::decrypt_file_to_temp(
        const std::string& /*baseUrl*/,
        const std::string& /*binaryId*/,
        const std::string& /*authToken*/,
        const std::filesystem::path& encPath,
        std::filesystem::path& outTempPath)
    {
        if (!std::filesystem::exists(encPath))
        {
            LOG(WARNING) << "Encrypted downloader: Target encrypted file does not exist: " << encPath.string();
            return false;
        }

        std::ifstream inFile(encPath, std::ios::binary | std::ios::ate);
        if (!inFile.is_open())
        {
            LOG(WARNING) << "Encrypted downloader: Could not open encrypted file on disk.";
            return false;
        }

        size_t totalLen = static_cast<size_t>(inFile.tellg());
        constexpr size_t headerLen = crypto::win_aes::KEY_SIZE + crypto::win_aes::IV_SIZE;
        constexpr size_t minTotalLen = headerLen + crypto::win_aes::TAG_SIZE;

        if (totalLen <= minTotalLen)
        {
            LOG(WARNING) << "Encrypted downloader: File on disk is too small for decryption.";
            inFile.close();
            return false;
        }

        std::vector<uint8_t> rawPayload(totalLen);
        inFile.seekg(0, std::ios::beg);
        inFile.read(reinterpret_cast<char*>(rawPayload.data()), totalLen);
        inFile.close();

        // Extract key (32 bytes) and IV (12 bytes) from payload header
        std::vector<uint8_t> aesKey(rawPayload.begin(), rawPayload.begin() + crypto::win_aes::KEY_SIZE);
        std::vector<uint8_t> aesIv(rawPayload.begin() + crypto::win_aes::KEY_SIZE, rawPayload.begin() + headerLen);

        size_t bodyLen = totalLen - headerLen;
        size_t ciphertextLen = bodyLen - crypto::win_aes::TAG_SIZE;

        std::vector<uint8_t> ciphertext(rawPayload.begin() + headerLen, rawPayload.begin() + headerLen + ciphertextLen);
        std::vector<uint8_t> tag(rawPayload.begin() + headerLen + ciphertextLen, rawPayload.end());

        std::vector<uint8_t> decryptedBytes;
        if (!crypto::win_aes::decrypt_gcm(ciphertext, aesKey, aesIv, tag, decryptedBytes))
        {
            LOG(WARNING) << "Encrypted downloader: Decryption failed before loading!";
            return false;
        }

        // Create temporary decrypted file (e.g., temp_decrypted_xyz.dll in temp directory)
        std::filesystem::path tempDir = std::filesystem::temp_directory_path();
        std::string tempFilename = "el_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".dll";
        outTempPath = tempDir / tempFilename;

        std::ofstream outFile(outTempPath, std::ios::binary | std::ios::trunc);
        if (!outFile.is_open())
        {
            LOG(WARNING) << "Encrypted downloader: Failed to write decrypted binary to temp path: " << outTempPath.string();
            return false;
        }

        outFile.write(reinterpret_cast<const char*>(decryptedBytes.data()), decryptedBytes.size());
        outFile.close();

        LOG(HACKER) << "Encrypted downloader: Binary successfully decrypted to temporary file: " << outTempPath.string();
        return true;
    }

    bool encrypted_downloader::download_and_decrypt_to_memory(
        const std::string& baseUrl,
        const std::string& binaryId,
        const std::string& authToken,
        std::vector<uint8_t>& outDecryptedBytes,
        std::function<void(float)> progressCb)
    {
        outDecryptedBytes.clear();

        if (binaryId.empty() || authToken.empty())
        {
            LOG(WARNING) << "Encrypted downloader: binary ID or auth token is empty";
            return false;
        }

        std::string authHeaderVal = "Bearer " + authToken;

        cpr::Url sessionUrl{ baseUrl + "/binary/session/" + binaryId };
        cpr::Header sessionHeader{
            { "Content-Type", "application/json" },
            { "Authorization", authHeaderVal }
        };

        auto sessionRes = cpr::Post(sessionUrl, sessionHeader);
        if (sessionRes.status_code != 200)
        {
            LOG(WARNING) << "Encrypted downloader: Failed to create session, HTTP status: " << sessionRes.status_code;
            return false;
        }

        auto sessionJson = nlohmann::json::parse(sessionRes.text, nullptr, false);
        if (sessionJson.is_discarded() || !sessionJson.contains("session_id") || !sessionJson.contains("key_hex") || !sessionJson.contains("iv_hex"))
        {
            LOG(WARNING) << "Encrypted downloader: Invalid session response JSON";
            return false;
        }

        std::string sessionId = sessionJson["session_id"].get<std::string>();
        std::string keyHex = sessionJson["key_hex"].get<std::string>();
        std::string ivHex = sessionJson["iv_hex"].get<std::string>();

        std::vector<uint8_t> aesKey = crypto::win_aes::hex_to_bytes(keyHex);
        std::vector<uint8_t> aesIv = crypto::win_aes::hex_to_bytes(ivHex);

        cpr::Url downloadUrl{ baseUrl + "/binary/download/encrypted/" + binaryId };
        cpr::Header downloadHeader{
            { "X-Session-ID", sessionId }
        };

        cpr::Response downloadRes;
        if (progressCb)
        {
            downloadRes = cpr::Get(downloadUrl, downloadHeader, cpr::ProgressCallback([&progressCb](cpr::cpr_off_t downloadTotal, cpr::cpr_off_t downloadedCurrent, cpr::cpr_off_t, cpr::cpr_off_t, intptr_t) -> bool {
                if (downloadTotal > 0)
                {
                    progressCb(static_cast<float>(downloadedCurrent) / static_cast<float>(downloadTotal));
                }
                return true;
            }));
        }
        else
        {
            downloadRes = cpr::Get(downloadUrl, downloadHeader);
        }

        if (downloadRes.status_code != 200 || downloadRes.text.size() <= crypto::win_aes::TAG_SIZE)
        {
            LOG(WARNING) << "Encrypted downloader: Download failed or payload too small, status: " << downloadRes.status_code;
            return false;
        }

        const std::string& rawPayload = downloadRes.text;
        size_t totalLen = rawPayload.size();
        size_t ciphertextLen = totalLen - crypto::win_aes::TAG_SIZE;

        std::vector<uint8_t> ciphertext(rawPayload.begin(), rawPayload.begin() + ciphertextLen);
        std::vector<uint8_t> tag(rawPayload.begin() + ciphertextLen, rawPayload.end());

        if (!crypto::win_aes::decrypt_gcm(ciphertext, aesKey, aesIv, tag, outDecryptedBytes))
        {
            LOG(WARNING) << "Encrypted downloader: Failed to decrypt payload in memory!";
            outDecryptedBytes.clear();
            return false;
        }

        LOG(INFO) << "Encrypted downloader: Successfully downloaded & decrypted payload in RAM! Size: " << outDecryptedBytes.size() << " bytes";
        return true;
    }
}
