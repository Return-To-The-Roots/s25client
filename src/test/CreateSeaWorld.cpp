// Copyright (c) 2016 Settlers Freaks (sf-team at siedler25.org)
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
#include "CreateSeaWorld.h"
#include "world/GameWorldGame.h"
#include "world/MapLoader.h"
#include "test/testHelpers.h"
#include <boost/foreach.hpp>

CreateSeaWorld::CreateSeaWorld(unsigned width, unsigned height, unsigned numPlayers):
    width_(width), height_(height), playerNations_(numPlayers, NAT_ROMANS)
{}

namespace{
    bool PlaceHarbor(MapPoint pt, GameWorldBase& world, std::vector<MapPoint>& harbors)
    {
        // Get all points within a radius of 3 and place the harbor on the first possible place
        std::vector<MapPoint> pts = world.GetPointsInRadius(pt, 3);
        BOOST_FOREACH(MapPoint curPt, pts)
        {
            // Harbor only at castles
            world.RecalcBQ(curPt);
            if(world.GetNode(curPt).bq != BQ_CASTLE)
                continue;
            // We must have a coast around
            for(unsigned i = 0; i < 6; i++)
            {
                MapPoint posCoastPt = world.GetNeighbour(curPt, i);
                // Coast must not be water
                if(world.IsWaterPoint(posCoastPt))
                    continue;
                // But somewhere around must be a sea
                for(unsigned j = 0; j < 6; j++)
                {
                    MapPoint posSeaPt = world.GetNeighbour(posCoastPt, j);
                    if(world.IsSeaPoint(posSeaPt))
                    {
                        harbors.push_back(curPt);
                        return true;
                    }
                }
            }
        }
        return false;
    }
}

bool CreateSeaWorld::operator()(GameWorldGame& world) const
{
    // For consistent results
    doInitGameRNG(0);

    world.Init(width_, height_, LT_GREENLAND);
    // Set everything to water
    RTTR_FOREACH_PT(MapPoint, width_, height_)
    {
        MapNode& node = world.GetNodeWriteable(pt);
        node.t1 = node.t2 = TT_WATER;
    }
    /* We create an outlined square of land in the water so it looks like:
     * WWWWWWWWWWWWWWWWWWWWWWW  Height of water: Offset
     * WWWWWWWWWWWWWWWWWWWWWWW
     * WWLLLLLLLLLLLLLLLLLLLWW  Height of land: landSize
     * WWLLLLLLLLLLLLLLLLLLLWW  Width of water (left and right): offset (on each side)
     * WWLLWWWWWWWWWWWWWWWLLWW  Width of land: landSize (on each side)
     * WWLLWWWWWWWWWWWWWWWLLWW
     * WWLLWWWWWWWWWWWWWWWLLWW
     * WWLLWWWWWWWWWWWWWWWLLWW
     * WWLLWWWWWWWWWWWWWWWLLWW
     * WWLLLLLLLLLLLLLLLLLLLWW
     * WWLLLLLLLLLLLLLLLLLLLWW
     * WWWWWWWWWWWWWWWWWWWWWWW  Height of water: Offset
     * WWWWWWWWWWWWWWWWWWWWWWW
     */
     // Init some land stripes of size 16 (a bit less than the HQ radius)
    const MapCoord offset = 7;
    const MapCoord landSize = 16;
    // We need the offset at each side, the land on each side
    // and at least the same amount of water between the land
    const MapCoord minSize = landSize * 3 + offset * 2;
    if(width_ < minSize || height_ < minSize)
        throw std::runtime_error("World to small");

    // Vertical
    for(MapPoint pt(offset, offset); pt.y < width_ - offset; ++pt.y)
    {
        for(pt.x = offset; pt.x < offset + landSize; ++pt.x)
        {
            MapNode& node = world.GetNodeWriteable(pt);
            node.t1 = node.t2 = TT_MEADOW1;
        }
        for(pt.x = width_ - offset - landSize; pt.x < width_ - offset; ++pt.x)
        {
            MapNode& node = world.GetNodeWriteable(pt);
            node.t1 = node.t2 = TT_MEADOW1;
        }
    }
    // Horizontal
    for(MapPoint pt(offset, offset); pt.x < width_ - offset; ++pt.x)
    {
        for(pt.y = offset; pt.y < offset + landSize; ++pt.y)
        {
            MapNode& node = world.GetNodeWriteable(pt);
            node.t1 = node.t2 = TT_MEADOW1;
        }
        for(pt.y = height_ - offset - landSize; pt.y < height_ - offset; ++pt.y)
        {
            MapNode& node = world.GetNodeWriteable(pt);
            node.t1 = node.t2 = TT_MEADOW1;
        }
    }

    // Place HQs at top, left, right, bottom
    std::vector<MapPoint> hqPositions;
    hqPositions.push_back(MapPoint(width_ / 2, offset + landSize / 2));
    hqPositions.push_back(MapPoint(offset + landSize / 2, height_ / 2));
    hqPositions.push_back(MapPoint(width_ - offset - landSize / 2, height_ / 2));
    hqPositions.push_back(MapPoint(width_ / 2, height_ - offset - landSize / 2));

    std::vector<MapPoint> harbors;
    // Place harbors
    BOOST_FOREACH(MapPoint pt, hqPositions)
    {
        unsigned harborsPlaced = 0;
        if(PlaceHarbor(pt - MapPoint(landSize / 2, 0), world, harbors))
            ++harborsPlaced;
        if(PlaceHarbor(pt + MapPoint(landSize / 2, 0), world, harbors))
            ++harborsPlaced;
        if(PlaceHarbor(pt - MapPoint(0, landSize / 2), world, harbors))
            ++harborsPlaced;
        if(PlaceHarbor(pt + MapPoint(0, landSize / 2), world, harbors))
            ++harborsPlaced;
        // Exactly 2 harbors should be placed per HQ (left and right or above and below)
        RTTR_Assert(harborsPlaced == 2);
    }

    MapLoader::InitSeasAndHarbors(world, harbors);

    if(!MapLoader::PlaceHQs(world, hqPositions, playerNations_, false))
        return false;
    world.InitAfterLoad();
    return true;
}


CreateWaterWorld::CreateWaterWorld(unsigned width, unsigned height, unsigned numPlayers): width_(width), height_(height), playerNations_(numPlayers, NAT_ROMANS)
{
    // Only 2 players supported
    RTTR_Assert(numPlayers == 2u);
}

bool CreateWaterWorld::operator()(GameWorldGame& world) const
{
    world.Init(width_, height_, LT_GREENLAND);
    // Set everything to water
    RTTR_FOREACH_PT(MapPoint, width_, height_)
    {
        MapNode& node = world.GetNodeWriteable(pt);
        node.t1 = node.t2 = TT_WATER;
    }
    // Create some land
    for(MapPoint pt(0, 0); pt.y < 10; ++pt.y)
    {
        for(pt.x = 0; pt.x < 10; ++pt.x)
        {
            MapNode& node = world.GetNodeWriteable(pt);
            node.t1 = node.t2 = TT_MEADOW1;
        }
    }
    std::vector<MapPoint> hqPositions;
    hqPositions.push_back(MapPoint(5, 9));
    hqPositions.push_back(MapPoint(9, 9));
    MapLoader::PlaceHQs(world, hqPositions, playerNations_, false);

    std::vector<MapPoint> harbors;
    PlaceHarbor(MapPoint(5, 0), world, harbors);
    RTTR_Assert(harbors.size() == 1u);
    MapLoader::InitSeasAndHarbors(world, harbors);
    return true;
}
