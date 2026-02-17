#include "http_client.hpp"

namespace gottvergessen
{
	http_client::http_client() :
	    m_proxy_mgr(m_session)
	{
		m_session.SetConnectTimeout(CONNECT_TIMEOUT);
		m_session.SetTimeout(REQUEST_TIMEOUT);
	}

	bool http_client::download_impl(const cpr::Url& url, const std::filesystem::path& path, cpr::Header headers, cpr::Parameters query_params)
	{
		m_session.SetUrl(url);
		m_session.SetHeader(headers);
		m_session.SetParameters(query_params);

		std::ofstream of(path, std::ios::binary);
		auto res = m_session.Download(of);

		return res.status_code == 200;
	}

    bool http_client::download_with_progress_impl(cpr::Url const& url, std::filesystem::path const& path, cpr::Header headers, cpr::Parameters query_params, std::function<void(float)> progress_cb)
    {
        m_session.SetUrl(url);
		m_session.SetHeader(headers);
		m_session.SetParameters(query_params);

		std::ofstream of(path, std::ios::binary | std::ios::trunc);
		if (!of.is_open())
			return false;

		auto* cb_ptr = new std::function<void(float)>(std::move(progress_cb));

		m_session.SetProgressCallback(
        cpr::ProgressCallback(
            [](cpr::cpr_off_t dltotal,
               cpr::cpr_off_t dlnow,
               cpr::cpr_off_t /*ultotal*/,
               cpr::cpr_off_t /*ulnow*/,
               intptr_t userdata) -> bool
            {
                auto* func = reinterpret_cast<std::function<void(float)>*>(userdata);

                if (func && dltotal > 0)
                {
                    float percent = (float(dlnow) / float(dltotal)) * 100.f;
                    (*func)(percent);
                }

                return true; // return false kalau mau cancel
            },
            reinterpret_cast<intptr_t>(cb_ptr)));

		auto res = m_session.Download(of);

		m_session.SetProgressCallback(cpr::ProgressCallback()); // penting, reset biar gak nempel

		of.close();

		return res.status_code == 200;
    }

    cpr::Response http_client::get_impl(const cpr::Url& url, cpr::Header headers, cpr::Parameters query_params)
	{
		m_session.SetUrl(url);
		m_session.SetHeader(headers);
		m_session.SetParameters(query_params);

		return m_session.Get();
	}

	cpr::Response http_client::post_impl(const cpr::Url& url, cpr::Header headers, cpr::Body body)
	{
		m_session.SetUrl(url);
		m_session.SetHeader(headers);
		m_session.SetBody(body);

		return m_session.Post();
	}

	bool http_client::init_impl(file proxy_settings_file)
	{
		return m_proxy_mgr.load(proxy_settings_file);
	}
}