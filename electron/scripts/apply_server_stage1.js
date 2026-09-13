const fs = require('fs');
const path = require('path');

const SERVER_ROOT = 'L:\\Coding\\Ellohim\\Ellohim-Server';

// 1. Update ellohim.sql
const sqlPath = path.join(SERVER_ROOT, 'ellohim.sql');
let sqlContent = fs.readFileSync(sqlPath, 'utf8');

if (!sqlContent.includes('client_releases')) {
  // Add DROP TABLE statements
  sqlContent = sqlContent.replace(
    'DROP TABLE IF EXISTS binaries CASCADE;',
    'DROP TABLE IF EXISTS binaries CASCADE;\nDROP TABLE IF EXISTS client_releases CASCADE;\nDROP TABLE IF EXISTS client_modules CASCADE;'
  );

  // Add CREATE TABLE statements after binaries table
  const tableDefs = `
CREATE TABLE client_releases (
    id UUID DEFAULT gen_random_uuid() PRIMARY KEY,
    version VARCHAR(50) NOT NULL,
    release_notes TEXT,
    file_name VARCHAR(255) NOT NULL,
    file_size BIGINT NOT NULL,
    checksum VARCHAR(64) NOT NULL,
    storage_path TEXT NOT NULL,
    is_mandatory BOOLEAN DEFAULT FALSE,
    min_supported_version VARCHAR(50) DEFAULT NULL,
    status VARCHAR(50) DEFAULT 'active',
    download_count BIGINT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE client_modules (
    id UUID DEFAULT gen_random_uuid() PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    version VARCHAR(50) NOT NULL,
    target_path VARCHAR(255) NOT NULL,
    file_name VARCHAR(255) NOT NULL,
    file_size BIGINT NOT NULL,
    checksum VARCHAR(64) NOT NULL,
    storage_path TEXT NOT NULL,
    is_required BOOLEAN DEFAULT TRUE,
    status VARCHAR(50) DEFAULT 'active',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
`;

  const binariesEndPattern = /CREATE TABLE binaries \([\s\S]*?\);\n/;
  sqlContent = sqlContent.replace(binariesEndPattern, (match) => match + tableDefs);

  fs.writeFileSync(sqlPath, sqlContent, 'utf8');
  console.log('Updated ellohim.sql');
} else {
  console.log('ellohim.sql already contains client_releases');
}

// 2. Update fwddec.hpp
const fwddecPath = path.join(SERVER_ROOT, 'src', 'interfaces', 'fwddec.hpp');
let fwddecContent = fs.readFileSync(fwddecPath, 'utf8');
if (!fwddecContent.includes('ClientRelease')) {
  fwddecContent = fwddecContent.replace(
    'class TicketComment;',
    'class TicketComment;\n    class ClientRelease;\n    class ClientModule;'
  );
  fs.writeFileSync(fwddecPath, fwddecContent, 'utf8');
  console.log('Updated fwddec.hpp');
} else {
  console.log('fwddec.hpp already contains ClientRelease');
}

// Ensure client directories exist
const clientDir = path.join(SERVER_ROOT, 'src', 'module', 'client');
const clientRepoDir = path.join(clientDir, 'repository');
const clientDtoDir = path.join(clientDir, 'dto');
fs.mkdirSync(clientRepoDir, { recursive: true });
fs.mkdirSync(clientDtoDir, { recursive: true });

