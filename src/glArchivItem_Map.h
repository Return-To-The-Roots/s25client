// $Id: glArchivItem_Map.h 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef GLARCHIVITEM_MAP_H_INCLUDED
#define GLARCHIVITEM_MAP_H_INCLUDED

#pragma once

#include "main.h"

class SerializedGameData;

enum MapLayer
{
    // 0 = Map_Header
    // 1 = Map_Unknown
    MAP_ALTITUDE = 2,
    MAP_TERRAIN1 = 3,
    MAP_TERRAIN2 = 4,
    MAP_ROADS_OLD = 5,
    MAP_LANDSCAPE = 6,
    MAP_TYPE = 7,
    MAP_ANIMALS = 8,
    // 9 = Map_Unknown
    MAP_BQ = 10,
    // 11 = Map_Unknown
    // 12 = Map_Unknown
    MAP_RESOURCES = 13,
    MAP_SHADOWS = 14,
    MAP_LAKES = 15,
    MAP_RESERVATIONS = 16,
    MAP_OWNER = 17
};

class glArchivItem_Map : public libsiedler2::ArchivItem_Map
{
    public:
        /// Konstruktor von @p glArchivItem_Map.
        glArchivItem_Map(void);
        /// Kopierkonstruktor von @p glArchivItem_Map.
        glArchivItem_Map(const glArchivItem_Map* item);
        /// Destruktor von @p glArchivItem_Map.
        ~glArchivItem_Map(void);

        /// lädt die Mapdaten aus einer Datei.
        int load(FILE* file, bool only_header);

        void Serialize(SerializedGameData* sgd) const;
        void Deserialize(SerializedGameData* sgd, const char* const map_name);

        /// liefert den Header der Map als konstantes Objekt zurück.
        const libsiedler2::ArchivItem_Map_Header& getHeader(void) const { return *header; }

        /// liefert einen Map-Layer zurück.
        const unsigned char* GetLayer(MapLayer type) const;
        /// liefert einen Map-Layer zurück.
        unsigned char* GetLayer(MapLayer type);

        /// liefert die Mapdaten an einer bestimmten Stelle zurück.
        unsigned char GetMapDataAt(MapLayer type, unsigned int pos) const;
        /// setzt die Mapdaten an einer bestimmten Stelle.
        void SetMapDataAt(MapLayer type, unsigned int pos, unsigned char value);

        /// liefert die Mapdaten an der Stelle X,Y zurück.
        unsigned char GetMapDataAt(MapLayer type, unsigned short x, unsigned short y) const;
        /// setzt die Mapdaten an der Stelle X,Y.
        void SetMapDataAt(MapLayer type, unsigned short x, unsigned short y, unsigned char value);

    private:
        const libsiedler2::ArchivItem_Map_Header* header;

};

#endif // !GLARCHIVITEM_MAP_H_INCLUDED
