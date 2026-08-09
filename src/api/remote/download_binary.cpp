#include "http_client/http_client.hpp"
#include "api/remote/download_binary.hpp"
#include "gui.hpp"
#include "ui/ui.hpp"
#include "api/encrypted_downloader.hpp"
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
		std::string download_target = uuid.empty() ? file_name : uuid;

		LOG(HACKER) << "Checking encrypted binary payload (.enc) on disk for: " << file_name << " (UUID: " << uuid << ")";

		std::ifstream fileStream(location, std::ios::binary | std::ios::ate);
		std::streamoff file_size = -1;
		if (fileStream.is_open())
		{
			file_size = static_cast<std::streamoff>(fileStream.tellg());
			fileStream.close();
		}

		if (!std::filesystem::exists(location) || file_size < 0x1000)
		{
			LOG(HACKER) << "Encrypted binary payload missing on disk. Downloading from server (" << download_target << ")...";

			if (!this->download_impl(download_target, location))
			{
				LOG(WARNING) << "Failed to download encrypted binary from Ellohim-Server.";
				return false;
			}
		}

		LOG(HACKER) << "Encrypted binary payload verified on disk: " << file_name;
		return true;
	}

	bool download_binary::prepare_temp_decrypted_binary_impl(std::filesystem::path& out_temp_path)
	{
		std::string uuid = this->get_selected_uuid();
		std::string file_name = this->get_binary_name();
		if (file_name.empty()) file_name = "binary_package.dll";

		auto location = m_location.get_file(file_name).get_path();
		std::string download_target = uuid.empty() ? file_name : uuid;

		std::string baseUrl = environment_manager::get_base_url();
		std::string token = user_authentication::get_token();

		LOG(HACKER) << "Decrypting cached binary payload to temporary file before injection...";

		if (!encrypted_downloader::decrypt_file_to_temp(baseUrl, download_target, token, location, out_temp_path))
		{
			LOG(WARNING) << "Failed to decrypt binary payload from disk (file may be corrupted or using legacy format). Re-downloading fresh payload...";
			
			std::error_code ec;
			std::filesystem::remove(location, ec);

			if (this->download_impl(download_target, location))
			{
				if (encrypted_downloader::decrypt_file_to_temp(baseUrl, download_target, token, location, out_temp_path))
				{
					return true;
				}
			}

			LOG(WARNING) << "Failed to decrypt binary payload to temp path!";
			return false;
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
		std::filesystem::path target_path;
		if (location.empty())
		{
			target_path = m_location.get_file(filename).get_path();
		}
		else if (location.is_absolute())
		{
			target_path = location;
		}
		else
		{
			target_path = m_location.get_file(location).get_path();
		}

		std::string binary_id = filename;
		std::string baseUrl = environment_manager::get_base_url();
		std::string token = user_authentication::get_token();

		LOG(HACKER) << "Downloading raw encrypted binary payload for ID: " << binary_id << " to " << target_path.string();

		bool ok = encrypted_downloader::download_encrypted_to_file(baseUrl, binary_id, token, target_path, [](float progress) {
			LOG(INFO) << "Progress: " << static_cast<int>(progress * 100.0f) << "%";
			ui::get().m_download_progress = progress;
		});

		if (!ok)
		{
			LOG(WARNING) << "Failed to download encrypted binary payload from Ellohim-Server.";
			return false;
		}

		LOG(HACKER) << "Encrypted binary payload saved to disk: " << target_path.string();
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
			set_binary_data_impl(res.text);
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

			cpr::Url url = environment_manager::get_url("/binary/my-binaries");

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
			cpr::Url url = environment_manager::get_url("/binary");
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

	bool download_binary::download_encrypted_impl(const std::string& binary_id, std::vector<uint8_t>& out_bytes, std::function<void(float)> progress_cb)
	{
		std::string baseUrl = environment_manager::get_base_url();
		std::string token = user_authentication::get_token();
		return encrypted_downloader::download_and_decrypt_to_memory(baseUrl, binary_id, token, out_bytes, progress_cb);
	}
}