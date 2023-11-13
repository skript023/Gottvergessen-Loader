#pragma once
#include "common.hpp"
#include "injection_method.hpp"
#include "file_manager.hpp"

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

	class injection final : public injection_method
	{
	public:
		explicit injection(const folder& dllFile);

		~injection() noexcept;

		bool create_remote_thread(std::string file_name) override;
		//bool thread_execution_hijacking(std::string file_name) override;
		bool validate_binary(std::filesystem::path filename);
		bool inject_library();
		eValidType validate_file(const std::filesystem::path& dllFile);
	private:
		folder m_filename;
	};

	inline injection* g_injection;
}