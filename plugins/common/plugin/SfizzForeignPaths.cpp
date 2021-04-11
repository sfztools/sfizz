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
    std::unique_ptr<WCHAR[]> nameW;
    unsigned nameSize = MultiByteToWideChar(CP_UTF8, 0, name, -1, nullptr, 0);
    if (nameSize == 0)
        return {};
    nameW.reset(new WCHAR[nameSize]);
    if (MultiByteToWideChar(CP_UTF8, 0, name, -1, nameW.get(), nameSize) == 0)
        return {};

    const WCHAR ariaKeyPath[] = L"Software\\Plogue Art et Technologie, Inc\\Aria";

    HKEY key = nullptr;
    LSTATUS status = RegOpenKeyExW(HKEY_CURRENT_USER, ariaKeyPath, 0, KEY_QUERY_VALUE, &key);
    if (status != ERROR_SUCCESS)
        return {};

    DWORD valueSize = 32768 * sizeof(WCHAR);
    std::unique_ptr<WCHAR[]> valueW(new WCHAR[(valueSize / sizeof(WCHAR)) + 1]());
    DWORD valueType;
    status = RegQueryValueExW(
        key, nameW.get(), nullptr,
        &valueType, reinterpret_cast<LPBYTE>(valueW.get()), &valueSize);
    RegCloseKey(key);
    if (status != ERROR_SUCCESS || (valueType != REG_SZ && valueType != REG_EXPAND_SZ))
        return {};

    return fs::path(valueW.get());
}
#elif defined(__APPLE__)
    // implementation in SfizzForeignPaths.mm
#else
fs::path getAriaPathSetting(const char* name)
{
    (void)name;
    return {};
}
#endif

} // namespace SfizzPaths
