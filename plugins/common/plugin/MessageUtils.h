// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

namespace Messages {

/**
 * Simple matcher for message handling in O(N)
 * @param[in] pattern  Pattern to match, where '&' characters match positive integer numbers
 * @param[in] path     Path to match against the pattern
 * @param[out] indices Table which received the indices, with size >= the number of '&' in the pattern
 */
bool matchOSC(const char* pattern, const char* path, unsigned* indices);

} // namespace Messages
