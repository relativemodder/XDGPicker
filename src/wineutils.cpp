#include "wineutils.hpp"


WineUtils* WineUtils::getInstance() {
    static WineUtils* instance;
    if (instance == nullptr) {
        instance = new WineUtils();
    }

    return instance;
}


bool WineUtils::isWine() {
    HMODULE hntdll = GetModuleHandle("ntdll.dll");
    auto proc_exists = (void *)GetProcAddress(hntdll, "wine_get_version");

    return proc_exists != nullptr;
}


std::string WineUtils::getWinePlatform() {
    static void (CDECL *pwine_get_host_version)(char **sysname, char **release);
    HMODULE hntdll = GetModuleHandle("ntdll.dll");
    pwine_get_host_version = (void (CDECL *)(char **, char **))GetProcAddress(hntdll, "wine_get_host_version");

    char* sysname;
    char* version;
    pwine_get_host_version(&sysname, &version);

    return std::string(sysname);
}


bool WineUtils::isLinux() {
    if (!isWine()) return false;

    return getWinePlatform() == "Linux";
}