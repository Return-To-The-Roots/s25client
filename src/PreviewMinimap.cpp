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
#include "PreviewMinimap.h"
#include "RttrConfig.h"
#include "files.h"
#include "lua/GameDataLoader.h"
#include "mygettext/mygettext.h"
#include "ogl/glArchivItem_Map.h"
#include "world/MapGeometry.h"
#include "gameData/MinimapConsts.h"
#include "gameData/TerrainDesc.h"
#include "gameData/WorldDescription.h"
#include "libsiedler2/ArchivItem_Map_Header.h"
#include "libutil/Log.h"

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
    RTTR_Assert(gfxSet < NUM_LTS);
    lt = Landscape(gfxSet);
    objects = s2map.GetLayer(MAP_TYPE);
    terrain1 = s2map.GetLayer(MAP_TERRAIN1);
    terrain2 = s2map.GetLayer(MAP_TERRAIN2);
    if(s2map.HasLayer(MAP_SHADOWS))
        shadows = s2map.GetLayer(MAP_SHADOWS);
    else
        CalcShadows(s2map.GetLayer(MAP_ALTITUDE));

    WorldDescription worldDesc;
    GameDataLoader gdLoader(worldDesc, RTTRCONFIG.ExpandPath(FILE_PATHS[1]) + "/world");
    if(!gdLoader.Load())
        LOG.write(_("Failed to load game data!"));
    else
    {
        for(DescIdx<TerrainDesc> t(0); t.value < worldDesc.terrain.size(); t.value++)
        {
            const TerrainDesc& ter = worldDesc.get(t);
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
        int r = GetRed(color) + shading;
        int g = GetGreen(color) + shading;
        int b = GetBlue(color) + shading;

        if(r < 0)
            r = 0;
        if(r > 255)
            r = 255;
        if(g < 0)
            g = 0;
        if(g > 255)
            g = 255;
        if(b < 0)
            b = 0;
        if(b > 255)
            b = 255;

        color = MakeColor(0xFF, unsigned(r), unsigned(g), unsigned(b));
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
