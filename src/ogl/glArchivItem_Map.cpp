// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h" // IWYU pragma: keep
#include "glArchivItem_Map.h"

#include "../libsiedler2/src/ArchivItem_Raw.h"
#include "libsiedler2/src/ArchivItem_Map_Header.h"
#include "glAllocator.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

glArchivItem_Map::glArchivItem_Map()
    : ArchivItem_Map(), header(NULL)
{
}

glArchivItem_Map::~glArchivItem_Map()
{
}


///////////////////////////////////////////////////////////////////////////////
/**
 *  lädt die Mapdaten aus einer Datei.
 *
 *  @param[in] file Dateihandle der Datei
 *  @param[in] only_header Soll nur der Header gelesen werden?
 *
 *  @return liefert Null bei Erfolg, ungleich Null bei Fehler
 *
 *  @author FloSoft
 */
int glArchivItem_Map::load(std::istream& file, bool only_header)
{
    if(libsiedler2::ArchivItem_Map::load(file, only_header) != 0)
        return 1;

    header = dynamic_cast<const libsiedler2::ArchivItem_Map_Header*>(get(0));
    RTTR_Assert(header);

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  liefert einen Map-Layer zurück.
 *
 *  @param[in] type Typ des Layers.
 *
 *  @author FloSoft
 */
const std::vector<unsigned char>& glArchivItem_Map::GetLayer(MapLayer type) const
{
    RTTR_Assert(HasLayer(type));
    const libsiedler2::ArchivItem_Raw* item = dynamic_cast<const libsiedler2::ArchivItem_Raw*>(get(type + 1)); // 0 = header
    RTTR_Assert(item);
    return item->getData();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  liefert einen Map-Layer zurück.
 *
 *  @param[in] type Typ des Layers.
 *
 *  @author FloSoft
 */
std::vector<unsigned char>& glArchivItem_Map::GetLayer(MapLayer type)
{
    RTTR_Assert(HasLayer(type));
    libsiedler2::ArchivItem_Raw* item = dynamic_cast<libsiedler2::ArchivItem_Raw*>(get(type + 1)); // 0 = header
    RTTR_Assert(item);
    return item->getData();
}

bool glArchivItem_Map::HasLayer(MapLayer type) const
{
    return get(type + 1) != NULL;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  liefert die Mapdaten an einer bestimmten Stelle zurück.
 *
 *  @param[in] type Typ des Layers.
 *  @param[in] pos  Position in den Daten.
 *
 *  @author FloSoft
 */
unsigned char glArchivItem_Map::GetMapDataAt(MapLayer type, unsigned int pos) const
{
    return GetLayer(type)[pos];
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  setzt die Mapdaten an einer bestimmten Stelle.
 *
 *  @param[in] type  Typ des Layers.
 *  @param[in] pos   Position in den Daten.
 *  @param[in] value zu setzender Wert an der Position.
 *
 *  @author FloSoft
 */
void glArchivItem_Map::SetMapDataAt(MapLayer type, unsigned int pos, unsigned char value)
{
    GetLayer(type)[pos] = value;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  liefert die Mapdaten an der Stelle X,Y zurück.
 *
 *  @param[in] type Typ des Layers.
 *  @param[in] x    X-Position in den Daten.
 *  @param[in] y    Y-Position in den Daten.
 *
 *  @author FloSoft
 */
unsigned char glArchivItem_Map::GetMapDataAt(MapLayer type, unsigned short x, unsigned short y) const
{
    return GetMapDataAt(type, y * header->getWidth() + x);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  setzt die Mapdaten an der Stelle X,Y.
 *
 *  @param[in] type  Typ des Layers.
 *  @param[in] x     X-Position in den Daten.
 *  @param[in] y     Y-Position in den Daten.
 *  @param[in] value zu setzender Wert an der Position.
 *
 *  @author FloSoft
 */
void glArchivItem_Map::SetMapDataAt(MapLayer type, unsigned short x, unsigned short y, unsigned char value)
{
    SetMapDataAt(type, y * header->getWidth() + x, value);
}
