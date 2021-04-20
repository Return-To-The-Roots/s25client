// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Minimap.h"
#include "RttrForeachPt.h"
#include "libsiedler2/PixelBufferBGRA.h"

Minimap::Minimap(const MapExtent& mapSize) : mapSize(mapSize) {}

void Minimap::CreateMapTexture()
{
    map.DeleteTexture();

    /// Buffer für die Daten erzeugen
    libsiedler2::PixelBufferBGRA buffer(mapSize.x * 2, mapSize.y);

    RTTR_FOREACH_PT(MapPoint, mapSize)
    {
        // Die 2. Terraindreiecke durchgehen
        for(unsigned t = 0; t < 2; ++t)
        {
            libsiedler2::ColorBGRA color(CalcPixelColor(pt, t));
            unsigned xCoord = (pt.x * 2 + t + (pt.y & 1)) % buffer.getWidth();
            buffer.set(xCoord, pt.y, color);
        }
    }

    map.setInterpolateTexture(false);
    map.create(buffer);
}

void Minimap::Draw(const Rect& rect)
{
    BeforeDrawing();
    if(rect.getSize().x > 0u && rect.getSize().y > 0)
        map.DrawFull(rect);
}

void Minimap::BeforeDrawing() {}

/**
 *  Variiert die übergebene Farbe zufällig in der Helligkeit
 */
unsigned Minimap::VaryBrightness(const unsigned color, const int range)
{
    int add = 100 - rand() % (2 * range);

    int red = GetRed(color) * add / 100;
    if(red < 0)
        red = 0;
    else if(red > 0xFF)
        red = 0xFF;
    int green = GetGreen(color) * add / 100;
    if(green < 0)
        green = 0;
    else if(green > 0xFF)
        green = 0xFF;
    int blue = GetBlue(color) * add / 100;
    if(blue < 0)
        blue = 0;
    else if(blue > 0xFF)
        blue = 0xFF;

    return MakeColor(GetAlpha(color), red, green, blue);
}
