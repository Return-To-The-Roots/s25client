// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