// 3. Create client_release.repository.hpp
const repoPath = path.join(clientRepoDir, 'client_release.repository.hpp');
const repoContent = `#pragma once

#include "common.hpp"
#include "dsql/dsql.hpp"
#include "interfaces/fwddec.hpp"

namespace ellohim
{
	class ClientRelease : public dsql::Model<ClientRelease>
	{
	public:
		DSQL_TABLE("client_releases");

		DSQL_COLUMN(id, std::string);
		DSQL_COLUMN(version, std::string);
		DSQL_COLUMN(release_notes, std::string);
		DSQL_COLUMN(file_name, std::string);
		DSQL_COLUMN(file_size, int64_t);
		DSQL_COLUMN(checksum, std::string);
		DSQL_COLUMN(storage_path, std::string);
		DSQL_COLUMN(is_mandatory, bool);
		DSQL_COLUMN(min_supported_version, std::string);
		DSQL_COLUMN(status, std::string);
		DSQL_COLUMN(download_count, int64_t);
		DSQL_COLUMN(created_at, std::string);
		DSQL_COLUMN(updated_at, std::string);
	};

	class ClientModule : public dsql::Model<ClientModule>
	{
	public:
		DSQL_TABLE("client_modules");

		DSQL_COLUMN(id, std::string);
		DSQL_COLUMN(name, std::string);
		DSQL_COLUMN(version, std::string);
		DSQL_COLUMN(target_path, std::string);
		DSQL_COLUMN(file_name, std::string);
		DSQL_COLUMN(file_size, int64_t);
		DSQL_COLUMN(checksum, std::string);
		DSQL_COLUMN(storage_path, std::string);
		DSQL_COLUMN(is_required, bool);
		DSQL_COLUMN(status, std::string);
		DSQL_COLUMN(created_at, std::string);
		DSQL_COLUMN(updated_at, std::string);
	};
}
`;
fs.writeFileSync(repoPath, repoContent, 'utf8');
console.log('Created client_release.repository.hpp');

// 4. Create client_release.dto.hpp
const dtoPath = path.join(clientDtoDir, 'client_release.dto.hpp');
const dtoContent = `#pragma once
#include <common.hpp>
#include "../repository/client_release.repository.hpp"

namespace ellohim
{
	struct ClientReleaseDto
	{
		ClientReleaseDto() = default;
		ClientReleaseDto(const ClientRelease& r) :
		    id(r.id()),
		    version(r.version()),
		    release_notes(r.release_notes()),
		    file_name(r.file_name()),
		    file_size(r.file_size()),
		    checksum(r.checksum()),
		    is_mandatory(r.is_mandatory()),
		    min_supported_version(r.min_supported_version()),
		    status(r.status()),
		    download_count(r.download_count()),
		    created_at(r.created_at()),
		    updated_at(r.updated_at())
		{
			download_url = "/client/download/release/" + id;
		}

		std::string id;
		std::string version;
		std::string release_notes;
		std::string file_name;
		int64_t file_size = 0;
		std::string checksum;
		bool is_mandatory = false;
		std::string min_supported_version;
		std::string status = "active";
		int64_t download_count = 0;
		std::string download_url;
		std::string created_at;
		std::string updated_at;

		nlohmann::json to_json() const
		{
			return *this;
		}

		static ClientReleaseDto from_json(nlohmann::json const& j)
		{
			return j.get<ClientReleaseDto>();
		}

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(ClientReleaseDto, id, version, release_notes, file_name, file_size, checksum, is_mandatory, min_supported_version, status, download_count, download_url, created_at, updated_at)
	};

	struct ClientModuleDto
	{
		ClientModuleDto() = default;
		ClientModuleDto(const ClientModule& m) :
		    id(m.id()),
		    name(m.name()),
		    version(m.version()),
		    target_path(m.target_path()),
		    file_name(m.file_name()),
		    file_size(m.file_size()),
		    checksum(m.checksum()),
		    is_required(m.is_required()),
		    status(m.status()),
		    created_at(m.created_at()),
		    updated_at(m.updated_at())
		{
			download_url = "/client/download/module/" + id;
		}

		std::string id;
		std::string name;
		std::string version;
		std::string target_path;
		std::string file_name;
		int64_t file_size = 0;
		std::string checksum;
		bool is_required = true;
		std::string status = "active";
		std::string download_url;
		std::string created_at;
		std::string updated_at;

		nlohmann::json to_json() const
		{
			return *this;
		}

		static ClientModuleDto from_json(nlohmann::json const& j)
		{
			return j.get<ClientModuleDto>();
		}

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(ClientModuleDto, id, name, version, target_path, file_name, file_size, checksum, is_required, status, download_url, created_at, updated_at)
	};

	struct CheckUpdateResponseDto
	{
		bool has_update = false;
		bool is_mandatory = false;
		std::string current_version;
		std::string latest_version;
		std::optional<ClientReleaseDto> latest_release = std::nullopt;
		std::vector<ClientModuleDto> modules;

		nlohmann::json to_json() const
		{
			return *this;
		}

		static CheckUpdateResponseDto from_json(nlohmann::json const& j)
		{
			return j.get<CheckUpdateResponseDto>();
		}

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(CheckUpdateResponseDto, has_update, is_mandatory, current_version, latest_version, latest_release, modules)
	};

	struct UpdateReleaseDto
	{
		std::optional<std::string> version;
		std::optional<std::string> release_notes;
		std::optional<bool> is_mandatory;
		std::optional<std::string> min_supported_version;
		std::optional<std::string> status;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(UpdateReleaseDto, version, release_notes, is_mandatory, min_supported_version, status)
	};

	struct UpdateModuleDto
	{
		std::optional<std::string> name;
		std::optional<std::string> version;
		std::optional<std::string> target_path;
		std::optional<bool> is_required;
		std::optional<std::string> status;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(UpdateModuleDto, name, version, target_path, is_required, status)
	};
}
`;
fs.writeFileSync(dtoPath, dtoContent, 'utf8');
console.log('Created client_release.dto.hpp');

