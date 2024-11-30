#include "winepath.hpp"
#include <processenv.h>

WinePath *WinePath::getInstance() {
  static WinePath *instance;
  if (instance == nullptr) {
    instance = new WinePath();
  }

  return instance;
}

int WinePath::cmd(char *cmd, char *output, DWORD maxbuffer) { // STOLEN!!!
  HANDLE readHandle;
  HANDLE writeHandle;
  HANDLE stdHandle;
  DWORD bytesRead;
  DWORD retCode;
  SECURITY_ATTRIBUTES sa;
  PROCESS_INFORMATION pi;
  STARTUPINFO si;

  ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
  ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
  ZeroMemory(&si, sizeof(STARTUPINFO));

  sa.bInheritHandle = true;
  sa.lpSecurityDescriptor = NULL;
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  si.cb = sizeof(STARTUPINFO);
  si.dwFlags = STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_HIDE;

  if (!CreatePipe(&readHandle, &writeHandle, &sa, NULL)) {
    OutputDebugString("cmd: CreatePipe failed!\n");
    return 0;
  }

  stdHandle = GetStdHandle(STD_OUTPUT_HANDLE);

  if (!SetStdHandle(STD_OUTPUT_HANDLE, writeHandle)) {
    OutputDebugString("cmd: SetStdHandle(writeHandle) failed!\n");
    return 0;
  }

  if (!CreateProcess(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
    OutputDebugString("cmd: CreateProcess failed!\n");
    return 0;
  }

  GetExitCodeProcess(pi.hProcess, &retCode);
  while (retCode == STILL_ACTIVE) {
    GetExitCodeProcess(pi.hProcess, &retCode);
  }

  if (!ReadFile(readHandle, output, maxbuffer, &bytesRead, NULL)) {
    OutputDebugString("cmd: ReadFile failed!\n");
    return 0;
  }
  output[bytesRead] = '\0';

  if (!SetStdHandle(STD_OUTPUT_HANDLE, stdHandle)) {
    OutputDebugString("cmd: SetStdHandle(stdHandle) failed!\n");
    return 0;
  }

  if (!CloseHandle(readHandle)) {
    OutputDebugString("cmd: CloseHandle(readHandle) failed!\n");
  }
  if (!CloseHandle(writeHandle)) {
    OutputDebugString("cmd: CloseHandle(writeHandle) failed!\n");
  }

  return 1;
}

inline void ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
          }));
}

inline void rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](unsigned char ch) { return !std::isspace(ch); })
              .base(),
          s.end());
}

inline void trim(std::string &s) {
  rtrim(s);
  ltrim(s);
}

std::string ReplaceAll(std::string str, const std::string &from,
                       const std::string &to) {
  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos +=
        to.length(); // Handles case where 'to' is a substring of 'from'
  }
  return str;
}

std::string trim_copy(std::string s) {
  trim(s);
  return s;
}

std::vector<std::string> WinePath::split(const std::string &s, char delim) {
  std::vector<std::string> result;
  std::stringstream ss(s);
  std::string item;

  while (getline(ss, item, delim)) {
    result.push_back(item);
  }

  return result;
}

std::string WinePath::getUnixFilePath(std::string windowsFilePath) {
  std::string unixCmd = fmt::format("winepath -u \"{}\"",
                                    ReplaceAll(windowsFilePath, "%20", " "));
  char unixPathBuffer[4098];

  cmd((char *)unixCmd.c_str(), unixPathBuffer, 4098);

  geode::log::info("Converting {} to Unix File path result: {}",
                   windowsFilePath, std::string(unixPathBuffer));

  return trim_copy(std::string(unixPathBuffer));
}

std::string WinePath::getWindowsFilePath(std::string unixFilePath) {
  std::string winCmd =
      fmt::format("winepath -w \"{}\"", ReplaceAll(unixFilePath, "%20", " "));
  char winPathBuffer[4098];

  cmd((char *)winCmd.c_str(), winPathBuffer, 4098);

  geode::log::info("Converting {} to Windows File path result: {}",
                   unixFilePath, std::string(winPathBuffer));

  return trim_copy(std::string(winPathBuffer));
}


std::map<std::basic_string<TCHAR>, std::basic_string<TCHAR>> WinePath::get_envs() {
    typedef std::basic_string<TCHAR> tstring; // Generally convenient
    std::map<tstring, tstring> env;

    auto free = [](LPTCH p) { FreeEnvironmentStrings(p); };
    auto env_block = std::unique_ptr<TCHAR, decltype(free)>{
            GetEnvironmentStrings(), free};

    for (LPTCH i = env_block.get(); *i != TCHAR('\0'); ++i) {
        tstring key;
        tstring value;

        for (; *i != TCHAR('='); ++i)
            key += *i;
        ++i;
        for (; *i != TCHAR('\0'); ++i)
            value += *i;

        env[key] = value;
    }

    return env;
}


std::string WinePath::getUnixHome() {
    auto envs = WinePath::getInstance()->get_envs();
	auto wine_username = envs["WINEUSERNAME"];

	geode::log::info("Wine username: {}", wine_username);

    return "/home/" + wine_username;
}

std::string WinePath::getUnixHomeInWindows() {
  return getWindowsFilePath(getUnixHome());
}