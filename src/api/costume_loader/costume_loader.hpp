#pragma once
#include "common.hpp"
#include "thread_pool.hpp"
#include "api/http_request.hpp"
#include "api/url_encryption.hpp"

namespace gottvergessen
{
	class costume_loader
	{
	public:
		explicit costume_loader();

		~costume_loader();

		costume_loader(costume_loader const& that) = delete;
		costume_loader& operator=(costume_loader const& that) = delete;
		costume_loader(costume_loader&& that) = delete;
		costume_loader& operator=(costume_loader&& that) = delete;
		void execute();
		bool download(const std::string filename);
	private:
		const cpr::Url url = xorstr("https://gottvergessen.000webhostapp.com/api/v1/costume");
		const std::string extension = xorstr(".json");
	};

	inline costume_loader* g_costume_loader;
}