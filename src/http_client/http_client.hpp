#pragma once
#include <cpr/cpr.h>
#include "proxy_mgr.hpp"
#include "file_manager/file.hpp"

namespace gottvergessen
{
    constexpr auto CONNECT_TIMEOUT = 1000;
    constexpr auto REQUEST_TIMEOUT = 5000;

    class http_client
    {
    private:
        cpr::Session m_session;
        proxy_mgr m_proxy_mgr;
        static http_client& get() { static http_client i{}; return i; }
    private:
        http_client();
        virtual ~http_client() = default;
        http_client(const http_client&) = delete;
        http_client(http_client&&) noexcept = delete;
        http_client& operator=(const http_client&) = delete;
        http_client& operator=(http_client&&) noexcept = delete;

        bool download_impl(const cpr::Url& url, const std::filesystem::path& path, cpr::Header headers = {}, cpr::Parameters query_params = {});
        bool download_with_progress_impl(cpr::Url const& url, std::filesystem::path const& path, cpr::Header headers, cpr::Parameters query_params, std::function<void(float)> progress_cb);
        cpr::Response get_impl(const cpr::Url& url, cpr::Header headers = {}, cpr::Parameters query_params = {});
        cpr::Response post_impl(const cpr::Url& url, cpr::Header headers = {}, cpr::Body body = {});
        
        proxy_mgr& proxy_mgr_impl()
        {
            return m_proxy_mgr;
        }

        bool init_impl(file proxy_settings_file);
    public:
        static bool download(const cpr::Url& url, const std::filesystem::path& path, cpr::Header headers = {}, cpr::Parameters query_params = {}) { return get().download_impl(url, path, headers, query_params); }
        static bool download_with_progress(const cpr::Url& url, const std::filesystem::path& path, cpr::Header headers = {}, cpr::Parameters query_params = {}, std::function<void(float)> progress_cb = {}) { return get().download_with_progress_impl(url, path, headers, query_params, std::move(progress_cb)); }
        static cpr::Response get(const cpr::Url& url, cpr::Header headers = {}, cpr::Parameters query_params = {}) { return get().get_impl(url, headers, query_params); }
        static cpr::Response post(const cpr::Url& url, cpr::Header headers = {}, cpr::Body body = {}) { return get().post_impl(url, headers, body); }

        static proxy_mgr& proxy_mgr()
        {
            return get().proxy_mgr_impl();
        }

        static bool init(file proxy_settings_file) { return get().init_impl(proxy_settings_file); }
    };
}