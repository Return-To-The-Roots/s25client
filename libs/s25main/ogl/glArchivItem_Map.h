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

#include "libsiedler2/ArchivItem_Map.h"

namespace libsiedler2 {
class ArchivItem_Map_Header;
}

enum MapLayer
{
    MAP_ALTITUDE = 0,
    MAP_TERRAIN1 = 1,
    MAP_TERRAIN2 = 2,
    MAP_ROADS_OLD = 3,
    MAP_LANDSCAPE = 4,
    MAP_TYPE = 5,
    MAP_ANIMALS = 6,
    // 7 = Map_Unknown
    MAP_BQ = 8,
    // 9 = Map_Unknown
    // 10 = Map_Unknown
    MAP_RESOURCES = 11,
    MAP_SHADOWS = 12,
    MAP_LAKES = 13,
    MAP_RESERVATIONS = 14,
    MAP_OWNER = 15
};

class glArchivItem_Map : public libsiedler2::ArchivItem_Map
{
public:
    glArchivItem_Map();
    ~glArchivItem_Map() override;
    RTTR_CLONEABLE(glArchivItem_Map)

    /// lädt die Mapdaten aus einer Datei.
    int load(std::istream& file, bool only_header) override;
    /// Resets the data to the given archiv
    void load(const libsiedler2::ArchivItem_Map& map);

    /// liefert den Header der Map als konstantes Objekt zurück.
    const libsiedler2::ArchivItem_Map_Header& getHeader() const { return *header; }

    /// liefert einen Map-Layer zurück.
    const std::vector<unsigned char>& GetLayer(MapLayer type) const;
    /// liefert einen Map-Layer zurück.
    std::vector<unsigned char>& GetLayer(MapLayer type);
    bool HasLayer(MapLayer type) const;

    /// liefert die Mapdaten an einer bestimmten Stelle zurück.
    unsigned char GetMapDataAt(MapLayer type, unsigned pos) const;
    /// setzt die Mapdaten an einer bestimmten Stelle.
    void SetMapDataAt(MapLayer type, unsigned pos, unsigned char value);

    /// liefert die Mapdaten an der Stelle X,Y zurück.
    unsigned char GetMapDataAt(MapLayer type, unsigned short x, unsigned short y) const;
    /// setzt die Mapdaten an der Stelle X,Y.
    void SetMapDataAt(MapLayer type, unsigned short x, unsigned short y, unsigned char value);

private:
    const libsiedler2::ArchivItem_Map_Header* header;
};
