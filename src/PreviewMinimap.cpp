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
#include "PreviewMinimap.h"
#include "ogl/glArchivItem_Map.h"
#include "world/MapGeometry.h"
#include "gameData/MinimapConsts.h"
#include "gameData/TerrainData.h"
#include "libsiedler2/src/ArchivItem_Map_Header.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

PreviewMinimap::PreviewMinimap(const glArchivItem_Map* const s2map)
{
    if(s2map)
        SetMap(*s2map);
}

void PreviewMinimap::SetMap(const glArchivItem_Map& s2map)
{
    map_width = s2map.getHeader().getWidth();
    map_height = s2map.getHeader().getHeight();

    unsigned char gfxSet = s2map.getHeader().getGfxSet();
    RTTR_Assert(gfxSet < LT_COUNT);
    lt = LandscapeType(gfxSet);
    objects = s2map.GetLayer(MAP_TYPE);
    terrain1 = s2map.GetLayer(MAP_TERRAIN1);
    terrain2 = s2map.GetLayer(MAP_TERRAIN2);
    if(s2map.HasLayer(MAP_SHADOWS))
        shadows = s2map.GetLayer(MAP_SHADOWS);
    else
        CalcShadows(s2map.GetLayer(MAP_ALTITUDE));

    CreateMapTexture();
}

///////////////////////////////////////////////////////////////////////////////
/**
*
*  @author OLiver
*/
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
        color = TerrainData::GetColor(lt, TerrainData::MapIdx2Terrain(t == 0 ? terrain1[GetMMIdx(pt)] : terrain2[GetMMIdx(pt)]));

        // Schattierung
        const int shading = shadows[GetMMIdx(pt)] - 0x40;
        int r = GetRed(color) + shading;
        int g = GetGreen(color) + shading;
        int b = GetBlue(color) + shading;

        if(r < 0) r = 0;
        if(r > 255) r = 255;
        if(g < 0) g = 0;
        if(g > 255) g = 255;
        if(b < 0) b = 0;
        if(b > 255) b = 255;

        color = MakeColor(0xFF, unsigned(r), unsigned(g), unsigned(b));
    }

    return color;
}

unsigned char PreviewMinimap::CalcShading(const MapPoint pt, const std::vector<unsigned char>& altitudes) const
{
    int altitude = altitudes[GetMMIdx(pt)];
    MapPoint tmp = MakeMapPoint(GetNeighbour(Point<int>(pt), Direction::NORTHEAST), map_width, map_height);
    int A = altitudes[GetMMIdx(tmp)] - altitude;
    tmp = MakeMapPoint(GetNeighbour2(Point<int>(pt), 0), map_width, map_height);
    int B = altitudes[GetMMIdx(tmp)] - altitude;
    tmp = MakeMapPoint(GetNeighbour(Point<int>(pt), Direction::WEST), map_width, map_height);
    int C = altitudes[GetMMIdx(tmp)] - altitude;
    tmp = MakeMapPoint(GetNeighbour2(Point<int>(pt), 7), map_width, map_height);
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
    shadows.resize(map_width * map_height);
    for(MapPoint pt(0, 0); pt.y < map_height; ++pt.y)
    {
        for(pt.x = 0; pt.x < map_width; ++pt.x)
            shadows[GetMMIdx(pt)] = CalcShading(pt, altitudes);
    }
}
