// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PreviewMinimap.h"
#include "RttrForeachPt.h"
#include "helpers/mathFuncs.h"
#include "lua/GameDataLoader.h"
#include "mygettext/mygettext.h"
#include "world/MapGeometry.h"
#include "gameData/MinimapConsts.h"
#include "gameData/TerrainDesc.h"
#include "gameData/WorldDescription.h"
#include "libsiedler2/ArchivItem_Map.h"
#include "libsiedler2/ArchivItem_Map_Header.h"
#include "s25util/Log.h"

PreviewMinimap::PreviewMinimap(const libsiedler2::ArchivItem_Map* const s2map)
{
    if(s2map)
        setMap(*s2map);
}

void PreviewMinimap::setMap(const libsiedler2::ArchivItem_Map& s2map)
{
    const libsiedler2::ArchivItem_Map_Header& header = s2map.getHeader();
    mapSize.x = header.getWidth();
    mapSize.y = header.getHeight();

    using libsiedler2::MapLayer;

    unsigned char gfxSet = header.getGfxSet();
    objects = s2map.getLayer(MapLayer::ObjectType);
    terrain1 = s2map.getLayer(MapLayer::Terrain1);
    terrain2 = s2map.getLayer(MapLayer::Terrain2);
    if(s2map.hasLayer(MapLayer::Shadows))
        shadows = s2map.getLayer(MapLayer::Shadows);
    else
        calcShadows(s2map.getLayer(MapLayer::Altitude));

    WorldDescription worldDesc;
    GameDataLoader gdLoader(worldDesc);
    if(!gdLoader.load())
        LOG.write(_("Failed to load game data!"));
    else
    {
        const auto lt = worldDesc.landscapes.find([gfxSet](const LandscapeDesc& l) { return l.s2Id == gfxSet; });
        for(const TerrainDesc& ter : worldDesc.terrain)
        {
            if(ter.landscape == lt)
                terrain2Clr[ter.s2Id] = ter.minimapColor;
        }
    }

    createMapTexture();
}

unsigned PreviewMinimap::calcPixelColor(const MapPoint pt, const unsigned t)
{
    unsigned color = 0;
    // Baum an dieser Stelle?
    unsigned char landscapeObj = objects[getMMIdx(pt)];
    if(landscapeObj >= 0xC4 && landscapeObj <= 0xC6)
        color = varyBrightness(TREE_COLOR, VARY_TREE_COLOR);
    // Granit an dieser Stelle?
    else if(landscapeObj == 0xCC || landscapeObj == 0xCD)
        color = varyBrightness(GRANITE_COLOR, VARY_GRANITE_COLOR);
    // Ansonsten die jeweilige Terrainfarbe nehmen
    else
    {
        color = terrain2Clr[t == 0 ? terrain1[getMMIdx(pt)] : terrain2[getMMIdx(pt)]];

        // Schattierung
        const int shading = shadows[getMMIdx(pt)] - 0x40;
        if(shading != 0)
        {
            const auto r = static_cast<int>(getRed(color)) + shading;
            const auto g = static_cast<int>(getGreen(color)) + shading;
            const auto b = static_cast<int>(getBlue(color)) + shading;
            using helpers::clamp;
            color = makeColor(0xFF, clamp(r, 0u, 255u), clamp(g, 0u, 255u), clamp(b, 0u, 255u));
        }
    }

    return color;
}

unsigned char PreviewMinimap::calcShading(const MapPoint pt, const std::vector<unsigned char>& altitudes) const
{
    int altitude = altitudes[getMMIdx(pt)];
    MapPoint tmp = makeMapPoint(getNeighbour(Position(pt), Direction::NorthEast), getMapSize());
    int A = altitudes[getMMIdx(tmp)] - altitude;
    tmp = makeMapPoint(getNeighbour2(Position(pt), 0), getMapSize());
    int B = altitudes[getMMIdx(tmp)] - altitude;
    tmp = makeMapPoint(getNeighbour(Position(pt), Direction::West), getMapSize());
    int C = altitudes[getMMIdx(tmp)] - altitude;
    tmp = makeMapPoint(getNeighbour2(Position(pt), 7), getMapSize());
    int D = altitudes[getMMIdx(tmp)] - altitude;

    int shadingS2 = 64 + 9 * A - 3 * B - 6 * C - 9 * D;
    if(shadingS2 > 128)
        shadingS2 = 128;
    else if(shadingS2 < 0)
        shadingS2 = 0;

    return shadingS2;
}

void PreviewMinimap::calcShadows(const std::vector<unsigned char>& altitudes)
{
    shadows.resize(altitudes.size());
    RTTR_FOREACH_PT(MapPoint, getMapSize())
        shadows[getMMIdx(pt)] = calcShading(pt, altitudes);
}
