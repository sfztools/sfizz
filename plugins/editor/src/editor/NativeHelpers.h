// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

bool openFileInExternalEditor(const char *filename);
bool openDirectoryInExplorer(const char *filename);
bool askQuestion(const char *text);

#if !defined(_WIN32) && !defined(__APPLE__)
bool isKdialogAvailable();
bool isZenityAvailable();
#endif
