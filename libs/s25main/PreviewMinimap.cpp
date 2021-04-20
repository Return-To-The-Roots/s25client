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
        SetMap(*s2map);
}

void PreviewMinimap::SetMap(const libsiedler2::ArchivItem_Map& s2map)
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
        CalcShadows(s2map.getLayer(MapLayer::Altitude));

    WorldDescription worldDesc;
    GameDataLoader gdLoader(worldDesc);
    if(!gdLoader.Load())
        LOG.write(_("Failed to load game data!"));
    else
    {
        DescIdx<LandscapeDesc> lt(0);
        for(DescIdx<LandscapeDesc> i(0); i.value < worldDesc.landscapes.size(); i.value++)
        {
            if(worldDesc.get(i).s2Id == gfxSet)
                lt = i;
        }
        for(DescIdx<TerrainDesc> i(0); i.value < worldDesc.terrain.size(); i.value++)
        {
            const TerrainDesc& ter = worldDesc.get(i);
            if(ter.landscape == lt)
                terrain2Clr[ter.s2Id] = ter.minimapColor;
        }
    }

    CreateMapTexture();
}

unsigned PreviewMinimap::CalcPixelColor(const MapPoint pt, const unsigned t)
{
    unsigned color = 0;
    // Baum an dieser Stelle?
    unsigned char landscape_obj = objects[GetMMIdx(pt)];
    if(landscape_obj >= 0xC4 && landscape_obj <= 0xC6)
        color = VaryBrightness(TREE_COLOR, VARY_TREE_COLOR);
    // Granit an dieser Stelle?
    else if(landscape_obj == 0xCC || landscape_obj == 0xCD)
        color = VaryBrightness(GRANITE_COLOR, VARY_GRANITE_COLOR);
    // Ansonsten die jeweilige Terrainfarbe nehmen
    else
    {
        color = terrain2Clr[t == 0 ? terrain1[GetMMIdx(pt)] : terrain2[GetMMIdx(pt)]];

        // Schattierung
        const int shading = shadows[GetMMIdx(pt)] - 0x40;
        if(shading != 0)
        {
            const auto r = static_cast<int>(GetRed(color)) + shading;
            const auto g = static_cast<int>(GetGreen(color)) + shading;
            const auto b = static_cast<int>(GetBlue(color)) + shading;
            using helpers::clamp;
            color = MakeColor(0xFF, clamp(r, 0u, 255u), clamp(g, 0u, 255u), clamp(b, 0u, 255u));
        }
    }

    return color;
}

unsigned char PreviewMinimap::CalcShading(const MapPoint pt, const std::vector<unsigned char>& altitudes) const
{
    int altitude = altitudes[GetMMIdx(pt)];
    MapPoint tmp = MakeMapPoint(GetNeighbour(Position(pt), Direction::NorthEast), GetMapSize());
    int A = altitudes[GetMMIdx(tmp)] - altitude;
    tmp = MakeMapPoint(GetNeighbour2(Position(pt), 0), GetMapSize());
    int B = altitudes[GetMMIdx(tmp)] - altitude;
    tmp = MakeMapPoint(GetNeighbour(Position(pt), Direction::West), GetMapSize());
    int C = altitudes[GetMMIdx(tmp)] - altitude;
    tmp = MakeMapPoint(GetNeighbour2(Position(pt), 7), GetMapSize());
    int D = altitudes[GetMMIdx(tmp)] - altitude;

    int shadingS2 = 64 + 9 * A - 3 * B - 6 * C - 9 * D;
    if(shadingS2 > 128)
        shadingS2 = 128;
    else if(shadingS2 < 0)
        shadingS2 = 0;

    return shadingS2;
}

void PreviewMinimap::CalcShadows(const std::vector<unsigned char>& altitudes)
{
    shadows.resize(altitudes.size());
    RTTR_FOREACH_PT(MapPoint, GetMapSize())
        shadows[GetMMIdx(pt)] = CalcShading(pt, altitudes);
}
