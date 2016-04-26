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
#ifndef MINIMAP_H_
#define MINIMAP_H_

#include "ogl/glArchivItem_Bitmap_Direct.h"
#include "gameTypes/MapTypes.h"
#include <vector>

class glArchivItem_Map;

class Minimap
{
protected:
    /// Breite und Höhe der Map (in Knoten)
    unsigned short map_width, map_height;

    /// Textur für die Map
    glArchivItem_Bitmap_Direct map;

public:
    Minimap(): map_width(0), map_height(0){}
    Minimap(const unsigned short map_width, const unsigned short map_height);
    virtual ~Minimap() {}

    virtual void SetMap(const glArchivItem_Map& s2map);

    /// Zeichnet die Minimap zentriert in die entsprechende Bounding-Box
    /// (x und y bezieht sich auf obere linke Ecke der Bounding Box)
    void Draw(const unsigned short x, const unsigned short y, const unsigned short width, const unsigned short height);

    /// Gibt Größe der Map zurück
    unsigned short GetMapWidth() const { return map_width; }
    unsigned short GetMapHeight() const { return map_height; }

protected:
    unsigned GetMMIdx(const MapPoint pt) const { return static_cast<unsigned>(pt.y) * static_cast<unsigned>(map_width) + static_cast<unsigned>(pt.x); }
    /// Variiert die übergebene Farbe zufällig in der Helligkeit
    unsigned VaryBrightness(const unsigned color, const int range) const;
    /// Erstellt die Textur
    void CreateMapTexture();
    virtual unsigned CalcPixelColor(const MapPoint pt, const unsigned t) = 0;
    /// Zusätzliche Dinge, die die einzelnen Maps vor dem Zeichenvorgang zu tun haben
    virtual void BeforeDrawing();
};

#endif

