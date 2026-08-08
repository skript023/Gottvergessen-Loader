#pragma once
#include "common.hpp"
#include "file_manager.hpp"
#include "api/http_request.hpp"
#include "api/url_encryption.hpp"
#include "api/user/user_authentication.hpp"
#include "api/environment.hpp"

namespace gottvergessen
{
	struct VersionInfo
	{
		int m_id{};
		std::string m_game{};
		unsigned int m_code{};
		std::string m_file{};
		std::string m_target{};
		std::string m_version{};
		int m_version_machine{};
		BOOL m_supported{ FALSE };
		BOOL m_valid{ FALSE };
	};

	struct LoaderVersion
	{
		std::string m_path{};
		std::string m_version{};
		int m_version_machine{};
		bool m_supported{ false };
		bool m_valid{ false };
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
				cpr::Url url = environment_manager::get().get_url("/binary/version");
				cpr::Header header = { { xorstr("Accept"), xorstr("application/json")} };
				auto res = cpr::Get(url, header);

				LOG(INFO) << res.text;
				
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

			std::string token = std::format("Bearer {}", user_authentication::get_token());

			try
			{
				cpr::Body body = json.dump();
				cpr::Header header = { 
					{ xorstr("Accept"), xorstr("application/json") }, 
					{ xorstr("Content-Type"), xorstr("application/json") },
					{ xorstr("Authorization"), token },
				};

				auto res = cpr::Post(cpr::Url{get_version_url()}, body, header);

				LOG(INFO) << res.text;

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
			auto folder = file_manager::get_project_folder("./Binary");
			auto base_dir = folder.get_file("./version.json").get_path();

			if (std::filesystem::exists(base_dir)) return true;

			nlohmann::ordered_json json = {
				{ xorstr("name"), m_selected_binary }
			};

			std::string token = std::format("Bearer {}", user_authentication::get_token());

			std::ofstream file(base_dir, std::ios::out | std::ios::trunc);

			try
			{
				cpr::Body body = json.dump();
				cpr::Header header = {
					{ xorstr("Accept"), xorstr("application/json") },
					{ xorstr("Content-Type"), xorstr("application/json") },
					{ xorstr("Authorization"), token },
				};

				auto res = cpr::Post(cpr::Url{get_version_url()}, body, header);

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
			auto folder = file_manager::get_project_folder(xorstr("./Binary"));
			auto base_dir = folder.get_file(xorstr("./version.json")).get_path();

			nlohmann::ordered_json json = {
				{ xorstr("name"), m_selected_binary}
			};

			std::string token = std::format("Bearer {}", user_authentication::get_token());

			std::ofstream file(base_dir, std::ios::out | std::ios::trunc);

			try
			{
				cpr::Body body = json.dump();
				cpr::Header header = {
					{xorstr("Accept"), xorstr("application/json")},
					{xorstr("Content-Type"), xorstr("application/json")},
					{xorstr("Authorization"), token},
				};

				auto res = cpr::Post(cpr::Url{get_version_url()}, body, header);

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
			auto folder = file_manager::get_project_folder(xorstr("./Binary"));
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
		std::string get_version_url() const { return environment_manager::get().get_url("/binary/version"); }
	};
}