#pragma once
#include "common.hpp"
#include "file_manager.hpp"
#include "api/http_request.hpp"
#include "api/url_encryption.hpp"
#include "api/user/user_authentication.hpp"

namespace gottvergessen
{
	struct VersionInfo
	{
		const int m_id;
		const std::string m_game;
		const unsigned int m_code;
		const std::string m_file;
		const std::string m_target;
		const std::string m_version;
		const int m_version_machine;
		const BOOL m_supported;
		const BOOL m_valid;
	};

	struct LoaderVersion
	{
		const std::string m_path;
		const std::string m_version;
		const int m_version_machine;
		const bool m_supported;
		const bool m_valid;
	};

	struct BinaryName
	{
		std::string m_name;
		std::string m_server_name;
	};

	class get_version
	{
		friend class download_binary;
	public:
		explicit get_version() = default;
		virtual ~get_version() noexcept = default;

		get_version(get_version const& that) = delete;
		get_version& operator=(get_version const& that) = delete;
		get_version(get_version&& that) = delete;
		get_version& operator=(get_version&& that) = delete;

		LoaderVersion get_loader_version()
		{
			try
			{
				cpr::Url url = xorstr("https://gottvergessen.000webhostapp.com/api/v1/version");
				cpr::Header header = { { xorstr("Accept"), xorstr("application/json")} };
				auto res = cpr::Get(url, header);

				nlohmann::json j = nlohmann::json::parse(res.text.begin(), res.text.end());

				return { j["file"], j["version"], j["version_machine"], j["supported"], j["valid"] };
			}
			catch (const std::exception&)
			{
				LOG(WARNING) << "Failed to get loader version info, is the host down?";
			}

			return {};
		}

		VersionInfo get_version_info()
		{
			nlohmann::ordered_json json = {
				{ xorstr("name"), m_selected_binary}
			};

			std::string token = std::format("Bearer {}", g_user_authentication->get_token());

			try
			{
				cpr::Body body = json.dump();
				cpr::Header header = { 
					{ xorstr("Accept"), xorstr("application/json") }, 
					{ xorstr("Content-Type"), xorstr("application/json") },
					{ xorstr("Authorization"), token },
				};

				auto res = cpr::Post(url, body, header);

				nlohmann::json j = nlohmann::json::parse(res.text.begin(), res.text.end());

				m_filename = j["file"];
				m_target_process = j["target"];

				return { j["id"], j["game"], j["code"], j["file"], j["target"], j["version"], j["version_machine"], j["supported"], j["valid"] };
			}
			catch (const std::exception&)
			{
				LOG(WARNING) << "Failed to get binary version info, is the host down?";
			}

			return {};
		}

		bool ensure_version_file()
		{
			auto folder = g_file_manager->get_project_folder("./Binary");
			auto base_dir = folder.get_file("./version.json").get_path();

			if (std::filesystem::exists(base_dir)) return true;

			nlohmann::ordered_json json = {
				{ xorstr("name"), m_selected_binary }
			};

			std::string token = std::format("Bearer {}", g_user_authentication->get_token());

			std::ofstream file(base_dir, std::ios::out | std::ios::trunc);

			try
			{
				cpr::Body body = json.dump();
				cpr::Header header = {
					{ xorstr("Accept"), xorstr("application/json") },
					{ xorstr("Content-Type"), xorstr("application/json") },
					{ xorstr("Authorization"), token },
				};

				auto res = cpr::Post(url, body, header);

				nlohmann::ordered_json j = nlohmann::ordered_json::parse(res.text.begin(), res.text.end());

				file << j.dump(4);
			}
			catch (const std::exception&)
			{
				LOG(WARNING) << "Failed to get binary version info, is the host down?";
				
				return false;
			}

			return true;
		}

		bool download_version_file()
		{
			auto folder = g_file_manager->get_project_folder(xorstr("./Binary"));
			auto base_dir = folder.get_file(xorstr("./version.json")).get_path();

			nlohmann::ordered_json json = {
				{ xorstr("name"), m_selected_binary}
			};

			std::string token = std::format("Bearer {}", g_user_authentication->get_token());

			std::ofstream file(base_dir, std::ios::out | std::ios::trunc);

			try
			{
				cpr::Body body = json.dump();
				cpr::Header header = {
					{xorstr("Accept"), xorstr("application/json")},
					{xorstr("Content-Type"), xorstr("application/json")},
					{xorstr("Authorization"), token},
				};

				auto res = cpr::Post(url, body, header);

				nlohmann::ordered_json j = nlohmann::ordered_json::parse(res.text.begin(), res.text.end());

				file << j.dump(4);
			}
			catch (const std::exception&)
			{
				LOG(WARNING) << "Failed to get binary version info, is the host down?";

				return false;
			}

			return true;
		}

		VersionInfo get_current_version()
		{
			auto folder = g_file_manager->get_project_folder(xorstr("./Binary"));
			auto base_dir = folder.get_file(xorstr("./version.json")).get_path();

			std::ifstream file(base_dir);

			nlohmann::json json_file;

			if (file.fail())
			{
				LOG(WARNING) << xorstr("File doesn't exist");

				this->download_version_file();

				LOG(HACKER) << xorstr("new version file downloaded successfully from server");
			}

			if (!file.is_open())
			{
				file.open(base_dir);
			}

			file >> json_file;

			auto& j = json_file;

			file.close();

			m_filename = j["file"];
			m_target_process = j["target"];

			return { j["id"], j["game"], j["code"], j["file"], j["target"], j["version"], j["version_machine"], j["supported"], j["valid"] };
		}

		VersionInfo m_version_info{};
	private:
		std::string m_key[8] = { "id", "game", "file", "target", "version", "version_machine", "supported", "valid" };
		std::string m_selected_binary;
		std::string m_target_process;
		std::string m_filename;
		const cpr::Url url = xorstr("https://gottvergessen.000webhostapp.com/api/v1/binary/version");
	};
}