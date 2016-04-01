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
#include "ctrlPreviewMinimap.h"
#include "ogl/glArchivItem_Map.h"
#include "libsiedler2/src/ArchivItem_Map_Header.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep
class Window;

ctrlPreviewMinimap::Player::Player() : x(0), y(0), color(0)
{
}

ctrlPreviewMinimap::ctrlPreviewMinimap(Window* parent,
                                       const unsigned int id,
                                       const unsigned short x,
                                       const unsigned short y,
                                       unsigned short width,
                                       unsigned short height,
                                       glArchivItem_Map* s2map) :
    ctrlMinimap(parent, id, x, y, width, height, 2, 2, width, height), minimap(NULL)
{
    SetMap(s2map);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Zeichnet die MapPreview
 *
 *  @author OLiver
 */
bool ctrlPreviewMinimap::Draw_()
{
    // Button drumrum zeichnen
    Draw3D(GetX() + GetLeft() - 2, GetY() + GetTop() - 2, GetWidthShow() + 4, GetHeightShow() + 4, TC_GREY, 0, false, false);

    // Map ansich zeichnen
    DrawMap(minimap);

    // Startpositionen zeichnen
    for(unsigned i = 0; i < MAX_PLAYERS; ++i)
    {
        // Spieler anwesend?
        if(players[i].color)
        {
            // Zeichnen
            DrawRectangle(GetX() + CalcMapCoordX(players[i].x) - 2,
                          GetY() + CalcMapCoordY(players[i].y) - 2, 4, 4, players[i].color);
        }
    }

    return true;
}

void ctrlPreviewMinimap::SetMap(const glArchivItem_Map* const s2map)
{
    if(!s2map)
        return;

    unsigned short map_width = s2map->getHeader().getWidth();
    unsigned short map_height = s2map->getHeader().getHeight();
    SetDisplaySize(width_, height_, map_width, map_height);
    minimap.SetMap(*s2map);
    for(unsigned i = 0; i < MAX_PLAYERS; i++)
        players[i].color = 0;

    // Startpositionen merken
    for(unsigned short y = 0; y < map_height; ++y)
    {
        for(unsigned short x = 0; x < map_width; ++x)
        {
            // Startposition eines Spielers an dieser Stelle?
            if(s2map->GetMapDataAt(MAP_TYPE, x, y) != 0x80)
                continue;
            unsigned player = s2map->GetMapDataAt(MAP_LANDSCAPE, x, y);
            if(player < MAX_PLAYERS)
            {
                players[player].x = x;
                players[player].y = y;
                players[player].color = PLAYER_COLORS[player % PLAYER_COLORS.size()];
            }
        }
    }
}