// 5. Create client.service.hpp
const serviceHppPath = path.join(clientDir, 'client.service.hpp');
const serviceHppContent = `#pragma once

#include "common.hpp"
#include <drogon/HttpResponse.h>
#include "dto/client_release.dto.hpp"
#include "interfaces/response.hpp"
#include "interfaces/file_response.hpp"
#include "repository/client_release.repository.hpp"

using namespace drogon;

namespace ellohim
{
	class client_service
	{
	public:
		explicit client_service();
		~client_service() noexcept;

		client_service(client_service const&) = delete;
		client_service& operator=(client_service const&) = delete;
		client_service(client_service&&) = delete;
		client_service& operator=(client_service&&) = delete;

		// Client Public Endpoints
		Task<HttpResponsePtr> checkUpdate(HttpRequestPtr req, response_data<CheckUpdateResponseDto> response = {});
		Task<HttpResponsePtr> downloadRelease(HttpRequestPtr req, std::string id, file_response response = {});
		Task<HttpResponsePtr> downloadModule(HttpRequestPtr req, std::string id, file_response response = {});
		Task<HttpResponsePtr> getModules(HttpRequestPtr req, response_data<ClientModuleDto> response = {});

		// Admin Management Endpoints
		Task<HttpResponsePtr> uploadRelease(HttpRequestPtr req, response_data<ClientReleaseDto> response = {});
		Task<HttpResponsePtr> findAllReleases(HttpRequestPtr req, response_data<ClientReleaseDto> response = {});
		Task<HttpResponsePtr> updateRelease(HttpRequest<UpdateReleaseDto> req, std::string id, response_data<ClientReleaseDto> response = {});
		Task<HttpResponsePtr> removeRelease(HttpRequestPtr req, std::string id, response_data<ClientReleaseDto> response = {});

		Task<HttpResponsePtr> uploadModule(HttpRequestPtr req, response_data<ClientModuleDto> response = {});
		Task<HttpResponsePtr> findAllModules(HttpRequestPtr req, response_data<ClientModuleDto> response = {});
		Task<HttpResponsePtr> updateModule(HttpRequest<UpdateModuleDto> req, std::string id, response_data<ClientModuleDto> response = {});
		Task<HttpResponsePtr> removeModule(HttpRequestPtr req, std::string id, response_data<ClientModuleDto> response = {});

	private:
		bool isVersionNewer(const std::string& candidate, const std::string& current);
	};
}
`;
fs.writeFileSync(serviceHppPath, serviceHppContent, 'utf8');
console.log('Created client.service.hpp');

