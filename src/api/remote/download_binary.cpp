#include "http_client/http_client.hpp"
#include "api/remote/download_binary.hpp"
#include "gui.hpp"
#include "ui/ui.hpp"
#include "api/user/user_authentication.hpp"

namespace gottvergessen
{
	download_binary::download_binary() = default;

	void download_binary::init_impl()
	{
		m_loader_version = get_loader_version();
		try
		{
			m_location = file_manager::get_project_folder("./Binary");
		}
		catch (...) {}
		generate_binaries_impl();
	}

	void download_binary::destroy_impl()
	{
	}

	bool download_binary::check_binary_before_injection_impl()
	{
		std::string uuid = this->get_selected_uuid();
		std::string file_name = this->get_binary_name();
		if (file_name.empty()) file_name = "binary_package.dll";

		auto location = m_location.get_file(file_name).get_path();

		LOG(HACKER) << "Checking binary from Server for payload: " << file_name << " (UUID: " << uuid << ")";

		std::ifstream fileStream(location, std::ios::binary | std::ios::ate);
		std::streamoff file_size = -1;
		if (fileStream.is_open())
		{
			file_size = static_cast<std::streamoff>(fileStream.tellg());
			fileStream.close();
		}

		std::string download_target = uuid.empty() ? file_name : uuid;

		if (!std::filesystem::exists(location) || file_size < 0x1000)
		{
			LOG(HACKER) << "Downloading binary payload from server (" << download_target << ")...";

			if (!this->download_impl(download_target, location))
			{
				LOG(WARNING) << "Failed to download binary from Ellohim-Server.";
				return false;
			}

			LOG(HACKER) << "New binary payload downloaded successfully: " << file_name;
		}

		return true;
	}

	bool download_binary::validate_before_injection_impl()
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

		if (!this->generate_impl(this->get_binary_name()))
		{
			LOG(WARNING) << "Host did not return valid version data, does it have a version.json?";
			return false;
		}

		LOG(HACKER) << "New DLL has been generated from server, the DLL version is " << m_current_version.m_version;
		return true;
	}

	bool download_binary::download_impl(const std::string filename, const std::filesystem::path& location)
	{
		std::filesystem::path target_path = location.empty() ? m_location.get_file(filename).get_path() : location;
		std::ofstream file(target_path, std::ios::binary | std::ios::trunc);

		std::string token = std::format("Bearer {}", user_authentication::get_token());

		try
		{
			cpr::Header header { 
				{ xorstr("Content-Type"), xorstr("application/json") }, 
				{ xorstr("Authorization"), token }
			};

			cpr::Url download_url = environment_manager::get().get_url("/binary/download/") + filename;

			auto ok = http_client::download_with_progress(download_url, target_path, header, cpr::Parameters{}, [&](float progress)
			{
				LOG(INFO) << "Progress: " << static_cast<int>(progress) << "%";
				ui::get().m_download_progress = progress / 100.0f;
			});

			if (!ok)
			{
				LOG(WARNING) << "Failed to download bin";
				return false;
			}
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

	bool download_binary::generate_impl(const std::string filename)
	{
		nlohmann::ordered_json json = {
			{ xorstr("name"), filename }
		};

		std::string token = std::format("Bearer {}", user_authentication::get_token());

		try
		{
			cpr::Body body = json.dump();
			cpr::Header header{
				{ xorstr("Content-Type"), xorstr("application/json") },
				{ xorstr("Authorization"), token }
			};

			auto res = cpr::Post(cpr::Url{get_binary_url()}, body, header);
			set_binary_data(res.text);
		}
		catch (const std::exception&)
		{
			LOG(WARNING) << "Failed to download binary, is the host down?";
			return false;
		}

		return true;
	}

	bool download_binary::generate_binaries_impl()
	{
		std::string token = std::format("Bearer {}", user_authentication::get_token());

		try
		{
			cpr::Header header{
				{ xorstr("Content-Type"), xorstr("application/json") },
				{ xorstr("Authorization"), token }
			};

			cpr::Url url = environment_manager::get().get_url("/binary/my-binaries");

			auto res = cpr::Get(url, header);

			auto parsed = nlohmann::ordered_json::parse(res.text, nullptr, false);
			if (!parsed.is_discarded())
			{
				if (parsed.contains("data") && parsed["data"].is_array())
				{
					this->m_binaries = parsed["data"];
				}
				else
				{
					this->m_binaries = parsed;
				}
				LOG(INFO) << "Loaded " << this->m_binaries.size() << " user binaries from GET /binary/my-binaries";
			}
		}
		catch (const std::exception&)
		{
			LOG(WARNING) << "Failed to fetch binary catalog from Ellohim-Server";
			return false;
		}

		return true;
	}

	bool download_binary::integrate_user_impl()
	{
		nlohmann::ordered_json json = {
			{ xorstr("username"), user_authentication::get_username() },
			{ xorstr("role"), user_authentication::get_role() },
			{ xorstr("token"), user_authentication::get_token() }
		};

		try
		{
			cpr::Url url = environment_manager::get().get_url("/binary");
			cpr::Body body = json.dump();
			cpr::Header header {
				{ xorstr("Content-Type"), xorstr("application/json") },
				{ xorstr("User-Agent"), user_authentication::get_user_agent() }
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