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

#include "PreviewMinimap.h"
#include "RttrForeachPt.h"
#include "helpers/mathFuncs.h"
#include "lua/GameDataLoader.h"
#include "mygettext/mygettext.h"
#include "ogl/glArchivItem_Map.h"
#include "world/MapGeometry.h"
#include "gameData/MinimapConsts.h"
#include "gameData/TerrainDesc.h"
#include "gameData/WorldDescription.h"
#include "libsiedler2/ArchivItem_Map_Header.h"
#include "s25util/Log.h"

PreviewMinimap::PreviewMinimap(const glArchivItem_Map* const s2map)
{
    if(s2map)
        SetMap(*s2map);
}

void PreviewMinimap::SetMap(const glArchivItem_Map& s2map)
{
    const libsiedler2::ArchivItem_Map_Header& header = s2map.getHeader();
    mapSize.x = header.getWidth();
    mapSize.y = header.getHeight();

    unsigned char gfxSet = header.getGfxSet();
    objects = s2map.GetLayer(MAP_TYPE);
    terrain1 = s2map.GetLayer(MAP_TERRAIN1);
    terrain2 = s2map.GetLayer(MAP_TERRAIN2);
    if(s2map.HasLayer(MAP_SHADOWS))
        shadows = s2map.GetLayer(MAP_SHADOWS);
    else
        CalcShadows(s2map.GetLayer(MAP_ALTITUDE));

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
    MapPoint tmp = MakeMapPoint(GetNeighbour(Position(pt), Direction::NORTHEAST), GetMapSize());
    int A = altitudes[GetMMIdx(tmp)] - altitude;
    tmp = MakeMapPoint(GetNeighbour2(Position(pt), 0), GetMapSize());
    int B = altitudes[GetMMIdx(tmp)] - altitude;
    tmp = MakeMapPoint(GetNeighbour(Position(pt), Direction::WEST), GetMapSize());
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
