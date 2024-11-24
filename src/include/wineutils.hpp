#pragma once
#include <Windows.h>
#include <string>


class WineUtils {
public:
    static WineUtils* getInstance();
public:
    bool isWine();
    std::string getWinePlatform();
    bool isLinux();
};
