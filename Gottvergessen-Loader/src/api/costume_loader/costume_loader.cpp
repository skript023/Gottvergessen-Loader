#include "costume_loader.hpp"
#include "file_manager.hpp"
#include "gui.hpp"

#include "api/user/user_authentication.hpp"

namespace gottvergessen
{
	costume_loader::costume_loader()
	{
		g_costume_loader = this;
	}
	costume_loader::~costume_loader()
	{
		g_costume_loader = nullptr;
	}
	void costume_loader::execute()
	{
		g_thread_pool->add_job([this] {
			download("male_accessories");
			download("male_hair");
			download("male_legs");
			download("male_shoes");
			download("male_tops");
			download("male_torsos");
			download("male_undershirts");
			});

		g_thread_pool->add_job([this] {
			download("female_accessories");
			download("female_hair");
			download("female_legs");
			download("female_shoes");
			download("female_tops");
			download("female_torsos");
			download("female_undershirts");
			});

		g_thread_pool->add_job([this] {
			download("props_male_bracelets");
			download("props_male_ears");
			download("props_male_glasses");
			download("props_male_hats");
			download("props_male_watches");
			download("masks_male");
			});

		g_thread_pool->add_job([this] {
			download("props_female_bracelets");
			download("props_female_ears");
			download("props_female_glasses");
			download("props_female_hats");
			download("props_female_watches");
			download("masks_female");
			});
	}
	bool costume_loader::download(const std::string filename)
	{
		auto folder = g_file_manager->get_project_folder("./costumes");
		auto base_dir = folder.get_file("./" + filename + ".json").get_path();

		if (std::filesystem::exists(base_dir)) return true;

		nlohmann::ordered_json body = {
			{ xorstr("name"), filename }
		};

		std::string content_type = xorstr("Content-Type: application/json");
		std::string accept = xorstr("Accept: application/json");
		std::string auth = std::format("Authorization: Bearer {}", g_user_authentication->get_token());

		std::ofstream file(base_dir, std::ios::out | std::ios::trunc);

		try
		{
			http::Request request(url);
			http::Response response = request.send("POST", body.dump(), { auth, accept, content_type });

			auto result = nlohmann::ordered_json::parse(response.body.begin(), response.body.end());

			file << result.dump(4);
			LOG(HACKER) << filename << extension << " downloaded successfully";
		}
		catch (const std::exception&)
		{
			LOG(WARNING) << "Failed to download costume, is the host down?";

			file.close();

			return false;
		}
		file.close();

		return true;
	}
}