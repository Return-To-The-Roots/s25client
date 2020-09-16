// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#pragma once

#include "DrawPoint.h"
#include "libsiedler2/ArchivItem_Bob.h"

/// Klasse für GL-Bobfiles.
class glArchivItem_Bob : public libsiedler2::ArchivItem_Bob
{
public:
    /// Zeichnet einen Animationsstep.
    void Draw(unsigned item, libsiedler2::ImgDir direction, bool fat, unsigned animationstep, DrawPoint drawPt,
              unsigned color);
    RTTR_CLONEABLE(glArchivItem_Bob)

    void mergeLinks(const std::map<uint16_t, uint16_t>& overrideLinks);
};
