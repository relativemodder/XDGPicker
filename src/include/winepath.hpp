#pragma once
#include <Windows.h>
#include <Geode/Geode.hpp>
#include <string>


class WinePath {
public:
    static WinePath* getInstance();
public:
    int cmd(char *cmd, char *output, DWORD maxbuffer);
    std::string getUnixFilePath(std::string windowsFilePath);
    std::string getWindowsFilePath(std::string unixFilePath);
    std::vector<std::string> split (const std::string &s, char delim);
};