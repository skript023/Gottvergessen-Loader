#pragma once
#include "common.hpp"
#include "file_manager.hpp"
#include "process/injection_method.hpp"
#include "process/methods/remote_thread_injection.hpp"
#include "process/methods/thread_hijack_injection.hpp"
#include "process/methods/manual_map_injection.hpp"
#include "process/methods/reflective_injection.hpp"
#include <memory>

namespace gottvergessen
{
	enum class eValidType
	{
		VALID,
		ACCESS_FAILURE,
		TOO_SMALL,
		ALLOCATION_FAILURE,
		NOT_A_DLL,
		INVALID_PLATFORM,
		NEED_UPDATE
	};

	// Process Injection Service Facade Class (Static Singleton with Strategy Pattern)
	class injection final
	{
	public:
		static injection& instance()
		{
			static injection instance_val;
			return instance_val;
		}

		static injection& get()
		{
			return instance();
		}

		// Static Facade API -> Delegasi ke *_impl()
		static void set_injection_mode(InjectionMode mode)
		{
			instance().set_injection_mode_impl(mode);
		}

		static InjectionMode get_injection_mode()
		{
			return instance().get_injection_mode_impl();
		}

		static void set_target_process(const std::string& process_name)
		{
			instance().set_target_process_impl(process_name);
		}

		static std::string get_target_process()
		{
			return instance().get_target_process_impl();
		}

		static bool validate_binary(std::filesystem::path filename)
		{
			return instance().validate_binary_impl(filename);
		}

		static bool inject_library()
		{
			return instance().inject_library_impl();
		}

		static eValidType validate_file(const std::filesystem::path& dllFile)
		{
			return instance().validate_file_impl(dllFile);
		}

		void set_location(const folder& dllFile) { m_filename = dllFile; }

	private:
		injection();
		~injection() noexcept = default;

		injection(injection const&) = delete;
		injection& operator=(injection const&) = delete;
		injection(injection&&) = delete;
		injection& operator=(injection&&) = delete;

		// Implementation Private Methods (*_impl suffix)
		void set_injection_mode_impl(InjectionMode mode);
		InjectionMode get_injection_mode_impl() const { return m_mode; }

		void set_target_process_impl(const std::string& process_name) { m_target_process = process_name; }
		std::string get_target_process_impl() const { return m_target_process; }

		bool validate_binary_impl(std::filesystem::path filename);
		bool inject_library_impl();
		eValidType validate_file_impl(const std::filesystem::path& dllFile);

		std::unique_ptr<injection_method> create_strategy(InjectionMode mode);

	private:
		folder m_filename{};
		std::string m_target_process{"notepad.exe"};
		std::uint32_t m_pid{0};
		InjectionMode m_mode{InjectionMode::CreateRemoteThread};
		std::unique_ptr<injection_method> m_strategy;
	};
}