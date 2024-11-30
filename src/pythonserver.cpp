#include "pythonserver.hpp"
#include "Geode/utils/web.hpp"
#include <chrono>
#include <exception>
#include <filesystem>
#include <thread>


PythonServer *PythonServer::getInstance() {
  static PythonServer *instance;

  if (instance == nullptr) {
    instance = new PythonServer();
  }

  return instance;
}


void PythonServer::setupPaths() {
  oldPythonPath =
      WinePath::getInstance()->getWindowsFilePath("/usr/bin/python3");
  newPythonPath = "python.exe";
  scriptPath = "geode/unzipped/relative.xdgpicker/resources/relative.xdgpicker/"
               "server.py";
}


bool PythonServer::copyFiles() {
  setupPaths();

  if (std::filesystem::exists(newPythonPath)) {
    return true;
  }

  if (!std::filesystem::exists(oldPythonPath)) {
    geode::log::error("No Python found at /usr/bin/python3");
    geode::Notification::create(
        fmt::format("No Python found at /usr/bin/python3"),
        geode::NotificationIcon::Error);
    return false;
  }

  try {
    std::filesystem::copy(oldPythonPath, newPythonPath,
                        std::filesystem::copy_options::overwrite_existing);
  }
  catch (std::exception) {
    MessageBox(NULL, "Unable to copy Python to your GD directory, for some reason, do it manually.", "Error", MB_ICONERROR);
    return false;
  }
  return true;
}


bool PythonServer::start() {
  if (!copyFiles()) {
    return false;
  }

  system(fmt::format("{} \"{}\"", newPythonPath, scriptPath).c_str());
  return true;
}


size_t pwriteFunction(void *ptr, size_t size, size_t nmemb, std::string* data) {
    data->append((char*) ptr, size * nmemb);
    return size * nmemb;
}


geode::Task<bool> PythonServer::isServerAlive() {
  return geode::Task<bool>::runWithCallback(
        [] 
        (auto resolve, auto progress, auto cancelled) {
            geode::utils::web::WebRequest req = geode::utils::web::WebRequest();

            req.get("http://127.0.0.1:8912").listen([resolve](geode::utils::web::WebResponse* response) {
                if (response->ok() == false) {
                  geode::log::error("Listen you, damn, the response is not ok yk???");
                  resolve(false);
                  return;
                }
                resolve(true);
            });

        
    }, "Bullshit");
}


geode::Task<std::string> PythonServer::makeGetRequest(std::string path) {
    return geode::Task<std::string>::runWithCallback(
        [path = path] 
        (auto resolve, auto progress, auto cancelled) {
            geode::utils::web::WebRequest req = geode::utils::web::WebRequest();

            req.get("http://127.0.0.1:8912" + path).listen([resolve](geode::utils::web::WebResponse* response) {
                auto responseString = response->string().unwrap();
                geode::log::info("Gotcha!");
                resolve(responseString);
            });

        
    }, "Bullshit");
}