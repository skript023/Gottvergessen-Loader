#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "file_manager.hpp"

namespace gottvergessen
{
	enum class Environment
	{
		LOCAL = 0,
		PRODUCTION = 1,
		CUSTOM = 2
	};

	struct EnvironmentInfo
	{
		Environment type;
		std::string name;
		std::string base_url;
	};

	class environment_manager
	{
	public:
		static environment_manager& get()
		{
			static environment_manager instance;
			return instance;
		}

		environment_manager()
		{
			m_environments = {
				{ Environment::LOCAL, "Localhost (localhost:8180)", "http://localhost:8180" },
				{ Environment::PRODUCTION, "Production (apie.rena.my.id)", "https://apie.rena.my.id" },
				{ Environment::CUSTOM, "Custom Server", "http://localhost:8180" }
			};
			load_config();
		}

		static constexpr bool is_dev_build()
		{
#if defined(_DEBUG) || defined(DEV_BUILD)
			return true;
#else
			return false;
#endif
		}

		// Static Facade API -> Delegasi ke *_impl()
		static std::string get_url(const std::string& endpoint) { return get().get_url_impl(endpoint); }
		static std::string get_base_url() { return get().get_base_url_impl(); }
		static Environment get_current_environment() { return get().get_current_environment_impl(); }
		static void set_environment(Environment env) { get().set_environment_impl(env); }
		static std::string get_custom_url() { return get().get_custom_url_impl(); }
		static void set_custom_url(const std::string& url) { get().set_custom_url_impl(url); }

		Environment get_current_environment_impl() const
		{
			if constexpr (!is_dev_build())
				return Environment::PRODUCTION;
			return m_current_env;
		}

		void set_environment_impl(Environment env)
		{
			if constexpr (!is_dev_build())
				return;
			m_current_env = env;
			save_config();
		}

		std::string get_base_url_impl() const
		{
			if constexpr (!is_dev_build())
			{
				return "https://apie.rena.my.id";
			}

			switch (m_current_env)
			{
			case Environment::LOCAL:
				return "http://localhost:8180";
			case Environment::PRODUCTION:
				return "https://apie.rena.my.id";
			case Environment::CUSTOM:
				return m_custom_url.empty() ? "http://localhost:8180" : m_custom_url;
			default:
				return "https://apie.rena.my.id";
			}
		}

		void set_custom_url_impl(const std::string& url)
		{
			if constexpr (!is_dev_build())
				return;
			m_custom_url = url;
			if (m_current_env == Environment::CUSTOM)
				save_config();
		}

		std::string get_custom_url_impl() const
		{
			return m_custom_url;
		}

		std::string get_url_impl(const std::string& endpoint) const
		{
			std::string base = get_base_url_impl();
			while (!base.empty() && base.back() == '/')
				base.pop_back();

			if (endpoint.empty())
				return base;

			if (endpoint.front() == '/')
				return base + endpoint;
			else
				return base + "/" + endpoint;
		}

		const std::vector<EnvironmentInfo>& get_environments() const
		{
			return m_environments;
		}

		void load_config()
		{
			try
			{
				auto folder = file_manager::get_project_folder("./Config");
				auto file_path = folder.get_file("./environment.json").get_path();

				// In production/release builds, delete environment.json if it exists to eliminate risk
				if constexpr (!is_dev_build())
				{
					if (std::filesystem::exists(file_path))
					{
						std::filesystem::remove(file_path);
					}
					return;
				}

				if (std::filesystem::exists(file_path))
				{
					std::ifstream f(file_path);
					nlohmann::json j = nlohmann::json::parse(f, nullptr, false);
					if (!j.is_discarded())
					{
						if (j.contains("env_type") && j["env_type"].is_number_integer())
						{
							int env_int = j["env_type"].get<int>();
							if (env_int >= 0 && env_int <= 2)
								m_current_env = static_cast<Environment>(env_int);
						}
						if (j.contains("custom_url") && j["custom_url"].is_string())
						{
							m_custom_url = j["custom_url"].get<std::string>();
						}
					}
				}
			}
			catch (...) {}
		}

		void save_config()
		{
			if constexpr (!is_dev_build())
				return;

			try
			{
				auto folder = file_manager::get_project_folder("./Config");
				auto file_path = folder.get_file("./environment.json").get_path();
				nlohmann::json j = {
					{"env_type", static_cast<int>(m_current_env)},
					{"custom_url", m_custom_url}
				};
				std::ofstream f(file_path, std::ios::trunc);
				f << j.dump(4);
			}
			catch (...) {}
		}

	private:
		Environment m_current_env{ Environment::PRODUCTION };
		std::string m_custom_url{ "http://localhost:8180" };
		std::vector<EnvironmentInfo> m_environments;
	};
}

#endif // ENVIRONMENT_HPP
