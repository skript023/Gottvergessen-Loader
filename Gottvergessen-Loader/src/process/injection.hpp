#pragma once
#include "common.hpp"
#include "injection_method.hpp"

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

	class injection : public injection_method
	{
	public:
		explicit injection(const std::filesystem::path& dllFile);

		~injection() noexcept;

		injection(injection const& that) = delete;
		injection& operator=(injection const& that) = delete;
		injection(injection&& that) = delete;
		injection& operator=(injection&& that) = delete;

		bool create_remote_thread(std::string file_name) override;
		bool thread_execution_hijacking(std::string file_name) override;
		bool inject_library();
		eValidType validate_file(const std::filesystem::path& dllFile);
	private:
		std::filesystem::path filename;
	};

	inline injection* g_injection;
}