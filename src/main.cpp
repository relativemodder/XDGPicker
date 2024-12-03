#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <relative.linuxapi/include/linuxapi.hpp>
#include <vector>

using namespace geode::prelude;


bool linuxOpenFolder(std::filesystem::path const& path){
	LinuxAPI::getInstance()->openDirectory(path.string())
	.listen([] (geode::Result<bool>* resultPtr) {
		auto result = *resultPtr;

		if (!result.isOk()) {
			FLAlertLayer::create(
				"Linux Picker",
				"Failed to open folder",
				"OK"
			)->show();
		}
	});
	return true;
}


geode::Task<geode::Result<std::filesystem::path>> linuxFilePick(
	geode::utils::file::PickMode mode, const geode::utils::file::FilePickOptions &options
) {
	return geode::Task<geode::Result<std::filesystem::path>>::runWithCallback(
		[mode, options] (auto resolve, auto progress, auto cancelled) {
			if (mode == geode::utils::file::PickMode::OpenFile) {
				LinuxAPI::getInstance()->openFiles()
				.listen([resolve] (geode::Result<std::vector<std::string>>* resultPtr) {
					if (!resultPtr->isOk()) {
						resolve(geode::Err(
							resultPtr->unwrapErr()
						));
						return;
					}

					auto filePaths = resultPtr->unwrap();
					auto filePathString = filePaths[0];
					auto filePath = std::filesystem::path(filePathString);

					resolve(geode::Ok(filePath));
				});
			}

			if (mode == geode::utils::file::PickMode::SaveFile) {
				LinuxAPI::getInstance()->saveFile(
					options.defaultPath->empty() 
					? "" 
					: options.defaultPath.value().string()
				).listen([resolve] (geode::Result<std::string>* resultPtr) {
					if (!resultPtr->isOk()) {
						resolve(geode::Err(
							resultPtr->unwrapErr()
						));
						return;
					}

					resolve(
						geode::Ok(
							std::filesystem::path(resultPtr->unwrap())
						)
					);
				});
			}

			if (mode == geode::utils::file::PickMode::OpenFolder) {
				LinuxAPI::getInstance()->openFiles(false, true)
				.listen([resolve] (geode::Result<std::vector<std::string>>* resultPtr) {
					if (!resultPtr->isOk()) {
						resolve(geode::Err(
							resultPtr->unwrapErr()
						));
						return;
					}

					auto filePaths = resultPtr->unwrap();
					auto filePathString = filePaths[0];
					auto filePath = std::filesystem::path(filePathString);

					resolve(geode::Ok(filePath));
				});
			}
		}
	);
}


geode::Task<geode::Result<std::vector<std::filesystem::path>>> linuxPickMany(
	const geode::utils::file::FilePickOptions &options
) {
	return geode::Task<geode::Result<std::vector<std::filesystem::path>>>::runWithCallback(
		[] (auto resolve, auto progress, auto cancelled) {
			LinuxAPI::getInstance()->openFiles(true, false)
			.listen([resolve] (geode::Result<std::vector<std::string>>* resultPtr) {
				if (!resultPtr->isOk()) {
					resolve(geode::Err(
						resultPtr->unwrapErr()
					));
					return;
				}
				

				auto filePathStrings = resultPtr->unwrap();
				auto filePaths = std::vector<std::filesystem::path>();

				for (auto fpString : filePathStrings) {
					log::info("Got string: {}", fpString);
					filePaths.push_back(
						std::filesystem::path(fpString)
					);
				}

				resolve(
					geode::Ok(filePaths)
				);
			});
		}
	);
}


$execute {

	Mod::get()->hook(
		reinterpret_cast<void*>(
			geode::addresser::getNonVirtual(
				geode::modifier::Resolve<std::filesystem::path const&>::func(
					&utils::file::openFolder
				)
			)
		),
		&linuxOpenFolder,
		"utils::file::openFolder",
		tulip::hook::TulipConvention::Stdcall		
	).unwrap();

	Mod::get()->hook(
		reinterpret_cast<void*>(
			geode::addresser::getNonVirtual(
				geode::modifier::Resolve<
					file::PickMode, 
					const file::FilePickOptions &
				>::func(&utils::file::pick)
			)
		),
		&linuxFilePick,
		"utils::file::pick",
		tulip::hook::TulipConvention::Stdcall		
	).unwrap();

	Mod::get()->hook(
		reinterpret_cast<void*>(
			geode::addresser::getNonVirtual(
				geode::modifier::Resolve<
					const file::FilePickOptions &
				>::func(&utils::file::pickMany)
			)
		),
		&linuxPickMany,
		"utils::file::pickMany",
		tulip::hook::TulipConvention::Stdcall		
	).unwrap();
}