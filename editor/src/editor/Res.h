// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <string>

namespace Res {

std::string getPath(const char* relativePath);

void initializeRootPath(const char* rootPath);
void initializeRootPathFromCurrentModule(const char* pathSuffix);

} // namespace Res
