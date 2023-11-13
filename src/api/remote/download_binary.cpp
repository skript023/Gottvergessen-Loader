#include "download_binary.hpp"
#include "gui.hpp"

namespace gottvergessen
{
	download_binary::download_binary(const folder& location) : m_location(location), m_loader_version(get_loader_version())
	{
		this->generate_binaries();
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

		if (!m_current_version.m_supported || !m_latest_version.m_supported)
		{
			LOG(WARNING) << "This version is unsupported, injection terminated";
			return false;
		}

		if (m_current_version.m_id != m_latest_version.m_id)
		{
			LOG(WARNING) << "Invalid category, redownload new version file";
			this->download_version_file();
			this->check_binary_before_injection();
			LOG(HACKER) << "New version file downloaded successfully";
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

	bool download_binary::validate_before_injection()
	{
		auto m_current_version = this->get_version_info();

		LOG(HACKER) << "Server binary version is " << m_current_version.m_version;

		if (!m_current_version.m_supported)
		{
			LOG(WARNING) << "This version is unsupported, injection terminated";
			return false;
		}

		if (!m_current_version.m_valid)
		{
			LOG(WARNING) << "Host did not return valid version data, does it have a version.json?";

			return false;
		}

		if (!this->generate(this->get_binary_name()))
		{
			LOG(WARNING) << "Host did not return valid version data, does it have a version.json?";

			return false;
		}

		LOG(HACKER) << "New DLL has been generated from server, the DLL version is " << m_current_version.m_version;

		return true;
	}


	bool download_binary::download(const std::string filename, const std::filesystem::path& location) const
	{
		std::ofstream file(location, std::ios::binary | std::ios::trunc);

		nlohmann::ordered_json json = {
			{ xorstr("name"), filename }
		};

		std::string token = std::format("Bearer {}", g_user_authentication->get_token());

		try
		{
			cpr::Body body = json.dump();
			cpr::Header header { 
				{ xorstr("Content-Type"), xorstr("application/json") }, 
				{ xorstr("Authorization"), token }
			};

			auto res = cpr::Post(url, body, header);

			std::ostream_iterator<std::uint8_t> output(file);
			cpr::Download(file, res.url);
			//std::ranges::copy(res.text.begin(), res.text.end(), output);
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

	bool download_binary::generate(const std::string filename)
	{
		nlohmann::ordered_json json = {
			{ xorstr("name"), filename }
		};

		std::string token = std::format("Bearer {}", g_user_authentication->get_token());

		try
		{
			cpr::Body body = json.dump();
			cpr::Header header{
				{ xorstr("Content-Type"), xorstr("application/json") },
				{ xorstr("Authorization"), token }
			};

			auto res = cpr::Post(url, body, header);

			set_binary_data(res.text);
		}
		catch (const std::exception&)
		{
			LOG(WARNING) << "Failed to download binary, is the host down?";

			return false;
		}

		return true;
	}

	bool download_binary::generate_binaries()
	{
		std::string token = std::format("Bearer {}", g_user_authentication->get_token());

		try
		{
			cpr::Header header{
				{ xorstr("Content-Type"), xorstr("application/json") },
				{ xorstr("Authorization"), token }
			};

			cpr::Url url = xorstr("https://gottvergessen.000webhostapp.com/api/v1/binary/all");

			auto res = cpr::Get(url, header);

			this->m_binaries = nlohmann::ordered_json::parse(res.text);
			auto& data = this->m_binaries.begin().value();
		}
		catch (const std::exception&)
		{
			LOG(WARNING) << "Failed to download binary, is the host down?";

			return false;
		}

		return true;
	}
	bool download_binary::integrate_user()
	{
		nlohmann::ordered_json json = {
			{ xorstr("username"), g_user_authentication->get_username() },
			{ xorstr("hardware_uuid"), g_user_authentication->get_bios() },
			{ xorstr("computer_name"), g_user_authentication->get_computer_name() },
			{ xorstr("role"), g_user_authentication->get_role() },
			{ xorstr("token"), g_user_authentication->get_token() }
		};

		try
		{
			cpr::Url url = xorstr("http://localhost:8000/api/v1/injection/grants-access");
			cpr::Body body = json.dump();
			cpr::Header header {
				{ xorstr("Content-Type"), xorstr("application/json") },
			};

			auto res = cpr::Post(url, body, header);
			if (res.status_code != 200) return false;

			auto json = nlohmann::json::parse(res.text);
			LOG(HACKER) << json["message"];
		}
		catch (const std::exception&)
		{
			LOG(WARNING) << "Failed to download binary, is the host down?";

			return false;
		}

		return true;
	}
}