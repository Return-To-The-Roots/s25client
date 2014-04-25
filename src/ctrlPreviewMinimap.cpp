// $Id: ctrlPreviewMinimap.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "ctrlPreviewMinimap.h"

#include "glArchivItem_Map.h"
#include "MinimapConsts.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
ctrlPreviewMinimap::Player::Player() : x(0), y(0), color(0)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
ctrlPreviewMinimap::ctrlPreviewMinimap(Window* parent,
                                       const unsigned int id,
                                       const unsigned short x,
                                       const unsigned short y,
                                       unsigned short width,
                                       unsigned short height,
                                       glArchivItem_Map* s2map) :
    ctrlMinimap(parent, id, x, y, width, height, 2, 2, s2map ? s2map->getHeader().getWidth() : width, s2map ? s2map->getHeader().getHeight() : height),
    minimap(s2map)
{
    if(s2map)
    {
        unsigned short map_width = s2map->getHeader().getWidth();
        unsigned short map_height = s2map->getHeader().getHeight();

        // Startpositionen merken
        for(unsigned short y = 0; y < map_height; ++y)
        {
            for(unsigned short x = 0; x < map_width; ++x)
            {
                // Startposition eines Spielers an dieser Stelle?
                if(s2map->GetMapDataAt(MAP_TYPE, x, y) == 0x80)
                {
                    unsigned player = s2map->GetMapDataAt(MAP_LANDSCAPE, x, y);
                    assert(player < MAX_PLAYERS);
                    players[player].x = x;
                    players[player].y = y;
                }
            }
        }
    }
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
