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
		void select_binary(const std::string name) { m_selected_binary = name; }
		[[nodiscard]] std::string selected_binary() const { return m_selected_binary; }
		[[nodiscard]] std::string get_binary_name() const { return m_filename; }
		[[nodiscard]] std::string injection_target() const { return m_target_process; }
		void set_binary_data(const std::string data) { m_binary_data = {data.begin(), data.end()}; }
		[[nodiscard]] std::string binary_data() const { return m_binary_data; }
		nlohmann::ordered_json load_binaries() const { return m_binaries; }
		size_t binaries_size() const { return m_binaries.size(); }
		std::string get_binary_by_id(int id) const 
		{ 
			int index = 0;
			for (auto it = m_binaries.begin(); it != m_binaries.end(); ++it)
			{
				if (auto& data = it.value(); index == id)
				{
					return data["game"].get<std::string>();
				}
				index++;
			}

			return {};
		}
		std::string get_file_by_id(int id) const 
		{ 
			int index = 0;
			for (auto& bin : m_binaries.items())
			{
				if (auto& data = bin.value(); index == id)
				{
					return data["file"].get<std::string>();
				}
				index++;
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
		nlohmann::ordered_json m_binaries{};
		LoaderVersion m_loader_version;
		folder m_location;
		std::string m_binary_data;
		const cpr::Url url = xorstr("https://gottvergessen.000webhostapp.com/api/v1/binary/shellcode");
	};

	inline download_binary* g_download_binary;
}