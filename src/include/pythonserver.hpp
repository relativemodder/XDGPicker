#pragma once
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <filesystem>
#include <string>
#include <winepath.hpp>


class PythonServer {
public:
	std::string scriptPath;
private:
    std::string oldPythonPath;
	std::string newPythonPath;
public:
    static PythonServer* getInstance();
    void setupPaths();
    bool copyFiles();
    bool start();
    geode::Task<std::string> makeGetRequest(std::string path);
    geode::Task<bool> isServerAlive();
};