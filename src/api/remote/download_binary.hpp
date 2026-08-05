#include "common.hpp"
#include "file_manager.hpp"
#include "api/http_request.hpp"
#include "api/url_encryption.hpp"
#include "api/remote/get_binary_version.hpp"

namespace gottvergessen
{
	class download_binary : private get_version
	{
	public:
		explicit download_binary(const folder& location);

		~download_binary();

		download_binary(download_binary const& that) = delete;
		download_binary& operator=(download_binary const& that) = delete;
		download_binary(download_binary&& that) = delete;
		download_binary& operator=(download_binary&& that) = delete;

		bool check_binary_before_injection();
		bool validate_before_injection();
		bool download(const std::string filename, const std::filesystem::path& location) const;
		bool generate(const std::string filename);
		bool generate_binaries();
		bool integrate_user();
		bool is_version_valid() const { return m_loader_version.m_valid; }
		[[nodiscard]] int loader_version_machine() const { return m_loader_version.m_version_machine; }
		[[nodiscard]] std::string loader_version() const { return m_loader_version.m_version; }
		void select_binary_index(int index) { m_selected_index = index; }
		[[nodiscard]] int selected_index() const { return m_selected_index; }
		void select_binary(const std::string name) { m_selected_binary = name; }
		[[nodiscard]] std::string selected_binary() const { return m_selected_binary; }
		[[nodiscard]] std::string get_selected_uuid() const { return get_uuid_by_id(m_selected_index); }
		[[nodiscard]] std::string get_selected_file_name() const 
		{ 
			std::string fn = get_file_by_id(m_selected_index); 
			return fn.empty() ? m_filename : fn; 
		}
		[[nodiscard]] std::string get_binary_name() const 
		{ 
			std::string fn = get_file_by_id(m_selected_index);
			return fn.empty() ? (m_filename.empty() ? "binary.dll" : m_filename) : fn; 
		}
		void set_target_process(const std::string& process_name) { m_target_process = process_name; }
		[[nodiscard]] std::string injection_target() const { return m_target_process.empty() ? "notepad.exe" : m_target_process; }
		void set_binary_data(const std::string data) { m_binary_data = {data.begin(), data.end()}; }
		[[nodiscard]] std::string binary_data() const { return m_binary_data; }
		nlohmann::ordered_json load_binaries() const { return m_binaries; }
		size_t binaries_size() const 
		{ 
			if (m_binaries.is_array()) return m_binaries.size();
			if (m_binaries.is_object()) return m_binaries.size();
			return 0;
		}
		std::string get_uuid_by_id(int id) const 
		{ 
			if (m_binaries.is_array() && id >= 0 && id < (int)m_binaries.size())
			{
				auto& data = m_binaries[id];
				if (data.contains("id") && data["id"].is_string()) return data["id"].get<std::string>();
			}
			else if (m_binaries.is_object())
			{
				int index = 0;
				for (auto it = m_binaries.begin(); it != m_binaries.end(); ++it)
				{
					if (index == id)
					{
						auto& data = it.value();
						if (data.contains("id") && data["id"].is_string()) return data["id"].get<std::string>();
					}
					index++;
				}
			}
			return {};
		}
		std::string get_binary_by_id(int id) const 
		{ 
			if (m_binaries.is_array() && id >= 0 && id < (int)m_binaries.size())
			{
				auto& data = m_binaries[id];
				if (data.contains("name") && data["name"].is_string()) return data["name"].get<std::string>();
				if (data.contains("game") && data["game"].is_string()) return data["game"].get<std::string>();
			}
			else if (m_binaries.is_object())
			{
				int index = 0;
				for (auto it = m_binaries.begin(); it != m_binaries.end(); ++it)
				{
					if (index == id)
					{
						auto& data = it.value();
						if (data.contains("name") && data["name"].is_string()) return data["name"].get<std::string>();
						if (data.contains("game") && data["game"].is_string()) return data["game"].get<std::string>();
					}
					index++;
				}
			}

			return {};
		}
		std::string get_file_by_id(int id) const 
		{ 
			if (m_binaries.is_array() && id >= 0 && id < (int)m_binaries.size())
			{
				auto& data = m_binaries[id];
				if (data.contains("file_name") && data["file_name"].is_string()) return data["file_name"].get<std::string>();
				if (data.contains("file") && data["file"].is_string()) return data["file"].get<std::string>();
			}
			else if (m_binaries.is_object())
			{
				int index = 0;
				for (auto& bin : m_binaries.items())
				{
					if (index == id)
					{
						auto& data = bin.value();
						if (data.contains("file_name") && data["file_name"].is_string()) return data["file_name"].get<std::string>();
						if (data.contains("file") && data["file"].is_string()) return data["file"].get<std::string>();
					}
					index++;
				}
			}

			return {};
		}

		template <class InIterator, class OutIterator>
		void copy(InIterator begin, InIterator end, OutIterator result)
		{
			int i = 0;
			for (InIterator it = begin; it != end; ++it)
			{
				LOG(HACKER) << "Progress : " << i - end << "%";
				*result++ = *it; i++;
			}
		}
		BinaryName m_binary_name[4] = {
			{ xorstr("GTA V Mod Menu"), xorstr("gta") },
			{ xorstr("Scarlet Nexus"), xorstr("scarlet-nexus") },
			{ xorstr("Tower of Fantasy"), xorstr("tower-of-fantasy") },
			{ xorstr("Elsword Zero"), xorstr("ElsZero") }
		};
	private:
		int m_selected_index{0};
		nlohmann::ordered_json m_binaries{};
		LoaderVersion m_loader_version;
		folder m_location;
		std::string m_binary_data;
		const cpr::Url url = xorstr("http://localhost:8180/binary");
	};

	inline download_binary* g_download_binary;
}