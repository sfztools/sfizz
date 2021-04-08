// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "NativeHelpers.h"

#if defined(_WIN32)
#include "ghc/fs_std.hpp"
#include <windows.h>
#include <cstring>

static WCHAR *stringToWideChar(const char *str, int strCch = -1)
{
    unsigned strSize = MultiByteToWideChar(CP_UTF8, 0, str, strCch, nullptr, 0);
    if (strSize == 0)
        return {};
    std::unique_ptr<WCHAR[]> strW(new WCHAR[strSize]);
    if (MultiByteToWideChar(CP_UTF8, 0, str, strCch, strW.get(), strSize) == 0)
        return {};
    return strW.release();
}

static char* stringToUTF8(const wchar_t *strW, int strWCch = -1)
{
    unsigned strSize = WideCharToMultiByte(CP_UTF8, 0, strW, strWCch, nullptr, 0, nullptr, nullptr);
    if (strSize == 0)
        return {};
    std::unique_ptr<char[]> str(new char[strSize]);
    if (WideCharToMultiByte(CP_UTF8, 0, strW, strWCch, str.get(), strSize, nullptr, nullptr) == 0)
        return {};
    return str.release();
}

bool openFileInExternalEditor(const char *filename)
{
    std::wstring path = stringToWideChar(filename);

    SHELLEXECUTEINFOW info;
    memset(&info, 0, sizeof(info));

    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_CLASSNAME;
    info.lpVerb = L"open";
    info.lpFile = path.c_str();
    info.lpClass = L"txtfile";
    info.nShow = SW_SHOW;

    return ShellExecuteExW(&info);
}

bool openDirectoryInExplorer(const char *filename)
{
    std::wstring path = stringToWideChar(filename);

    SHELLEXECUTEINFOW info;
    memset(&info, 0, sizeof(info));

    info.cbSize = sizeof(info);
    info.lpVerb = L"explore";
    info.lpFile = path.c_str();
    info.nShow = SW_SHOW;

    return ShellExecuteExW(&info);
}

bool openURLWithExternalProgram(const char *url)
{
    std::wstring path = stringToWideChar(url);

    SHELLEXECUTEINFOW info;
    memset(&info, 0, sizeof(info));

    info.cbSize = sizeof(info);
    info.lpVerb = L"open";
    info.lpFile = path.c_str();
    info.nShow = SW_SHOW;

    return ShellExecuteExW(&info);
}

bool askQuestion(const char *text)
{
    int ret = MessageBoxW(nullptr, stringToWideChar(text), L"Question", MB_YESNO);
    return ret == IDYES;
}

std::string getOperatingSystemName()
{
    LSTATUS status;
    HKEY key = nullptr;
    const WCHAR keyPath[] = L"Software\\Microsoft\\Windows NT\\CurrentVersion";
    const WCHAR valueName[] = L"ProductName";
    const char fallbackName[] = "Windows (unknown)";

    status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_QUERY_VALUE, &key);
    if (status != ERROR_SUCCESS)
        return fallbackName;

    DWORD valueSize = 32768 * sizeof(WCHAR);
    std::unique_ptr<WCHAR[]> valueW(new WCHAR[(valueSize / sizeof(WCHAR)) + 1]());
    DWORD valueType;
    status = RegQueryValueExW(
        key, valueName, nullptr,
        &valueType, reinterpret_cast<LPBYTE>(valueW.get()), &valueSize);
    RegCloseKey(key);
    if (status != ERROR_SUCCESS || (valueType != REG_SZ && valueType != REG_EXPAND_SZ))
        return fallbackName;

    std::unique_ptr<char[]> valueUTF8(stringToUTF8(valueW.get()));
    return valueUTF8.get();
}

std::string getProcessorName()
{
    LSTATUS status;
    HKEY key = nullptr;
    const WCHAR keyPath[] = L"Hardware\\Description\\System\\CentralProcessor\\0";
    const WCHAR valueName[] = L"ProcessorNameString";
    const char fallbackName[] = "Unknown";

    status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_QUERY_VALUE, &key);
    if (status != ERROR_SUCCESS)
        return fallbackName;

    DWORD valueSize = 32768 * sizeof(WCHAR);
    std::unique_ptr<WCHAR[]> valueW(new WCHAR[(valueSize / sizeof(WCHAR)) + 1]());
    DWORD valueType;
    status = RegQueryValueExW(
        key, valueName, nullptr,
        &valueType, reinterpret_cast<LPBYTE>(valueW.get()), &valueSize);
    RegCloseKey(key);
    if (status != ERROR_SUCCESS || (valueType != REG_SZ && valueType != REG_EXPAND_SZ))
        return fallbackName;

    std::unique_ptr<char[]> valueUTF8(stringToUTF8(valueW.get()));
    return valueUTF8.get();
}

