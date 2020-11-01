// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "NativeHelpers.h"
#include <stdexcept>

#if defined(_WIN32)
#include <windows.h>
#include <shlobj.h>

const fs::path& getUserDocumentsDirectory()
{
    static const fs::path directory = []() -> fs::path {
        std::unique_ptr<WCHAR[]> path(new WCHAR[32768]);
        if (SHGetFolderPathW(nullptr, CSIDL_PERSONAL|CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, path.get()) != S_OK)
            throw std::runtime_error("Cannot get the document directory.");
        return fs::path(path.get());
    }();
    return directory;
}
#elif defined(__APPLE__)
    // implemented in NativeHelpers.mm
#else
#include <glib.h>

const fs::path& getUserDocumentsDirectory()
{
    static const fs::path directory = []() -> fs::path {
        const gchar* path = g_get_user_special_dir(G_USER_DIRECTORY_DOCUMENTS);
        if (!path)
            throw std::runtime_error("Cannot get the document directory.");
        return fs::path(path);
    }();
    return directory;
}
#endif
