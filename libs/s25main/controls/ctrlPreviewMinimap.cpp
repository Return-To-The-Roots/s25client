// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlPreviewMinimap.h"
#include "libsiedler2/ArchivItem_Map.h"
#include "libsiedler2/ArchivItem_Map_Header.h"

ctrlPreviewMinimap::Player::Player() : pos(0, 0), color(0) {}

ctrlPreviewMinimap::ctrlPreviewMinimap(Window* parent, const unsigned id, const DrawPoint& pos, const Extent& size,
                                       libsiedler2::ArchivItem_Map* s2map)
    : ctrlMinimap(parent, id, pos, size, Extent(2, 2), Extent(0, 0)), minimap(nullptr)
{
    setMap(s2map);
}

/**
 *  Zeichnet die MapPreview
 */
void ctrlPreviewMinimap::drawImpl()
{
    if(mapSize == Extent::all(0))
        return;

    // Button drumrum zeichnen
    draw3DBorder(getBoundaryRect(), TextureColor::Grey, true);

    // Map ansich zeichnen
    drawMap(minimap);

    const Extent playerPxlSize(4, 4);
    const DrawPoint basePos = getDrawPos();
    // Startpositionen zeichnen
    for(const Player& player : players)
    {
        // Spieler anwesend?
        if(player.pos.isValid())
            drawRectangle(Rect(basePos + calcMapCoord(player.pos), playerPxlSize), player.color);
    }
}

Rect ctrlPreviewMinimap::getBoundaryRect() const
{
    Rect borderRect = Rect::move(getMapDrawArea(), -DrawPoint(padding));
    borderRect.setSize(borderRect.getSize() + padding * 2u);
    return borderRect;
}

void ctrlPreviewMinimap::setMap(const libsiedler2::ArchivItem_Map* const s2map)
{
    for(auto& player : players)
        player.pos = MapPoint::invalid();
    if(!s2map)
    {
        setMapSize(Extent::all(0));
        return;
    }

    const unsigned short map_width = s2map->getHeader().getWidth();
    const unsigned short map_height = s2map->getHeader().getHeight();
    setMapSize(Extent(map_width, map_height));
    minimap.setMap(*s2map);

    // Startpositionen merken
    for(unsigned short y = 0; y < map_height; ++y)
    {
        for(unsigned short x = 0; x < map_width; ++x)
        {
            using libsiedler2::MapLayer;
            // Startposition eines Spielers an dieser Stelle?
            if(s2map->getMapDataAt(MapLayer::ObjectType, x, y) != libsiedler2::OT_HeadquarterMask)
                continue;
            const unsigned player = s2map->getMapDataAt(MapLayer::ObjectIndex, x, y);
            if(player < players.size())
            {
                players[player].pos = MapPoint(x, y);
                players[player].color = PLAYER_COLORS[player % PLAYER_COLORS.size()];
            }
        }
    }
}
