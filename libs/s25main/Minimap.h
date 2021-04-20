// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Rect.h"
#include "ogl/glArchivItem_Bitmap_Direct.h"
#include "gameTypes/MapCoordinates.h"

class Minimap
{
protected:
    /// Breite und Höhe der Map (in Knoten)
    MapExtent mapSize;

    /// Textur für die Map
    glArchivItem_Bitmap_Direct map;

public:
    Minimap() : mapSize(0, 0) {}
    Minimap(const MapExtent& mapSize);
    virtual ~Minimap() = default;

    /// Draw the minimap in the given rectangle (stretching if required)
    void Draw(const Rect& rect);

    /// Gibt Größe der Map zurück
    MapExtent GetMapSize() const { return mapSize; }

protected:
    unsigned GetMMIdx(const MapPoint pt) const
    {
        return static_cast<unsigned>(pt.y) * mapSize.x + static_cast<unsigned>(pt.x);
    }
    /// Variiert die übergebene Farbe zufällig in der Helligkeit
    static unsigned VaryBrightness(unsigned color, int range);
    /// Erstellt die Textur
    void CreateMapTexture();
    virtual unsigned CalcPixelColor(MapPoint pt, unsigned t) = 0;
    /// Zusätzliche Dinge, die die einzelnen Maps vor dem Zeichenvorgang zu tun haben
    virtual void BeforeDrawing();
};
