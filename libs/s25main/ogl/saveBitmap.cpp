// Copyright (c) 2005 - 2019 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

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
