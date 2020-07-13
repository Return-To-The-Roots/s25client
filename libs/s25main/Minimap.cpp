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