std::string getCurrentProcessName()
{
    DWORD size = 32768;
    std::unique_ptr<WCHAR[]> buffer(new WCHAR[size]());

    if (!GetModuleFileNameW(nullptr, buffer.get(), size))
        return {};

    buffer[size - 1] = L'\0';
    const WCHAR* name = buffer.get();

    if (const WCHAR* pos = wcsrchr(name, L'\\'))
        name = pos + 1;

    std::unique_ptr<char[]> nameUTF8(stringToUTF8(name));
    return nameUTF8.get();
}
#elif defined(__APPLE__)
    // implemented in NativeHelpers.mm
#else
#include <gio/gio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <vector>
#include <regex>
#include <fstream>
#include <cstring>
#include <cerrno>
extern "C" { extern char **environ; }
extern "C" { extern char *__progname; }

static bool openFileByMimeType(const char *filename, const char *mimetype)
{
    GAppInfo* appinfo = g_app_info_get_default_for_type(mimetype, FALSE);
    if (!appinfo)
        return 1;

    GList* files = nullptr;
    GFile* file = g_file_new_for_path(filename);
    files = g_list_append(files, file);
    gboolean success = g_app_info_launch(appinfo, files, nullptr, nullptr);
    g_object_unref(file);
    g_list_free(files);
    g_object_unref(appinfo);
    return success == TRUE;
}

bool openFileInExternalEditor(const char *filename)
{
    return openFileByMimeType(filename, "text/plain");
}

bool openDirectoryInExplorer(const char *filename)
{
    return openFileByMimeType(filename, "inode/directory");
}

bool openURLWithExternalProgram(const char *url)
{
    gboolean success = g_app_info_launch_default_for_uri(url, nullptr, nullptr);
    return success == TRUE;
}

static std::vector<char *> createForkEnviron()
{
    std::vector<char *> newEnv;
    newEnv.reserve(256);
    for (char **envp = environ; *envp; ++envp) {
        // ensure the process will link with system libraries,
        // and not these from the Ardour bundle.
        if (strncmp(*envp, "LD_LIBRARY_PATH=", 16) == 0)
            continue;
        newEnv.push_back(*envp);
    }
    newEnv.push_back(nullptr);
    return newEnv;
}

static constexpr char zenityPath[] = "/usr/bin/zenity";

bool askQuestion(const char *text)
{
    char *argv[] = {
        const_cast<char *>(zenityPath),
        const_cast<char *>("--question"),
        const_cast<char *>("--text"),
        const_cast<char *>(text),
        nullptr,
    };

    std::vector<char *> newEnv = createForkEnviron();
    char **envp = newEnv.data();

    pid_t forkPid = vfork();
    if (forkPid == -1)
        return false;

    if (forkPid == 0) {
        execve(argv[0], argv, envp);
        _exit(1);
    }

    int wret;
    int wstatus;
    do {
        wret = waitpid(forkPid, &wstatus, 0);
    } while (wret == -1 && errno == EINTR);

    if (wret == -1 || !WIFEXITED(wstatus))
        return false;

    return WEXITSTATUS(wstatus) == 0;
}

bool isZenityAvailable()
{
    return access(zenityPath, X_OK) == 0;
}

std::string getOperatingSystemName()
{
    std::string name;

    std::ifstream in("/etc/os-release", std::ios::binary);
    if (!in)
        in = std::ifstream("/usr/lib/os-release", std::ios::binary);
    if (in) {
        std::string line;
        line.reserve(256);
        for (bool found = false; !found && std::getline(in, line); ) {
            const char prefix[] = "PRETTY_NAME=";
            size_t length = sizeof(prefix) - 1;
            found = line.size() >= length && !memcmp(line.data(), prefix, length);
            if (found) {
                if (char* value = g_shell_unquote(line.c_str() + length, nullptr)) {
                    name.assign(value);
                    g_free(value);
                }
            }
        }
        in.close();
    }

    if (name.empty()) {
        utsname un {};
        int ret = uname(&un);
        if (ret != -1 && un.sysname[0] != '\0')
            name.append(un.sysname);
        else {
            name.append("Unknown");
        }
        if (ret != -1 && un.release[0] != '\0') {
            name.push_back(' ');
            name.append(un.release);
        }
    }

    return name;
}

std::string getProcessorName()
{
    std::string name;
    std::string line;
    std::ifstream in("/proc/cpuinfo", std::ios::binary);
    std::regex re("^model name\\s*:\\s*(.*)");

    line.reserve(256);

    while (name.empty() && std::getline(in, line) && !line.empty()) {
        std::smatch match;
        if (std::regex_match(line, match, re))
            name = match[1];
    }

    if (name.empty())
        name = "Unknown";

    return name;
}

std::string getCurrentProcessName()
{
    std::string name;

    const std::string commPath = "/proc/" + std::to_string(getpid()) + "/comm";

    if (std::ifstream in { commPath, std::ios::binary }) {
        name.reserve(256);
        for (int c; (c = in.get()) != std::char_traits<char>::eof() && c != '\n'; )
            name.push_back(static_cast<unsigned char>(c));
    }

    if (name.empty()) {
        if (const char* progname = __progname)
            name.assign(progname);
    }

    return name;
}
#endif