// 6. Create client.service.cpp
const serviceCppPath = path.join(clientDir, 'client.service.cpp');
const serviceCppContent = `#include "client.service.hpp"
#include "common/compression/compressor.hpp"
#include "common/http_util.hpp"
#include <exception/exception.hpp>
#include <coroutine/coroutine.hpp>
#include <drogon/MultiPart.h>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace ellohim
{
	client_service::client_service()
	{
		LOG(INFO) << "Client Management Service registered";
		try
		{
			fs::create_directories("uploads/client/releases");
			fs::create_directories("uploads/client/modules");
		}
		catch (const std::exception& e)
		{
			LOG(WARNING) << "Could not initialize client upload directories: " << e.what();
		}
	}

	client_service::~client_service() noexcept
	{
	}

	bool client_service::isVersionNewer(const std::string& candidate, const std::string& current)
	{
		if (candidate.empty()) return false;
		if (current.empty()) return true;
		if (candidate == current) return false;

		auto parseVer = [](const std::string& v) -> std::vector<int> {
			std::vector<int> parts;
			std::string clean = v;
			if (!clean.empty() && (clean[0] == 'v' || clean[0] == 'V'))
			{
				clean = clean.substr(1);
			}
			std::stringstream ss(clean);
			std::string item;
			while (std::getline(ss, item, '.'))
			{
				try
				{
					// handle build metadata or dashes like 1.0.0-beta
					size_t dash = item.find('-');
					if (dash != std::string::npos) item = item.substr(0, dash);
					parts.push_back(std::stoi(item));
				}
				catch (...)
				{
					parts.push_back(0);
				}
			}
			while (parts.size() < 3) parts.push_back(0);
			return parts;
		};

		auto candParts = parseVer(candidate);
		auto curParts = parseVer(current);

		for (size_t i = 0; i < candParts.size() && i < curParts.size(); ++i)
		{
			if (candParts[i] > curParts[i]) return true;
			if (candParts[i] < curParts[i]) return false;
		}
		return false;
	}

	Task<HttpResponsePtr> client_service::checkUpdate(HttpRequestPtr req, response_data<CheckUpdateResponseDto> response)
	{
		std::string clientVersion = req->getParameter("version");

		auto releases = co_await ClientRelease::findAll(dsql::QueryOptions{
		    .where = dsql::Where::And({{"status", dsql::Op::EQ, "active"}}),
		    .order = {{"created_at", dsql::Dir::DESC}},
		    .limit = 10});

		CheckUpdateResponseDto result;
		result.current_version = clientVersion;

		if (!releases.empty())
		{
			// Find release with the newest version
			std::shared_ptr<ClientRelease> newestRelease = releases[0];
			for (const auto& r : releases)
			{
				if (isVersionNewer(r->version(), newestRelease->version()))
				{
					newestRelease = r;
				}
			}

			result.latest_version = newestRelease->version();
			bool hasNewer = isVersionNewer(newestRelease->version(), clientVersion);
			result.has_update = hasNewer;

			if (hasNewer)
			{
				bool mandatory = newestRelease->is_mandatory();
				std::string minSupported = newestRelease->min_supported_version();
				if (!minSupported.empty() && isVersionNewer(minSupported, clientVersion))
				{
					mandatory = true;
				}
				result.is_mandatory = mandatory;
				result.latest_release = ClientReleaseDto(*newestRelease);
			}
		}

		// Also attach active client modules
		auto modules = co_await ClientModule::findAll(dsql::QueryOptions{
		    .where = dsql::Where::And({{"status", dsql::Op::EQ, "active"}}),
		    .order = {{"created_at", dsql::Dir::DESC}}});

		for (const auto& m : modules)
		{
			result.modules.push_back(ClientModuleDto(*m));
		}

		response.data = result;
		response.message = result.has_update ? "New client update available" : "Client is up to date";
		response.success = true;

		co_return response.json();
	}

	Task<HttpResponsePtr> client_service::downloadRelease(HttpRequestPtr req, std::string id, file_response response)
	{
		if (id.empty()) throw BadRequestException("Release ID is required");

		auto release = co_await ClientRelease::findByPk(id);
		if (!release) throw NotFoundException("Client release not found");

		std::string storagePath = release->storage_path();
		if (!fs::exists(storagePath))
		{
			throw NotFoundException("Release binary file not found on disk");
		}

		// Increment download count
		release->download_count(release->download_count() + 1);
		co_await release->save();

		// Drogon's HttpResponse::newFileResponse handles HTTP Range requests (resumable downloads)
		auto fileResp = HttpResponse::newFileResponse(storagePath, release->file_name(), CT_APPLICATION_OCTET_STREAM);
		fileResp->addHeader("Accept-Ranges", "bytes");
		fileResp->addHeader("Content-Disposition", fmt::format("attachment; filename=\"{}\"", release->file_name()));
		fileResp->addHeader("Access-Control-Expose-Headers", "Content-Disposition, Content-Range, Accept-Ranges");

		co_return fileResp;
	}

	Task<HttpResponsePtr> client_service::downloadModule(HttpRequestPtr req, std::string id, file_response response)
	{
		if (id.empty()) throw BadRequestException("Module ID is required");

		auto moduleObj = co_await ClientModule::findByPk(id);
		if (!moduleObj) throw NotFoundException("Client module not found");

		std::string storagePath = moduleObj->storage_path();
		if (!fs::exists(storagePath))
		{
			throw NotFoundException("Module file not found on disk");
		}

		auto fileResp = HttpResponse::newFileResponse(storagePath, moduleObj->file_name(), CT_APPLICATION_OCTET_STREAM);
		fileResp->addHeader("Accept-Ranges", "bytes");
		fileResp->addHeader("Content-Disposition", fmt::format("attachment; filename=\"{}\"", moduleObj->file_name()));
		fileResp->addHeader("Access-Control-Expose-Headers", "Content-Disposition, Content-Range, Accept-Ranges");

		co_return fileResp;
	}

	Task<HttpResponsePtr> client_service::getModules(HttpRequestPtr req, response_data<ClientModuleDto> response)
	{
		auto rows = co_await ClientModule::findAll(dsql::QueryOptions{
		    .where = dsql::Where::And({{"status", dsql::Op::EQ, "active"}}),
		    .order = {{"created_at", dsql::Dir::DESC}}});

		std::vector<ClientModuleDto> dtos;
		dtos.reserve(rows.size());
		for (const auto& r : rows)
		{
			dtos.push_back(ClientModuleDto(*r));
		}

		response.data = dtos;
		response.message = "Success retrieve client modules";
		response.success = true;

		co_return response.json();
	}

	Task<HttpResponsePtr> client_service::uploadRelease(HttpRequestPtr req, response_data<ClientReleaseDto> response)
	{
		MultiPartParser parser;
		if (parser.parse(req) != 0 || parser.getFiles().empty())
		{
			throw BadRequestException("A valid release executable file is required");
		}

		const auto& file = parser.getFiles()[0];
		auto params = parser.getParameters();

		std::string version = params["version"];
		if (version.empty()) throw BadRequestException("Version string is required");

		std::string releaseNotes = params["release_notes"];
		std::string minSupported = params["min_supported_version"];
		bool isMandatory = (params["is_mandatory"] == "true" || params["is_mandatory"] == "1");

		std::string fileName = file.getFileName();
		if (fileName.empty()) fileName = fmt::format("Gottvergessen-Loader-{}.exe", version);

		std::string fileContent(file.fileData(), file.fileLength());
		if (fileContent.empty()) throw BadRequestException("File payload is empty");

		int64_t fileSize = static_cast<int64_t>(fileContent.size());
		std::string checksum = compression::Compressor::calculate_sha256(fileContent);

		std::string targetDir = "uploads/client/releases/" + version;
		fs::create_directories(targetDir);
		std::string targetPath = targetDir + "/" + fileName;

		std::ofstream out(targetPath, std::ios::binary);
		if (!out.is_open())
		{
			throw InternalServerErrorException("Failed to write release file to disk");
		}
		out.write(fileContent.data(), fileContent.size());
		out.close();

		dsql::FieldMap data;
		data["version"] = version;
		data["release_notes"] = releaseNotes;
		data["file_name"] = fileName;
		data["file_size"] = fileSize;
		data["checksum"] = checksum;
		data["storage_path"] = targetPath;
		data["is_mandatory"] = isMandatory;
		if (!minSupported.empty()) data["min_supported_version"] = minSupported;
		data["status"] = "active";
		data["download_count"] = int64_t{0};

		auto created = co_await ClientRelease::create(data);
		if (!created)
		{
			fs::remove(targetPath);
			throw InternalServerErrorException("Failed to persist client release to database");
		}

		response.data = ClientReleaseDto(*created);
		response.message = "Client release uploaded successfully";
		response.success = true;

		co_return response.status(k201Created).json();
	}

	Task<HttpResponsePtr> client_service::findAllReleases(HttpRequestPtr req, response_data<ClientReleaseDto> response)
	{
		int page = 1;
		int limit = 20;
		const auto& pStr = req->getParameter("page");
		const auto& lStr = req->getParameter("limit");
		if (!pStr.empty()) page = std::stoi(pStr);
		if (!lStr.empty()) limit = std::stoi(lStr);
		int offset = (page - 1) * limit;

		auto rows = co_await ClientRelease::findAll(dsql::QueryOptions{
		    .order = {{"created_at", dsql::Dir::DESC}},
		    .limit = limit,
		    .offset = offset});

		int64_t total = co_await ClientRelease::count({});

		std::vector<ClientReleaseDto> dtos;
		dtos.reserve(rows.size());
		for (const auto& r : rows)
		{
			dtos.push_back(ClientReleaseDto(*r));
		}

		response.data = dtos;
		response.total = total;
		response.page = page;
		response.limit = limit;
		response.last_page = static_cast<int>((total + limit - 1) / limit);
		response.message = "Success retrieve client releases";
		response.success = true;

		co_return response.json();
	}

	Task<HttpResponsePtr> client_service::updateRelease(HttpRequest<UpdateReleaseDto> req, std::string id, response_data<ClientReleaseDto> response)
	{
		if (id.empty()) throw BadRequestException("Release ID is required");

		auto release = co_await ClientRelease::findByPk(id);
		if (!release) throw NotFoundException("Client release not found");

		auto json = req.json();
		release->setJson(json);

		if (!co_await release->save())
		{
			throw InternalServerErrorException("Failed to update client release");
		}

		response.data = ClientReleaseDto(*release);
		response.message = "Successfully updated client release";
		response.success = true;

		co_return response.json();
	}

	Task<HttpResponsePtr> client_service::removeRelease(HttpRequestPtr req, std::string id, response_data<ClientReleaseDto> response)
	{
		if (id.empty()) throw BadRequestException("Release ID is required");

		auto release = co_await ClientRelease::findByPk(id);
		if (!release) throw NotFoundException("Client release not found");

		std::string path = release->storage_path();
		if (fs::exists(path))
		{
			std::error_code ec;
			fs::remove(path, ec);
		}

		if (!co_await release->destroy())
		{
			throw InternalServerErrorException("Failed to delete client release");
		}

		response.message = "Client release deleted successfully";
		response.success = true;

		co_return response.json();
	}

	Task<HttpResponsePtr> client_service::uploadModule(HttpRequestPtr req, response_data<ClientModuleDto> response)
	{
		MultiPartParser parser;
		if (parser.parse(req) != 0 || parser.getFiles().empty())
		{
			throw BadRequestException("A valid module file (.dll/.bin) is required");
		}

		const auto& file = parser.getFiles()[0];
		auto params = parser.getParameters();

		std::string name = params["name"];
		std::string version = params["version"];
		std::string targetPathRel = params["target_path"];
		bool isRequired = (params["is_required"] != "false" && params["is_required"] != "0");

		if (name.empty()) throw BadRequestException("Module name is required");
		if (version.empty()) throw BadRequestException("Module version is required");
		if (targetPathRel.empty()) targetPathRel = "modules";

		std::string fileName = file.getFileName();
		if (fileName.empty()) fileName = name + ".dll";

		std::string fileContent(file.fileData(), file.fileLength());
		if (fileContent.empty()) throw BadRequestException("Module file payload is empty");

		int64_t fileSize = static_cast<int64_t>(fileContent.size());
		std::string checksum = compression::Compressor::calculate_sha256(fileContent);

		std::string targetDir = "uploads/client/modules/" + name;
		fs::create_directories(targetDir);
		std::string savedPath = targetDir + "/" + fileName;

		std::ofstream out(savedPath, std::ios::binary);
		if (!out.is_open())
		{
			throw InternalServerErrorException("Failed to write module file to disk");
		}
		out.write(fileContent.data(), fileContent.size());
		out.close();

		dsql::FieldMap data;
		data["name"] = name;
		data["version"] = version;
		data["target_path"] = targetPathRel;
		data["file_name"] = fileName;
		data["file_size"] = fileSize;
		data["checksum"] = checksum;
		data["storage_path"] = savedPath;
		data["is_required"] = isRequired;
		data["status"] = "active";

		auto created = co_await ClientModule::create(data);
		if (!created)
		{
			fs::remove(savedPath);
			throw InternalServerErrorException("Failed to persist module to database");
		}

		response.data = ClientModuleDto(*created);
		response.message = "Client module uploaded successfully";
		response.success = true;

		co_return response.status(k201Created).json();
	}

	Task<HttpResponsePtr> client_service::findAllModules(HttpRequestPtr req, response_data<ClientModuleDto> response)
	{
		auto rows = co_await ClientModule::findAll(dsql::QueryOptions{
		    .order = {{"created_at", dsql::Dir::DESC}}});

		std::vector<ClientModuleDto> dtos;
		dtos.reserve(rows.size());
		for (const auto& r : rows)
		{
			dtos.push_back(ClientModuleDto(*r));
		}

		response.data = dtos;
		response.total = static_cast<int64_t>(dtos.size());
		response.message = "Success retrieve client modules";
		response.success = true;

		co_return response.json();
	}

	Task<HttpResponsePtr> client_service::updateModule(HttpRequest<UpdateModuleDto> req, std::string id, response_data<ClientModuleDto> response)
	{
		if (id.empty()) throw BadRequestException("Module ID is required");

		auto mod = co_await ClientModule::findByPk(id);
		if (!mod) throw NotFoundException("Client module not found");

		auto json = req.json();
		mod->setJson(json);

		if (!co_await mod->save())
		{
			throw InternalServerErrorException("Failed to update module");
		}

		response.data = ClientModuleDto(*mod);
		response.message = "Successfully updated module";
		response.success = true;

		co_return response.json();
	}

	Task<HttpResponsePtr> client_service::removeModule(HttpRequestPtr req, std::string id, response_data<ClientModuleDto> response)
	{
		if (id.empty()) throw BadRequestException("Module ID is required");

		auto mod = co_await ClientModule::findByPk(id);
		if (!mod) throw NotFoundException("Client module not found");

		std::string path = mod->storage_path();
		if (fs::exists(path))
		{
			std::error_code ec;
			fs::remove(path, ec);
		}

		if (!co_await mod->destroy())
		{
			throw InternalServerErrorException("Failed to delete module");
		}

		response.message = "Client module deleted successfully";
		response.success = true;

		co_return response.json();
	}
}
`;
fs.writeFileSync(serviceCppPath, serviceCppContent, 'utf8');
console.log('Created client.service.cpp');

