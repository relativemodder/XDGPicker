#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <filesystem>
#include "Geode/binding/FLAlertLayer.hpp"
#include "pythonserver.hpp"
#include "winepath.hpp"
#include "wineutils.hpp"
#include "detours.hpp"


using namespace geode::prelude;


geode::Hook* openFolderHook;
geode::Hook* filePickHook;
geode::Hook* pickManyHook;


class $modify(PickerMenuLayer, MenuLayer) {

	void addToClipboard(char* pszText) {
		int nStrLen = strlen(pszText);
		HGLOBAL hMem = GlobalAlloc(nStrLen + 1, GMEM_SHARE);
		char* pCopyTo = (char*) GlobalLock(hMem);
		strcpy(pCopyTo, pszText);
		GlobalUnlock(hMem);
		OpenClipboard(NULL);
		EmptyClipboard();

		SetClipboardData(CF_TEXT, hMem);
		CloseClipboard();
	}

	void tutorialStep1() {
		log::info("Tutorial: Step 1");
		auto wine_home_dir = WinePath::getInstance()->getUnixHome();
		log::info("Home: {}", wine_home_dir);

		auto guideURL = "https://raw.githubusercontent.com/relativemodder/XDGPicker/refs/heads/main/src/guide.pdf";
		addToClipboard((char*)guideURL);


		auto originalServerPath = std::filesystem::canonical(PythonServer::getInstance()->scriptPath);
		auto originalServerPathInWindows = WinePath::getInstance()->getWindowsFilePath(originalServerPath.string());

		auto destinationServerPath = wine_home_dir + "/picker_server.py";
		auto destinationServerPathInWindows = WinePath::getInstance()->getWindowsFilePath(destinationServerPath);

		try {
			std::filesystem::copy(originalServerPathInWindows, destinationServerPathInWindows, std::filesystem::copy_options::overwrite_existing);
		}
		catch (const std::exception& e) {
			log::error("Failed to copy server script: {}", e.what());
			return;
		}
		

		auto originalDesktopPath = std::filesystem::canonical("geode/unzipped/relative.xdgpicker/resources/relative.xdgpicker/picker_server.desktop");
		auto originalDesktopPathInWindows = WinePath::getInstance()->getWindowsFilePath(originalDesktopPath.string());

		auto destinationDesktopPath = wine_home_dir + "/.config/autostart/picker_server.desktop";
		auto destinationDesktopPathInWindows = WinePath::getInstance()->getWindowsFilePath(destinationDesktopPath);

		std::ifstream originalDesktopFile(originalDesktopPathInWindows);
		std::ofstream destinationDesktopFile(destinationDesktopPathInWindows);

		std::string line;
		while (std::getline(originalDesktopFile, line)) {
			size_t pos = line.find("<home>");
			if (pos != std::string::npos) {
				line.replace(pos, 6, wine_home_dir);
			}
			destinationDesktopFile << line << "\n";
		}

		originalDesktopFile.close();
		destinationDesktopFile.close();

		FLAlertLayer::create(
			"Attention", 
			fmt::format(
					"Server autostart file was copied to <cy>{}</c>. Relogin into your <cg>desktop environment</c>.\n\n{}", 
					destinationDesktopPath,
					"<cl>Guide link</c> was copied to your <cr>clipboard.</c>"
				), 
			"OK"
		)->show();
	}

	void transitionToStep(int step) {
		log::info("Transitioning to step {}", step);
		SEL_CallFunc selector;

		if (step == 1) {
			selector = callfunc_selector(PickerMenuLayer::tutorialStep1);
		}
		else {
			log::error("Step {} not implemented", step);
			return;
		}

		log::info("Step {} found", step);

		auto call = CCCallFunc::create(this, selector);
		auto wait = CCDelayTime::create(0.3);

		auto actions = CCArray::create();
		actions->addObject(wait);
		actions->addObject(call);

		log::info("Running action");
		runAction(CCSequence::create(actions));
	}

	void checkTask() {
		geode::log::info("Checking for server running");

		PythonServer::getInstance()->isServerAlive()
		.listen([self = this] (bool* result) {  
			bool res = *result;

			if (res) return;

			openFolderHook->disable();
			filePickHook->disable();
			pickManyHook->disable();

			geode::createQuickPopup(
				"Oops", "Looks like your Python Side <cr>is not running.</c> Copy it to the autostart directory?", 
				"No", "Yes", 
				[self](auto, bool btn2) {
					if (btn2) {
						self->transitionToStep(1);
					}
				}
			);
		});
	}

	bool init() {
		if (!MenuLayer::init()) return false;

		auto call = CCCallFunc::create(this, callfunc_selector(PickerMenuLayer::checkTask));
		runAction(call);

		return true;
	}
};


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

	HANDLE s_outHandle = nullptr;

	s_outHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!s_outHandle && AttachConsole(ATTACH_PARENT_PROCESS)) {
		s_outHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	}

	std::string path;
    DWORD dummy;
	char buf[MAX_PATH + 1];
    auto count = GetFinalPathNameByHandleA(s_outHandle, buf, MAX_PATH + 1,
        FILE_NAME_OPENED | VOLUME_NAME_NT);

    if (count != 0) {
        path = std::string(buf, count - 1);
    }

	if ((count == 0 || path.ends_with("\\dev\\null"))) {
		
		Loader::get()->getLoadedMod("geode.loader")->setSettingValue("show-platform-console", true);

		auto consoleWindow = GetConsoleWindow();

		if (consoleWindow == nullptr) {
			auto result = MessageBox(NULL, "Running with redirected stdout to /dev/null, restart?", "Wine/Proton Bug", MB_OKCANCEL);

			if (result == IDOK) {
				geode::utils::game::restart();
				return;
			}

			MessageBox(NULL, "Ok, that's your decision, good luck lol", "Ok then", MB_OKCANCEL);
		}

		ShowWindow(consoleWindow, SW_HIDE);
	}

	PythonServer::getInstance()->setupPaths();

	openFolderHook = Mod::get()->hook(
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

	filePickHook = Mod::get()->hook(
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

	pickManyHook = Mod::get()->hook(
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