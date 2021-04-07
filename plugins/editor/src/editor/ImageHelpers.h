// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <ghc/fs_std.hpp>

#include "utility/vstgui_before.h"
#include "vstgui/vstgui.h"
#include "utility/vstgui_after.h"

/**
 * @brief Loads a bitmap from an image file, with a large support of formats
 * through the stb_image library.
 */
VSTGUI::SharedPointer<VSTGUI::CBitmap> loadAnyFormatImage(const fs::path& filePath);

/**
 * @brief Adjust the scale factor of this bitmap, such that both its dimensions
 * fit into a frame of the given size.
 */
void downscaleToWidthAndHeight(VSTGUI::CBitmap* bitmap, VSTGUI::CPoint frameSize);
