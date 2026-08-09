#include "common.hpp"
#include "encrypted_downloader.hpp"
#include "../crypto/win_aes.hpp"
#include "../logger.hpp"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

namespace gottvergessen
{
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
