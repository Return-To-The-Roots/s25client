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

#include "glArchivItem_Map.h"
#include "RTTR_Assert.h"
#include "libsiedler2/ArchivItem_Map_Header.h"
#include "libsiedler2/ArchivItem_Raw.h"

glArchivItem_Map::glArchivItem_Map() : header(nullptr) {}

glArchivItem_Map::~glArchivItem_Map() = default;

/**
 *  lädt die Mapdaten aus einer Datei.
 *
 *  @param[in] file Dateihandle der Datei
 *  @param[in] only_header Soll nur der Header gelesen werden?
 *
 *  @return liefert Null bei Erfolg, ungleich Null bei Fehler
 */
int glArchivItem_Map::load(std::istream& file, bool only_header)
{
    if(int ec = libsiedler2::ArchivItem_Map::load(file, only_header))
        return ec;

    header = dynamic_cast<const libsiedler2::ArchivItem_Map_Header*>(get(0));
    RTTR_Assert(header);

    return 0;
}

void glArchivItem_Map::load(const libsiedler2::ArchivItem_Map& map)
{
    static_cast<libsiedler2::Archiv&>(*this) = map;
    extraInfo = map.extraInfo;
    header = dynamic_cast<const libsiedler2::ArchivItem_Map_Header*>(get(0));
    RTTR_Assert(header);
}

/**
 *  liefert einen Map-Layer zurück.
 *
 *  @param[in] type Typ des Layers.
 */
const std::vector<unsigned char>& glArchivItem_Map::GetLayer(MapLayer type) const
{
    RTTR_Assert(HasLayer(type));
    const auto* item = dynamic_cast<const libsiedler2::ArchivItem_Raw*>(get(type + 1)); // 0 = header
    RTTR_Assert(item);
    return item->getData(); //-V522
}

/**
 *  liefert einen Map-Layer zurück.
 *
 *  @param[in] type Typ des Layers.
 */
std::vector<unsigned char>& glArchivItem_Map::GetLayer(MapLayer type)
{
    RTTR_Assert(HasLayer(type));
    auto* item = dynamic_cast<libsiedler2::ArchivItem_Raw*>(get(type + 1)); // 0 = header
    RTTR_Assert(item);
    return item->getData(); //-V522
}

bool glArchivItem_Map::HasLayer(MapLayer type) const
{
    return get(type + 1) != nullptr;
}

/**
 *  liefert die Mapdaten an einer bestimmten Stelle zurück.
 *
 *  @param[in] type Typ des Layers.
 *  @param[in] pos  Position in den Daten.
 */
unsigned char glArchivItem_Map::GetMapDataAt(MapLayer type, unsigned pos) const
{
    return GetLayer(type)[pos];
}

/**
 *  setzt die Mapdaten an einer bestimmten Stelle.
 *
 *  @param[in] type  Typ des Layers.
 *  @param[in] pos   Position in den Daten.
 *  @param[in] value zu setzender Wert an der Position.
 */
void glArchivItem_Map::SetMapDataAt(MapLayer type, unsigned pos, unsigned char value)
{
    GetLayer(type)[pos] = value;
}

/**
 *  liefert die Mapdaten an der Stelle X,Y zurück.
 *
 *  @param[in] type Typ des Layers.
 *  @param[in] x    X-Position in den Daten.
 *  @param[in] y    Y-Position in den Daten.
 */
unsigned char glArchivItem_Map::GetMapDataAt(MapLayer type, unsigned short x, unsigned short y) const
{
    return GetMapDataAt(type, y * header->getWidth() + x);
}

/**
 *  setzt die Mapdaten an der Stelle X,Y.
 *
 *  @param[in] type  Typ des Layers.
 *  @param[in] x     X-Position in den Daten.
 *  @param[in] y     Y-Position in den Daten.
 *  @param[in] value zu setzender Wert an der Position.
 */
void glArchivItem_Map::SetMapDataAt(MapLayer type, unsigned short x, unsigned short y, unsigned char value)
{
    SetMapDataAt(type, y * header->getWidth() + x, value);
}
