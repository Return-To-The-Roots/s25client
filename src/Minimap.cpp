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

#include "defines.h" // IWYU pragma: keep
#include "Minimap.h"
#include "Loader.h"
#include "ogl/glArchivItem_Map.h"
#include "libsiedler2/src/ArchivItem_Map_Header.h"
#include "ogl/oglIncludes.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

Minimap::Minimap(const unsigned short map_width, const unsigned short map_height)
    : map_width(map_width), map_height(map_height)
{}

void Minimap::SetMap(const glArchivItem_Map& s2map)
{
    map_width = s2map.getHeader().getWidth();
    map_height = s2map.getHeader().getHeight();
    CreateMapTexture();
}

void Minimap::CreateMapTexture()
{
    map.DeleteTexture();

    /// Buffer für die Daten erzeugen
    unsigned char* buffer = new unsigned char[map_width * 2 * map_height * 4];

    for(MapCoord y = 0; y < map_height; ++y)
    {
        for(MapCoord x = 0; x < map_width; ++x)
        {
            // Die 2. Terraindreiecke durchgehen
            for(unsigned t = 0; t < 2; ++t)
            {
                unsigned color = CalcPixelColor(MapPoint(x, y), t);

                unsigned pos  = y * map_width * 4 * 2 + (x * 4 * 2 + t * 4 + (y & 1) * 4) % (map_width * 4 * 2);
                buffer[pos + 2] = GetRed(color);
                buffer[pos + 1] = GetGreen(color);
                buffer[pos]   = GetBlue(color);
                buffer[pos + 3] = GetAlpha(color);
            }
        }
    }

    map.setFilter(GL_LINEAR);
    map.create(map_width * 2, map_height, buffer, map_width * 2, map_height,
               libsiedler2::FORMAT_RGBA, LOADER.GetPaletteN("pal5"));

    delete [] buffer;
}

void Minimap::Draw(const unsigned short x, const unsigned short y, const unsigned short width, const unsigned short height)
{
    BeforeDrawing();

    // Map ansich zeichnen
    map.Draw(x, y, width, height, 0, 0, 0, 0, COLOR_WHITE);
}

void Minimap::BeforeDrawing()
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Variiert die übergebene Farbe zufällig in der Helligkeit
 *
 *  @author OLiver
 */
unsigned Minimap::VaryBrightness(const unsigned color, const int range) const
{
    int add = 100 - rand() % (2 * range);

    int red = GetRed(color) * add / 100;
    if(red < 0) red = 0;
    else if(red > 0xFF) red = 0xFF;
    int green = GetGreen(color) * add / 100;
    if(green < 0) green = 0;
    else if(green > 0xFF) green = 0xFF;
    int blue = GetBlue(color) * add / 100;
    if(blue < 0) blue = 0;
    else if(blue > 0xFF) blue = 0xFF;

    return MakeColor(GetAlpha(color), red, green, blue);
}
