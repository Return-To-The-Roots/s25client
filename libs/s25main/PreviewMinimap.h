// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Minimap.h"
#include <map>

namespace libsiedler2 {
class ArchivItem_Map;
}

class PreviewMinimap : public Minimap
{
    std::vector<unsigned char> objects, terrain1, terrain2, shadows;
    std::map<uint8_t, uint32_t> terrain2Clr;

public:
    explicit PreviewMinimap(const libsiedler2::ArchivItem_Map* s2map);

    void SetMap(const libsiedler2::ArchivItem_Map& s2map);

protected:
    /// Berechnet die Farbe für einen bestimmten Pixel der Minimap (t = Terrain1 oder 2)
    unsigned CalcPixelColor(MapPoint pt, unsigned t) override;

private:
    unsigned char CalcShading(MapPoint pt, const std::vector<unsigned char>& altitudes) const;
    void CalcShadows(const std::vector<unsigned char>& altitudes);
};
