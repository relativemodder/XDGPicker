#include "winepath.hpp"


WinePath* WinePath::getInstance() {
    static WinePath* instance;
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
   
    ZeroMemory(&sa,sizeof(SECURITY_ATTRIBUTES));
    ZeroMemory(&pi,sizeof(PROCESS_INFORMATION));
    ZeroMemory(&si,sizeof(STARTUPINFO));
   
    sa.bInheritHandle=true;
    sa.lpSecurityDescriptor=NULL;
    sa.nLength=sizeof(SECURITY_ATTRIBUTES);
    si.cb=sizeof(STARTUPINFO);
    si.dwFlags=STARTF_USESHOWWINDOW;
    si.wShowWindow=SW_HIDE;
 
    if (!CreatePipe(&readHandle,&writeHandle,&sa,NULL))
    {
        OutputDebugString("cmd: CreatePipe failed!\n");
        return 0;
    }
 
    stdHandle=GetStdHandle(STD_OUTPUT_HANDLE);
 
    if (!SetStdHandle(STD_OUTPUT_HANDLE,writeHandle))
    {
        OutputDebugString("cmd: SetStdHandle(writeHandle) failed!\n");
        return 0;
    }
 
    if (!CreateProcess(NULL,cmd,NULL,NULL,TRUE,0,NULL,NULL,&si,&pi))
    {
        OutputDebugString("cmd: CreateProcess failed!\n");
        return 0;
    }
 
    GetExitCodeProcess(pi.hProcess,&retCode);
    while (retCode==STILL_ACTIVE)
    {
        GetExitCodeProcess(pi.hProcess,&retCode);
    }
 
    if (!ReadFile(readHandle,output,maxbuffer,&bytesRead,NULL))
    {
        OutputDebugString("cmd: ReadFile failed!\n");
        return 0;
    }
    output[bytesRead]='\0';
 
    if (!SetStdHandle(STD_OUTPUT_HANDLE,stdHandle))
    {
        OutputDebugString("cmd: SetStdHandle(stdHandle) failed!\n");
        return 0;
    }
 
    if (!CloseHandle(readHandle))
    {
        OutputDebugString("cmd: CloseHandle(readHandle) failed!\n");
    }
    if (!CloseHandle(writeHandle))
    {
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
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}


inline void trim(std::string &s) {
    rtrim(s);
    ltrim(s);
}


std::string trim_copy(std::string s) {
    trim(s);
    return s;
}


std::vector<std::string> WinePath::split (const std::string &s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss (s);
    std::string item;

    while (getline (ss, item, delim)) {
        result.push_back (item);
    }

    return result;
}


std::string WinePath::getUnixFilePath(std::string windowsFilePath) {
    geode::log::info("Converting {} to Unix File path", windowsFilePath);
	std::string unixCmd = fmt::format("winepath -u \"{}\"", windowsFilePath);
	char unixPathBuffer[4098];

	cmd((char* )unixCmd.c_str(), unixPathBuffer, 4098);

	return trim_copy(std::string(unixPathBuffer));
}


std::string WinePath::getWindowsFilePath(std::string unixFilePath) {
    std::string winCmd = fmt::format("winepath -w \"{}\"", unixFilePath);
	char winPathBuffer[4098];

	cmd((char* )winCmd.c_str(), winPathBuffer, 4098);

	return trim_copy(std::string(winPathBuffer));
}