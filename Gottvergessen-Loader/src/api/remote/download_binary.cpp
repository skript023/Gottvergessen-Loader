#include "download_binary.hpp"
#include "gui.hpp"

namespace gottvergessen
{
	download_binary::download_binary(const folder& location) : m_location(location), m_loader_version(get_loader_version())
	{
		g_download_binary = this;
	}

	download_binary::~download_binary()
	{
		g_download_binary = nullptr;
	}

	bool download_binary::check_binary_before_injection()
	{
		auto m_latest_version = this->get_version_info();
		auto m_current_version = this->get_current_version();

		LOG(HACKER) << "Server binary version is " << m_latest_version.m_version << " current version is " << m_current_version.m_version;

		if (m_current_version.m_id != m_latest_version.m_id)
		{
			this->download_version_file();
		}

		if (!m_latest_version.m_valid)
		{
			LOG(WARNING) << "Host did not return valid version data, does it have a version.json?";

			return false;
		}

		LOG(HACKER) << "Checking binary From Server";

		auto location = m_location.get_file(this->get_binary_name()).get_path();

		std::ifstream fileStream(location, std::ios::binary | std::ios::ate);

		const auto file_size = fileStream.tellg();
		if (std::filesystem::exists(location) && file_size < 0x1000)
		{
			fileStream.close();

			LOG(WARNING) << "DLL file seems inconceivably small probably file corrupted, request to inject ignored.";

			LOG(HACKER) << "Redownloading binary from server, please wait...";

			if (!this->download(this->get_binary_name(), location))
			{
				LOG(WARNING) << "Host did not return valid version data, does it have a version.json?";

				return false;
			}

			LOG(HACKER) << "New DLL has been downloaded from remote, new binary version is " << m_latest_version.m_version;
		}

		if (!std::filesystem::exists(location))
		{
			LOG(HACKER) << "Downloading DLL from server, please wait...";
			if (!this->download(this->get_binary_name(), location))
			{
				LOG(WARNING) << "Host did not return valid version data, does it have a version.json?";

				return false;
			}

			LOG(HACKER) << "New DLL has been downloaded from remote, new binary version is " << m_latest_version.m_version;
		}

		if (m_current_version.m_version_machine < m_latest_version.m_version_machine)
		{
			LOG(HACKER) << "DLL is outdated or request remote not valid, request updating binary from server...";
			LOG(HACKER) << "Updating DLL from server, please wait...";
			if (!this->download(this->get_binary_name(), location))
			{
				LOG(WARNING) << "Host did not return valid version data, does it have a version.json?";

				return false;
			}

			this->download_version_file();

			LOG(HACKER) << "New DLL has been downloaded from remote, new binary version is " << m_latest_version.m_version;
		}

		return true;
	}

	bool download_binary::download(const std::string filename, const std::filesystem::path& location) const
	{
		std::ofstream file(location, std::ios::binary | std::ios::trunc);

		nlohmann::ordered_json body = {
			{ xorstr("name"), filename }
		};

		std::string content_type = xorstr("Content-Type: application/json");
		std::string accept = xorstr("Accept: application/json");
		std::string auth = std::format("Authorization: Bearer {}", g_user_authentication->get_token());

		try
		{
			http::Request req(url);
			http::Response res = req.send("POST", body.dump(), { auth, accept, content_type });

			std::ostream_iterator<std::uint8_t> output(file);
			std::ranges::copy(res.body.begin(), res.body.end(), output);

		}
		catch (const std::exception&)
		{
			LOG(WARNING) << "Failed to download binary, is the host down?";

			file.close();

			return false;
		}
		file.close();

		return true;
	}
}