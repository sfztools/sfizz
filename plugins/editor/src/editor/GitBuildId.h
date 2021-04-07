// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

extern "C" {

/**
 * @brief Short identifier of the current head commit.
 * This generated identifier is empty if the build is not from a Git repository.
 */
extern const char* GitBuildId;

} // extern "C"
