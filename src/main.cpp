#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <WinUser.h>
#include <consoleapi3.h>
#include "Geode/utils/general.hpp"
#include "pythonserver.hpp"
#include "wineutils.hpp"
#include "detours.hpp"


using namespace geode::prelude;


bool initializedPickerMod = false;


$execute {
	if (initializedPickerMod) {
		return;
	}
	
	initializedPickerMod = true;

	if (!WineUtils::getInstance()->isLinux()) {
		Notification::create("Linux File Picker: your platform is not supported", NotificationIcon::Error);
		return;
	}

	// HANDLE s_outHandle = nullptr;
// 
	// s_outHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	// if (!s_outHandle && AttachConsole(ATTACH_PARENT_PROCESS)) {
	// 	s_outHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	// }
// 
	// std::string path;
    // DWORD dummy;
	// char buf[MAX_PATH + 1];
    // auto count = GetFinalPathNameByHandleA(s_outHandle, buf, MAX_PATH + 1,
    //     FILE_NAME_OPENED | VOLUME_NAME_NT);
// 
    // if (count != 0) {
    //     path = std::string(buf, count - 1);
    // }
// 
	// if ((count == 0 || path.ends_with("\\dev\\null"))) {
	// 	
	// 	Loader::get()->getLoadedMod("geode.loader")->setSettingValue("show-platform-console", true);
// 
	// 	auto consoleWindow = GetConsoleWindow();
// 
	// 	if (consoleWindow == nullptr) {
	// 		auto result = MessageBox(NULL, "Running with redirected stdout to /dev/null, restart?", "Wine/Proton Bug", MB_OKCANCEL);
// 
	// 		if (result == IDOK) {
	// 			geode::utils::game::restart();
	// 			return;
	// 		}
// 
	// 		MessageBox(NULL, "Ok, that's your decision, good luck lol", "Ok then", MB_OKCANCEL);
	// 	}
// 
	// 	ShowWindow(consoleWindow, SW_HIDE);
	// }


	// if (!PythonServer::getInstance()->start()) {
	// 	Notification::create("Failed to start the Python server :(", NotificationIcon::Error);
	// 	return;
	// }
	

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
	);

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
	);

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
	);
}