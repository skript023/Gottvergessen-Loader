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

		bool download(const std::string filename, const std::filesystem::path& location) const;
		bool is_version_valid() const { return m_loader_version.m_valid; }
		int loader_version_machine() const { return m_loader_version.m_version_machine; }
		std::string loader_version() const { return m_loader_version.m_version; }
		void select_binary(const std::string name) { m_selected_binary = name; }
		std::string selected_binary() const { return m_selected_binary; }
		std::string get_binary_name() const { return m_filename; }
		std::string injection_target() const { return m_target_process; }

		template <class InIterator, class OutIterator>
		void write_binary(InIterator begin, InIterator end, OutIterator result)
		{
			int i = 0;
			for (InIterator it = begin; it != end; ++it)
			{
				LOG(HACKER) << "Progress : " << i % *it << "%";
				*result++ = *it; i++;
			}
		}
		BinaryName m_binary_name[3] = {
			{ xorstr("GTA V Mod Menu"), xorstr("gta") },
			{ xorstr("Scarlet Nexus"), xorstr("scarlet-nexus") },
			{ xorstr("Tower of Fantasy"), xorstr("tower-of-fantasy") }
		};
	private:
		LoaderVersion m_loader_version;
		folder m_location;
		const std::string url = xorstr("http://localhost:8000/api/v1/binary/shellcode");
	};

	inline download_binary* g_download_binary;
}