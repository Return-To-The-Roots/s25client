// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "rttrDefines.h" // IWYU pragma: keep
#include "ctrlPreviewMinimap.h"
#include "ogl/glArchivItem_Map.h"
#include "libsiedler2/ArchivItem_Map_Header.h"

ctrlPreviewMinimap::Player::Player() : pos(0, 0), color(0) {}

ctrlPreviewMinimap::ctrlPreviewMinimap(Window* parent, const unsigned id, const DrawPoint& pos, const Extent& size, glArchivItem_Map* s2map)
    : ctrlMinimap(parent, id, pos, size, Extent(2, 2), Extent(0, 0)), minimap(NULL)
{
    SetMap(s2map);
}

/**
 *  Zeichnet die MapPreview
 */
void ctrlPreviewMinimap::Draw_()
{
    // Button drumrum zeichnen
    Draw3D(GetBoundaryRect(), TC_GREY, 0, false, false);

    // Map ansich zeichnen
    DrawMap(minimap);

    Extent playerPxlSize(4, 4);
    const DrawPoint basePos = GetDrawPos();
    // Startpositionen zeichnen
    for(unsigned i = 0; i < MAX_PLAYERS; ++i)
    {
        // Spieler anwesend?
        if(players[i].color)
            DrawRectangle(Rect(basePos + CalcMapCoord(players[i].pos), playerPxlSize), players[i].color);
    }
}

Rect ctrlPreviewMinimap::GetBoundaryRect() const
{
    Rect borderRect = Rect::move(GetMapDrawArea(), -DrawPoint(padding));
    borderRect.setSize(borderRect.getSize() + padding * 2u);
    return borderRect;
}

void ctrlPreviewMinimap::SetMap(const glArchivItem_Map* const s2map)
{
    if(!s2map)
    {
        SetMapSize(Extent::all(0));
        return;
    }

    unsigned short map_width = s2map->getHeader().getWidth();
    unsigned short map_height = s2map->getHeader().getHeight();
    SetMapSize(Extent(map_width, map_height));
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
                players[player].pos = MapPoint(x, y);
                players[player].color = PLAYER_COLORS[player % PLAYER_COLORS.size()];
            }
        }
    }
}
