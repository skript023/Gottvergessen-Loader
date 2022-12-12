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
		const std::string m_path;
		const std::string m_target;
		const std::string m_version;
		const int m_version_machine;
		const bool m_supported;
		const bool m_valid;
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
		get_version() = default;
		virtual ~get_version() noexcept = default;

		get_version(get_version const& that) = delete;
		get_version& operator=(get_version const& that) = delete;
		get_version(get_version&& that) = delete;
		get_version& operator=(get_version&& that) = delete;

		LoaderVersion get_loader_version()
		{
			try
			{
				http::Request req(xorstr("http://localhost:8000/api/v1/version"));
				http::Response res = req.send("GET", "", { "Accept: application/json" });

				nlohmann::json j = nlohmann::json::parse(res.body.begin(), res.body.end());

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
			nlohmann::ordered_json body = {
				{ xorstr("name"), m_selected_binary}
			};

			std::string content_type = xorstr("Content-Type: application/json");
			std::string accept = xorstr("Accept: application/json");
			std::string auth = std::format("Authorization: Bearer {}", g_user_authentication->get_token());

			try
			{
				http::Request req(url);
				http::Response res = req.send("POST", body.dump(), { content_type, accept, auth });

				nlohmann::json j = nlohmann::json::parse(res.body.begin(), res.body.end());

				m_filename = j["file"];
				m_target_process = j["target"];

				return { j["id"], j["file"], j["target"], j["version"], j["version_machine"], j["supported"], true};
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

			nlohmann::ordered_json body = {
				{ xorstr("name"), m_selected_binary}
			};

			std::string content_type = xorstr("Content-Type: application/json");

			std::ofstream file(base_dir, std::ios::out | std::ios::trunc);

			try
			{
				http::Request req(url);
				http::Response res = req.send(xorstr("POST"), body.dump(), {content_type});

				nlohmann::json j = nlohmann::json::parse(res.body.begin(), res.body.end());

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

			nlohmann::ordered_json body = {
				{ xorstr("name"), m_selected_binary}
			};

			std::string content_type = xorstr("Content-Type: application/json");

			std::ofstream file(base_dir, std::ios::out | std::ios::trunc);

			try
			{
				http::Request req(url);
				http::Response res = req.send(xorstr("POST"), body.dump(), {content_type});

				nlohmann::json j = nlohmann::json::parse(res.body.begin(), res.body.end());

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

				this->ensure_version_file();

				LOG(HACKER) << xorstr("Download version from server");
			}

			if (!file.is_open())
			{
				file.open(base_dir);
			}

			file >> json_file;

			auto& j = json_file;

			for (auto it = j.begin(); it != j.end(); ++it)
			{
				if (it.key().empty())
				{
					LOG(WARNING) << "Version file contain null object, attempting to get new version file";
					this->ensure_version_file();
					get_current_version();
				}
			}

			file.close();

			m_filename = j["file"];
			m_target_process = j["target"];

			return { j["id"], j["file"], j["target"], j["version"], j["version_machine"], j["supported"], j["valid"]};
		}

		VersionInfo m_version_info{};
	private:
		std::string m_selected_binary;
		std::string m_target_process;
		std::string m_filename;
		const std::string url = xorstr("http://localhost:8000/api/v1/binary/version");
	};
}