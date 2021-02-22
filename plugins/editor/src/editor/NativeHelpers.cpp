// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "NativeHelpers.h"

#if defined(_WIN32)
#include "ghc/fs_std.hpp"
#include <windows.h>
#include <cstring>

bool openFileInExternalEditor(const char *filename)
{
    std::wstring path = fs::u8path(filename).wstring();

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
    std::wstring path = fs::u8path(filename).wstring();

    SHELLEXECUTEINFOW info;
    memset(&info, 0, sizeof(info));

    info.cbSize = sizeof(info);
    info.lpVerb = L"explore";
    info.lpFile = path.c_str();
    info.nShow = SW_SHOW;

    return ShellExecuteExW(&info);
}
#elif defined(__APPLE__)
    // implemented in NativeHelpers.mm
#else
#include <gio/gio.h>

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
#endif
