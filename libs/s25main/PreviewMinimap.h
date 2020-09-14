// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "Minimap.h"
#include <map>

class glArchivItem_Map;

class PreviewMinimap : public Minimap
{
    std::vector<unsigned char> objects, terrain1, terrain2, shadows;
    std::map<uint8_t, uint32_t> terrain2Clr;

public:
    explicit PreviewMinimap(const glArchivItem_Map* s2map);

    void SetMap(const glArchivItem_Map& s2map);

protected:
    /// Berechnet die Farbe für einen bestimmten Pixel der Minimap (t = Terrain1 oder 2)
    unsigned CalcPixelColor(MapPoint pt, unsigned t) override;

private:
    unsigned char CalcShading(MapPoint pt, const std::vector<unsigned char>& altitudes) const;
    void CalcShadows(const std::vector<unsigned char>& altitudes);
};
