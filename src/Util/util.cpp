

#include <cstring>
#include "util.h"
#include "File.h"
#include "local_time.h"

#if defined(_WIN32)
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
extern "C" const IMAGE_DOS_HEADER __ImageBase;
#endif // defined(_WIN32)

using namespace std;

namespace FFZKit {

    string exePath(bool isExe) {
        char buffer[PATH_MAX * 2 + 1] = { 0 };
        int n = -1;
#if defined(_WIN32)
        n = GetModuleFileNameA(isExe?nullptr:(HINSTANCE)&__ImageBase, buffer, sizeof(buffer));
#elif defined(__MACH__) || defined(__APPLE__)
        n = sizeof(buffer);
        if (uv_exepath(buffer, &n) != 0) {
            n = -1;
        }
#elif defined(__linux__)
        n = readlink("/proc/self/exe", buffer, sizeof(buffer));
#endif

        string filePath;
        if(n <= 0) {
            filePath = "./";
        }
        else {
            filePath = buffer;
        }
#if defined(_WIN32)
        //windows下把路径统一转换层unix风格，因为后续都是按照unix风格处理的
        for(auto &ch : filePath) {
            if (ch == '\\') {
                ch = '/';
            }
        }
#endif // defined(_WIN32)

        return filePath;
    }

    string exeDir(bool isExe) {
        auto path = exePath(isExe);
        return path.substr(0, path.rfind('/')+ 1);
    }

    string exeName(bool isExe) {
        auto path = exePath(isExe);
        return path.substr(path.rfind('/')+ 1);
    }

    vector<string> split(const string &s, const char *delim) {
        vector<string> ret;
        size_t last = 0;
        auto index = s.find(delim, last);
        while (index != string::npos) {
            if (index - last > 0) {
                ret.push_back(s.substr(last, index - last));
            }
            last = index + strlen(delim);
            index = s.find(delim, last);
        }
        if (!s.size() || s.size() - last > 0) {
            ret.push_back(s.substr(last));
        }
        return ret;
    }

    struct tm getLocalTime(time_t sec) {
        struct tm tm;
#ifdef _WIN32
        localtime_s(&tm, &sec);
#else
        no_locks_localtime(&tm, sec);
#endif //_WIN32
        return tm;
    }


    string getThreadName() {
#if ((defined(__linux) || defined(__linux__)) && !defined(ANDROID)) || (defined(__MACH__) || defined(__APPLE__)) || (defined(ANDROID) && __ANDROID_API__ >= 26) || defined(__MINGW32__)
        string ret;
        ret.resize(64);
        auto tid = pthread_self();
        pthread_getname_np(tid, (char *) ret.data(), ret.size());
        if (ret[0]) {
            ret.resize(strlen(ret.data()));
            return ret;
        }
        return to_string((uint64_t) tid);
#elif defined(_MSC_VER)
        using GetThreadDescriptionFunc = HRESULT(WINAPI * )(_In_ HANDLE hThread, _In_ PWSTR * ppszThreadDescription);
    static auto getThreadDescription = reinterpret_cast<GetThreadDescriptionFunc>(::GetProcAddress(::GetModuleHandleA("Kernel32.dll"), "GetThreadDescription"));

    if (!getThreadDescription) {
        std::ostringstream ss;
        ss << std::this_thread::get_id();
        return ss.str();
    } else {
        PWSTR data;
        HRESULT hr = getThreadDescription(GetCurrentThread(), &data);
        if (SUCCEEDED(hr) && data[0] != '\0') {
            char threadName[MAX_PATH];
            size_t numCharsConverted;
            errno_t charResult = wcstombs_s(&numCharsConverted, threadName, data, MAX_PATH - 1);
            if (charResult == 0) {
                LocalFree(data);
                std::ostringstream ss;
                ss << threadName;
                return ss.str();
            } else {
                if (data) {
                    LocalFree(data);
                }
                return to_string((uint64_t) GetCurrentThreadId());
            }
        } else {
            if (data) {
                LocalFree(data);
            }
            return to_string((uint64_t) GetCurrentThreadId());
        }
    }
#else
    if (!thread_name.empty()) {
        return thread_name;
    }
    std::ostringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
#endif
    }


} // namespace FFZKit