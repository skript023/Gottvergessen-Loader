#include "common.hpp"
#include "renderer.hpp"
#include "benchmark.hpp"
#include "thread_pool.hpp"
#include "file_manager.hpp"

#include "process/injection.hpp"

#include "api/remote/download_binary.hpp"
#include "api/user/user_authentication.hpp"

int main()
{
	using namespace gottvergessen;

	auto logger_instance = std::make_unique<logger>("Gottvergessen Loader");
	logger_instance->enable();

	std::filesystem::path base_dir = std::getenv("appdata");
	base_dir /= "Ellohim Menu";

	file_manager::init(base_dir);

	user_authentication::init();
	download_binary::init();

	auto benchmark_instance = std::make_unique<benchmark>("Initialization");
	auto binary_file = file_manager::get_project_folder("./Binary");

	try
	{
		auto thread_pool_instance = std::make_unique<thread_pool>();
		auto render_instance = std::make_unique<renderer>();

		benchmark_instance->get_runtime();
		benchmark_instance->reset();
		
		render_instance->on_present();

		logger_instance->disable();

		render_instance.reset();
		benchmark_instance.reset();
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