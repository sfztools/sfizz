// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <string>

bool openFileInExternalEditor(const char *filename);
bool openDirectoryInExplorer(const char *filename);
bool openURLWithExternalProgram(const char *url);
bool askQuestion(const char *text);
std::string getOperatingSystemName();
std::string getProcessorName();
std::string getCurrentProcessName();

#if !defined(_WIN32) && !defined(__APPLE__)
bool isZenityAvailable();
#endif
