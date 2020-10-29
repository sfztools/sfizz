// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfizzForeignPaths.h"

namespace SfizzPaths {

#if defined(_WIN32)
#include <windows.h>

fs::path getAriaPathSetting(const char* name)
{
    fs::path path;

    HKEY key = 0;

    std::unique_ptr<WCHAR[]> nameW;
    unsigned nameSize = MultiByteToWideChar(CP_UTF8, 0, name, -1, nullptr, 0);
    if (nameSize == 0)
        return {};
    nameW.reset(new WCHAR[nameSize]);
    if (MultiByteToWideChar(CP_UTF8, 0, name, -1, nameW.get(), nameSize) == 0)
        return {};

    const WCHAR ariaKeyPath[] = L"Software\\Plogue Art et Technologie, Inc\\Aria";

    if (RegOpenKeyExW(HKEY_CURRENT_USER, ariaKeyPath, 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS) {
        WCHAR valueBuffer[32768 + 1];
        DWORD valueSize = sizeof(valueBuffer) - sizeof(WCHAR);
        if (RegQueryValueExW(key, nameW.get(), nullptr, nullptr, reinterpret_cast<LPBYTE>(valueBuffer), &valueSize) == ERROR_SUCCESS) {
            valueBuffer[32768] = L'\0';
            path = fs::path(valueBuffer);
        }
        RegCloseKey(key);
    }

    return path;
}
#elif defined(__APPLE__)
    // implementation in SfizzForeignPaths.mm
#else
fs::path getAriaPathSetting(const char* name)
{
    return {};
}
#endif

} // namespace SfizzPaths
