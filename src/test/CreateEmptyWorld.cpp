// Copyright (c) 2005 - 2016 Settlers Freaks (sf-team at siedler25.org)
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
#include "CreateEmptyWorld.h"
#include "world/GameWorldGame.h"
#include "world/MapLoader.h"

CreateEmptyWorld::CreateEmptyWorld(unsigned width, unsigned height, unsigned numPlayers):
    width_(width), height_(height), playerNations_(numPlayers, NAT_ROMANS)
{}

bool CreateEmptyWorld::operator()(GameWorldGame& world) const
{
    world.Init(width_, height_, LT_GREENLAND);
    // Set everything to meadow
    for(MapPoint pt(0,0); pt.y < height_; ++pt.y)
    {
        for(pt.x = 0; pt.x < width_; ++pt.x)
        {
            MapNode& node = world.GetNodeWriteable(pt);
            node.t1 = node.t2 = TT_MEADOW1;
        }
    }
    if(!playerNations_.empty())
    {
        // Distribute player HQs evenly across map
        unsigned numPlayers = playerNations_.size();
        Point<unsigned> numPlayersPerDim;
        numPlayersPerDim.x = static_cast<unsigned>(sqrt(numPlayers));
        numPlayersPerDim.y = numPlayers / numPlayersPerDim.x;
        // Distance between HQs
        Point<unsigned> playerDist = Point<unsigned>(width_ / numPlayersPerDim.x, height_ / numPlayersPerDim.y);
        // Start with a little offset so we don't place them at the map border
        MapPoint curPt(playerDist / 2);
        std::vector<MapPoint> hqPositions;
        for(unsigned y = 0; y < numPlayersPerDim.y; y++)
        {
            for (unsigned x = 0; x < numPlayersPerDim.x; x++)
            {
                hqPositions.push_back(curPt);
                curPt.x += playerDist.x;
            }
            curPt.x = playerDist.x / 2;
            curPt.y += playerDist.y;
        }
        if(!MapLoader::PlaceHQs(world, hqPositions, playerNations_, false))
            return false;
    }
    return true;
}
