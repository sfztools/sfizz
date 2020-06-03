// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "FileDialog.h"
#include <windows.h>
#include <ghc/fs_std.hpp>
#include <memory>
#include <cstring>
#include <cwchar>

static std::wstring utf8ToWide(const std::string& str)
{
    size_t max = str.size();
    std::unique_ptr<wchar_t[]> wide { new wchar_t[max + 1] };
    wide[max] = L'\0';
    MultiByteToWideChar(CP_UTF8, 0, str.data(), -1, wide.get(), max);
    return wide.get();
}

static std::string wideToUtf8(const std::wstring& strW)
{
    size_t max = strW.size() * sizeof(wchar_t);
    std::unique_ptr<char[]> utf8 { new char[max + 1] };
    utf8[max] = '\0';
    WideCharToMultiByte(CP_UTF8, 0, strW.data(), -1, utf8.get(), max, nullptr, nullptr);
    return utf8.get();
}

std::string FileDialog::chooseFile()
{
    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);

    std::wstring titleW;
    if (!title_.empty()) {
        titleW = utf8ToWide(title_);
        ofn.lpstrTitle = titleW.c_str();
    }

    std::wstring pathW;
    if (!path_.empty()) {
        pathW = utf8ToWide(path_);
        ofn.lpstrInitialDir = pathW.c_str();
    }

    size_t fileNameMax = 32768;
    std::unique_ptr<wchar_t[]> fileNameW { new wchar_t[fileNameMax] };
    fileNameW[0] = L'\0';
    ofn.lpstrFile = fileNameW.get();
    ofn.nMaxFile = fileNameMax;

    std::wstring filtersW;
    if (!filters_.empty()) {
        for (const Filter& filter : filters_) {
            filtersW.append(utf8ToWide(filter.name));
            filtersW.push_back(L'\0');
            for (size_t i = 0, n = filter.patterns.size(); i < n; ++i) {
                if (i > 0)
                    filtersW.push_back(L';');
                filtersW.append(utf8ToWide(filter.patterns[i]));
            }
            filtersW.push_back(L'\0');
        }
        filtersW.push_back(L'\0');
        ofn.lpstrFilter = filtersW.c_str();
    }

    BOOL success;
    if (mode_ == Mode::Save)
        success = GetSaveFileNameW(&ofn);
    else
        success = GetOpenFileNameW(&ofn);

    if (!success)
        return {};

    std::wstring resultW(fileNameW.get(), wcsnlen(fileNameW.get(), fileNameMax));
    return wideToUtf8(resultW);
}
