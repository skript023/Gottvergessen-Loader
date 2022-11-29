#include "common.hpp"
#include "api/http_request.hpp"
#include "api/url_encryption.hpp"
#include "api/remote/get_binary_version.hpp"

namespace gottvergessen
{
	class download_binary : private get_version
	{
	public:
		explicit download_binary(const std::filesystem::path& location);

		~download_binary();

		download_binary(download_binary const& that) = delete;
		download_binary& operator=(download_binary const& that) = delete;
		download_binary(download_binary&& that) = delete;
		download_binary& operator=(download_binary&& that) = delete;

		bool check_binary_before_injection();

		bool download(const std::string filename, const std::filesystem::path& location);
		int get_current_version_machine() const { return m_current_version.m_version_machine; }
		int get_latest_version_machine() const { return m_latest_version.m_version_machine; }
		std::string get_current_version() const { return m_current_version.m_version; }
		std::string get_latest_version() const { return m_latest_version.m_version; }

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
	private:
		VersionInfo m_latest_version;
		VersionInfo m_current_version;
		std::filesystem::path m_location;
		const std::string url_test = xorstr("http://localhost:8000/api/v1/binary/shellcode");
		const std::string url = xorstr("http://gottvergessen.000webhostapp.com/api/v1/binary/shellcode");
		const std::string file_extension = xorstr(".vpack");
	};

	inline download_binary* g_download_binary;
}