// 7. Create client.h
const clientHPath = path.join(clientDir, 'client.h');
const clientHContent = `#pragma once
#include "common.hpp"
#include <drogon/HttpController.h>
#include "client.service.hpp"

using namespace drogon;

namespace ellohim
{
	class client : public drogon::HttpController<client>
	{
		client_service m_client_service{};

	public:
		METHOD_LIST_BEGIN
		// Public Client endpoints
		ADD_METHOD_TO(client::checkUpdate, "/client/check-update", Get, Options, "ellohim::filter_header");
		ADD_METHOD_TO(client::downloadRelease, "/client/download/release/{id}", Get, Options, "ellohim::filter_header");
		ADD_METHOD_TO(client::downloadModule, "/client/download/module/{id}", Get, Options, "ellohim::filter_header");
		ADD_METHOD_TO(client::getModules, "/client/modules", Get, Options, "ellohim::filter_header");

		// Admin Management endpoints
		ADD_METHOD_TO(client::uploadRelease, "/client/admin/release/upload", Post, Options, "ellohim::filter_header", "ellohim::auth_middleware", "ellohim::admin_middleware");
		ADD_METHOD_TO(client::findAllReleases, "/client/admin/releases", Get, Options, "ellohim::filter_header", "ellohim::auth_middleware", "ellohim::admin_middleware");
		ADD_METHOD_TO(client::updateRelease, "/client/admin/release/{id}", Patch, Options, "ellohim::filter_header", "ellohim::auth_middleware", "ellohim::admin_middleware");
		ADD_METHOD_TO(client::removeRelease, "/client/admin/release/{id}", Delete, Options, "ellohim::filter_header", "ellohim::auth_middleware", "ellohim::admin_middleware");

		ADD_METHOD_TO(client::uploadModule, "/client/admin/module/upload", Post, Options, "ellohim::filter_header", "ellohim::auth_middleware", "ellohim::admin_middleware");
		ADD_METHOD_TO(client::findAllModules, "/client/admin/modules", Get, Options, "ellohim::filter_header", "ellohim::auth_middleware", "ellohim::admin_middleware");
		ADD_METHOD_TO(client::updateModule, "/client/admin/module/{id}", Patch, Options, "ellohim::filter_header", "ellohim::auth_middleware", "ellohim::admin_middleware");
		ADD_METHOD_TO(client::removeModule, "/client/admin/module/{id}", Delete, Options, "ellohim::filter_header", "ellohim::auth_middleware", "ellohim::admin_middleware");
		METHOD_LIST_END

		// Public
		Task<HttpResponsePtr> checkUpdate(HttpRequestPtr req);
		Task<HttpResponsePtr> downloadRelease(HttpRequestPtr req, std::string id);
		Task<HttpResponsePtr> downloadModule(HttpRequestPtr req, std::string id);
		Task<HttpResponsePtr> getModules(HttpRequestPtr req);

		// Admin
		Task<HttpResponsePtr> uploadRelease(HttpRequestPtr req);
		Task<HttpResponsePtr> findAllReleases(HttpRequestPtr req);
		Task<HttpResponsePtr> updateRelease(HttpRequestPtr req, std::string id);
		Task<HttpResponsePtr> removeRelease(HttpRequestPtr req, std::string id);

		Task<HttpResponsePtr> uploadModule(HttpRequestPtr req);
		Task<HttpResponsePtr> findAllModules(HttpRequestPtr req);
		Task<HttpResponsePtr> updateModule(HttpRequestPtr req, std::string id);
		Task<HttpResponsePtr> removeModule(HttpRequestPtr req, std::string id);
	};
}
`;
fs.writeFileSync(clientHPath, clientHContent, 'utf8');
console.log('Created client.h');

