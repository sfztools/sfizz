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

bool askQuestion(const char *text)
{
    int ret = MessageBoxW(nullptr, stringToWideChar(text), L"Question", MB_YESNO);
    return ret == IDYES;
}
#elif defined(__APPLE__)
    // implemented in NativeHelpers.mm
#else
#include <gio/gio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <cstring>
#include <cerrno>
extern "C" { extern char **environ; }

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

static constexpr char kdialogPath[] = "/usr/bin/kdialog";
static constexpr char zenityPath[] = "/usr/bin/zenity";

bool askQuestion(const char *text)
{
    char* const kdialogArgs[] = {
        const_cast<char *>(kdialogPath),
        const_cast<char *>("--yesno"),
        const_cast<char *>(text),
        nullptr,
    };

    char* const zenityArgs[] = {
        const_cast<char *>(zenityPath),
        const_cast<char *>("--question"),
        const_cast<char *>("--text"),
        const_cast<char *>(text),
        nullptr,
    };

    char *const *args;
    if (isKdialogAvailable())
        args = kdialogArgs;
    else if (isZenityAvailable())
        args = zenityArgs;
    else
        return false;

    std::vector<char *> newEnv = createForkEnviron();
    char **envp = newEnv.data();

    pid_t forkPid = vfork();
    if (forkPid == -1)
        return false;

    if (forkPid == 0) {
        execve(args[0], args, envp);
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

bool isKdialogAvailable()
{
    return access(kdialogPath, X_OK) == 0;
}

bool isZenityAvailable()
{
    return access(zenityPath, X_OK) == 0;
}
#endif
