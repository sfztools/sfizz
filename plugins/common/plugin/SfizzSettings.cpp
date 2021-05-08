// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfizzSettings.h"
#include "NativeHelpers.h"
#include <memory>
#include <cstdlib>

std::string SfizzSettings::load_or(const char* key, absl::string_view defaultValue)
{
    absl::optional<std::string> optValue = load(key);
    return optValue ? *optValue : std::string(defaultValue);
}

#if defined(_WIN32)
#include <windows.h>

static HKEY openRegistryKey()
{
    LSTATUS status;
    HKEY root = HKEY_CURRENT_USER;
    HKEY parent = root;
    HKEY key = nullptr;
    for (const WCHAR* component : {L"Software", L"SFZTools", L"sfizz"}) {
        status = RegCreateKeyExW(
            parent, component, 0, nullptr,
            REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &key, nullptr);
        if (parent != root)
            RegCloseKey(parent);
        if (status != ERROR_SUCCESS)
            return nullptr;
        parent = key;
    }
    return key;
}

absl::optional<std::string> SfizzSettings::load(const char* name)
{
    std::unique_ptr<WCHAR[]> nameW { stringToWideChar(name) };
    if (!nameW)
        return {};

    HKEY key = openRegistryKey();
    if (!key)
        return {};

    WCHAR valueW[32768];
    DWORD valueSize = sizeof(valueW);
    DWORD valueType;
    LSTATUS status = RegQueryValueExW(
        key, nameW.get(), nullptr,
        &valueType, reinterpret_cast<BYTE*>(valueW), &valueSize);
    RegCloseKey(key);
    if (status != ERROR_SUCCESS || (valueType != REG_SZ && valueType != REG_EXPAND_SZ))
        return {};

    std::unique_ptr<char[]> value { stringToUTF8(valueW) };
    if (!value)
        return {};

    return std::string(value.get());
}

bool SfizzSettings::store(const char* name, absl::string_view value)
{
    std::unique_ptr<WCHAR[]> nameW { stringToWideChar(name) };
    std::unique_ptr<WCHAR[]> valueW { stringToWideChar(std::string(value).c_str()) };
    if (!nameW || !valueW)
        return false;

    HKEY key = openRegistryKey();
    if (!key)
        return {};

    LSTATUS status = RegSetValueExW(
        key, nameW.get(), 0, RRF_RT_REG_SZ,
        reinterpret_cast<const BYTE*>(valueW.get()),
        (wcslen(valueW.get()) + 1) * sizeof(WCHAR));
    RegCloseKey(key);

    return status == ERROR_SUCCESS;
}
#elif defined(__APPLE__)
    // implementation in SfizzSettings.mm
#else
#include <pugixml.hpp>
#include <ghc/fs_std.hpp>

static const fs::path getSettingsPath()
{
    const fs::path dirPath = getXdgConfigHome() / "SFZTools" / "sfizz";
    std::error_code ec;
    fs::create_directories(dirPath, ec);
    if (ec)
        return {};
    return dirPath / "settings.xml";
}

absl::optional<std::string> SfizzSettings::load(const char* key)
{
    const fs::path path = getSettingsPath();
    if (path.empty())
        return {};

    pugi::xml_document doc;
    if (!doc.load_file(path.c_str()))
        return {};

    pugi::xml_node root = doc.child("properties");
    if (!root)
        return {};

    pugi::xml_node entry = root.find_child_by_attribute("entry", "key", key);
    if (!entry)
        return {};

    return std::string(entry.text().get());
}

bool SfizzSettings::store(const char* key, absl::string_view value)
{
    const fs::path path = getSettingsPath();
    if (path.empty())
        return false;

    pugi::xml_document doc;
    doc.load_file(path.c_str());

    pugi::xml_node root = doc.child("properties");
    if (!root)
        root = doc.append_child("properties");

    pugi::xml_node entry = root.find_child_by_attribute("entry", "key", key);
    if (!entry) {
        entry = root.append_child("entry");
        entry.append_attribute("key").set_value(key);
    }
    entry.text().set(std::string(value).c_str());

    return doc.save_file(path.c_str());
}
#endif