// 8. Create client.cc
const clientCcPath = path.join(clientDir, 'client.cc');
const clientCcContent = `#include "client.h"

namespace ellohim
{
	Task<HttpResponsePtr> client::checkUpdate(HttpRequestPtr req)
	{
		co_return co_await m_client_service.checkUpdate(req);
	}

	Task<HttpResponsePtr> client::downloadRelease(HttpRequestPtr req, std::string id)
	{
		co_return co_await m_client_service.downloadRelease(req, id);
	}

	Task<HttpResponsePtr> client::downloadModule(HttpRequestPtr req, std::string id)
	{
		co_return co_await m_client_service.downloadModule(req, id);
	}

	Task<HttpResponsePtr> client::getModules(HttpRequestPtr req)
	{
		co_return co_await m_client_service.getModules(req);
	}

	Task<HttpResponsePtr> client::uploadRelease(HttpRequestPtr req)
	{
		co_return co_await m_client_service.uploadRelease(req);
	}

	Task<HttpResponsePtr> client::findAllReleases(HttpRequestPtr req)
	{
		co_return co_await m_client_service.findAllReleases(req);
	}

	Task<HttpResponsePtr> client::updateRelease(HttpRequestPtr req, std::string id)
	{
		co_return co_await m_client_service.updateRelease(req, id);
	}

	Task<HttpResponsePtr> client::removeRelease(HttpRequestPtr req, std::string id)
	{
		co_return co_await m_client_service.removeRelease(req, id);
	}

	Task<HttpResponsePtr> client::uploadModule(HttpRequestPtr req)
	{
		co_return co_await m_client_service.uploadModule(req);
	}

	Task<HttpResponsePtr> client::findAllModules(HttpRequestPtr req)
	{
		co_return co_await m_client_service.findAllModules(req);
	}

	Task<HttpResponsePtr> client::updateModule(HttpRequestPtr req, std::string id)
	{
		co_return co_await m_client_service.updateModule(req, id);
	}

	Task<HttpResponsePtr> client::removeModule(HttpRequestPtr req, std::string id)
	{
		co_return co_await m_client_service.removeModule(req, id);
	}
}
`;
fs.writeFileSync(clientCcPath, clientCcContent, 'utf8');
console.log('Created client.cc');
console.log('Stage 1 files created successfully!');
