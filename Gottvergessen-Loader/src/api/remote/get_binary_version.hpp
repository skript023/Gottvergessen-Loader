#pragma once
#include "common.hpp"
#include "file_manager.hpp"
#include "api/http_request.hpp"
#include "api/url_encryption.hpp"

namespace gottvergessen
{
	struct VersionInfo
	{
		const std::string m_path;
		const std::string m_version;
		const int m_version_machine;
		const bool m_supported;
		const bool m_valid;
	};

	class get_version
	{
	public:
		get_version() = default;
		virtual ~get_version() noexcept = default;

		get_version(get_version const& that) = delete;
		get_version& operator=(get_version const& that) = delete;
		get_version(get_version&& that) = delete;
		get_version& operator=(get_version&& that) = delete;

		VersionInfo get_version_info()
		{
			nlohmann::ordered_json body = {
				{ xorstr("name"), "gottvergessen"}
			};

			std::string content_type = xorstr("Content-Type: application/json");

			try
			{
				http::Request req(url);
				http::Response res = req.send("POST", body.dump(), { content_type });

				nlohmann::json j = nlohmann::json::parse(res.body.begin(), res.body.end());

				return { j["file"], j["version"], j["version_machine"], j["supported"], true};
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
				{ xorstr("name"), "gottvergessen"}
			};

			std::string content_type = xorstr("Content-Type: application/json");

			std::ofstream file(base_dir, std::ios::out | std::ios::trunc);

			try
			{
				http::Request req(url);
				http::Response res = req.send("POST", body.dump(), { content_type });

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
			auto folder = g_file_manager->get_project_folder("./Binary");
			auto base_dir = folder.get_file("./version.json").get_path();

			nlohmann::ordered_json body = {
				{ xorstr("name"), "gottvergessen"}
			};

			std::string content_type = xorstr("Content-Type: application/json");

			std::ofstream file(base_dir, std::ios::out | std::ios::trunc);

			try
			{
				http::Request req(url);
				http::Response res = req.send("POST", body.dump(), { content_type });

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
			auto folder = g_file_manager->get_project_folder("./Binary");
			auto base_dir = folder.get_file("./version.json").get_path();

			std::ifstream file(base_dir);

			nlohmann::json json_file;

			if (file.fail())
			{
				LOG(HACKER) << "File doesn't exist";

				this->ensure_version_file();

				LOG(HACKER) << "Download version from server";
			}

			if (!file.is_open())
			{
				file.open(base_dir);
			}

			file >> json_file;

			auto& j = json_file;

			file.close();

			return { j["file"], j["version"], j["version_machine"], j["supported"], j["valid"]};
		}
		VersionInfo m_version_info{};
	private:
		const std::string url_test = xorstr("http://localhost:8000/api/v1/binary/version");
		const std::string url = xorstr("http://gottvergessen.000webhostapp.com/api/v1/binary/version");
		const std::string extension = xorstr(".json");
	};
}