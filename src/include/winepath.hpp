#pragma once
#include <Windows.h>
#include <Geode/Geode.hpp>
#include <string>

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to);


class WinePath {
public:
    static WinePath* getInstance();
public:
    int cmd(char *cmd, char *output, DWORD maxbuffer);
    std::string getUnixFilePath(std::string windowsFilePath);
    std::string getUnixHome();
    std::string getUnixHomeInWindows();
    std::string getWindowsFilePath(std::string unixFilePath);
    std::vector<std::string> split (const std::string &s, char delim);
    std::map<std::basic_string<TCHAR>, std::basic_string<TCHAR>> get_envs();
};