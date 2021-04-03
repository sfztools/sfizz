// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "ImageHelpers.h"
#include "EditorLibs.h"
#include <cstdio>

using namespace VSTGUI;

struct stbi_image_delete {
    void operator()(unsigned char* x) const noexcept { stbi_image_free(x); }
};
using stbi_image_u = std::unique_ptr<unsigned char[], stbi_image_delete>;

SharedPointer<CBitmap> loadAnyFormatImage(const fs::path& filePath)
{
    stbi_image_u image;
    int width, height, channels;

#if defined(_WIN32)
    FILE* file { _wfopen(filePath.wstring().c_str(), L"rb") };
#else
    FILE* file { fopen(filePath.c_str(), "rb") };
#endif
    if (file) {
        image.reset(stbi_load_from_file(file, &width, &height, &channels, STBI_rgb_alpha));
        fclose(file);
    }

    if (!image)
        return nullptr;

    SharedPointer<CBitmap> bitmap = makeOwned<CBitmap>(width, height);
    SharedPointer<CBitmapPixelAccess> accessor =
        owned(CBitmapPixelAccess::create(bitmap.get()));

    if (!accessor)
        return nullptr;

    const unsigned char* pixel = image.get();
    do {
        CColor c(pixel[0], pixel[1], pixel[2], pixel[3]);
        accessor->setColor(c);
        pixel += 4;
    } while (++*accessor);
    accessor = nullptr;

    return bitmap;
}

void downscaleToWidthAndHeight(VSTGUI::CBitmap* bitmap, VSTGUI::CPoint frameSize)
{
    if (!bitmap)
        return;

    CCoord frameW = frameSize.x;
    CCoord frameH = frameSize.y;
    CCoord bitmapW = bitmap->getWidth();
    CCoord bitmapH = bitmap->getHeight();

    CCoord scale =  1.0;
    if (bitmapW > frameW || bitmapH > frameH) {
        CCoord xScale = bitmapW / frameW;
        CCoord yScale = bitmapH / frameH;
        scale = (xScale > yScale) ? xScale : yScale;
    }

    if (PlatformBitmapPtr platformBitmap = bitmap->getPlatformBitmap())
        platformBitmap->setScaleFactor(scale);
}