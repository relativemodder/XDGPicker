#include "Geode/loader/Log.hpp"
#include "Geode/utils/web.hpp"
#include "winepath.hpp"
#include "pythonserver.hpp"
#include <detours.hpp>
#include <filesystem>

bool linuxOpenFolder(std::filesystem::path const& path) {
	std::string windowsPath = path.string();
	auto unixPathBuffer = WinePath::getInstance()->getUnixFilePath(windowsPath);

    using namespace geode::prelude;

    EventListener<web::WebTask> listener;

    auto req = web::WebRequest();
    req.get(
        ReplaceAll(
            fmt::format(
                "http://127.0.0.1:8912/openfilemanager/file://{}", 
                unixPathBuffer
            ), " ", "%20"
        )
    ).listen([](web::WebResponse* res){
        if (res->ok() == false) {
            auto wtf = res->string().err();
            geode::log::error("Hey you! We've got a problem here: {}", wtf);
            return;
        }
        geode::log::debug("Holy fuck! File Manager opened!");
    });

	return true;
}

auto pickerReq = geode::utils::web::WebRequest();
std::string pickerPath;


geode::Task<geode::Result<std::filesystem::path>> linuxFilePick(
	geode::utils::file::PickMode mode, const geode::utils::file::FilePickOptions &options
) {
	using RetTask = geode::Task<geode::Result<std::filesystem::path>>;

    return RetTask::runWithCallback([mode, options] (auto resolve, auto progress, auto cancelled) {
        switch (mode) {
            case geode::utils::file::PickMode::OpenFile:
                pickerPath = std::string("/openfile");
                break;
            
            case geode::utils::file::PickMode::SaveFile:
                pickerPath = std::string("/savefile");

                if (!options.defaultPath->empty()) {
                    auto suggestedName = options.defaultPath.value();
                    pickerPath = fmt::format("/savefile/{}", ReplaceAll(suggestedName.string(), " ", "%20"));
                }

                break;
            
            case geode::utils::file::PickMode::OpenFolder:
                pickerPath = std::string("/opendirectory");
                break;
            default:
                pickerPath = "idk";
        }

        geode::log::info("Path is: {}", pickerPath);

        PythonServer::getInstance()->makeGetRequest(pickerPath)
        .listen([resolve] (std::string* res) {
            geode::log::info("Result: {}", *res); // then we need to cut off the 'file://' part using substr

            std::string stringResult = *res;

            if (stringResult.substr(0, 16) == "No file selected") {
                return resolve(geode::Err(stringResult));
            }

            std::string unixPath = stringResult.substr(7);
            std::string windowsPath = WinePath::getInstance()->getWindowsFilePath(unixPath);

            resolve(geode::Ok(std::filesystem::path(windowsPath)));
        });
    });
}


geode::Task<geode::Result<std::vector<std::filesystem::path>>> linuxPickMany(
	const geode::utils::file::FilePickOptions &options
) {
    using RetTask = geode::Task<geode::Result<std::vector<std::filesystem::path>>>;

    return RetTask::runWithCallback([] (auto resolve, auto progress, auto cancelled) {
        pickerPath = "/openfiles";
        
        geode::log::info("Path is: {}", pickerPath);

        PythonServer::getInstance()->makeGetRequest(pickerPath)
        .listen([resolve] (std::string* res) {
            geode::log::info("Result: {}", *res);
            std::string stringResult = *res;

            if (stringResult.substr(0, 17) == "No files selected") {
                return resolve(geode::Err(stringResult));
            }

            auto strings = WinePath::getInstance()->split(stringResult, '|');
            std::vector<std::filesystem::path> paths;

            for (auto pathString : strings) {
                std::string unixPath = pathString.substr(7);
                std::string windowsPath = WinePath::getInstance()->getWindowsFilePath(unixPath);

                paths.push_back(windowsPath);
            }

            resolve(geode::Ok(paths));
        });
    });

}