#include "common.hpp"
#include "renderer.hpp"
#include "benchmark.hpp"
#include "thread_pool.hpp"
#include "file_manager.hpp"

#include "process/injection.hpp"

#include "api/remote/download_binary.hpp"
#include "api/user/user_authentication.hpp"
#include "api/costume_loader/costume_loader.hpp"

int main()
{
	using namespace gottvergessen;

	auto benchmark_instance = std::make_unique<benchmark>("Initialization");

	std::filesystem::path base_dir = std::getenv("appdata");
	base_dir /= "Ellohim Menu";

	auto file_manager_instance = std::make_unique<file_manager>(base_dir);

	auto binary_file = g_file_manager->get_project_folder("./Binary");

	auto logger_instance = std::make_unique<logger>("Gottvergessen Loader");

	try
	{
		auto thread_pool_instance = std::make_unique<thread_pool>();
		auto user_auth_instance = std::make_unique<user_authentication>();
		auto costumes_instance = std::make_unique<costume_loader>();

		auto binary_instance = std::make_unique<download_binary>(binary_file);
		auto inject_instance = std::make_unique<injection>(binary_file);
		auto render_instance = std::make_unique<renderer>();

		logger_instance->enable();
		benchmark_instance->get_runtime();
		benchmark_instance->reset();
		
		render_instance->on_present();

		logger_instance->disable();

		render_instance.reset();
		inject_instance.reset();
		binary_instance.reset();
		costumes_instance.reset();
		benchmark_instance.reset();
		user_auth_instance.reset();
		thread_pool_instance->destroy();
		thread_pool_instance.reset();
	}
	catch (const std::exception& ex)
	{
		LOG(WARNING) << ex.what();
	}

	logger_instance.reset();

	return EXIT_SUCCESS;
}