// Copyright (c) 2005 - 2019 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "saveBitmap.h"
#include "libsiedler2/Archiv.h"
#include "libsiedler2/ArchivItem_Bitmap_Raw.h"
#include "libsiedler2/ErrorCodes.h"
#include "libsiedler2/libsiedler2.h"
#include <stdexcept>

void saveBitmap(const libsiedler2::PixelBufferBGRA& buffer, const boost::filesystem::path& path)
{
    auto bmp = std::make_unique<libsiedler2::ArchivItem_Bitmap_Raw>();
    bmp->create(buffer);
    libsiedler2::Archiv archive;
    archive.push(std::move(bmp));
    if(int ec = libsiedler2::Write(path, archive))
        throw std::runtime_error(libsiedler2::getErrorString(ec));
}
