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

#pragma once

#include "World.h"
#include "gameData/TerrainDesc.h"

struct BQCalculator
{
    BQCalculator(const World& world) : world(world) {}

    using result_type = BuildingQuality;

    template<typename T_IsOnRoad>
    inline BuildingQuality operator()(MapPoint pt, T_IsOnRoad isOnRoad, bool flagOnly = false) const;

private:
    const World& world;
};

template<typename T_IsOnRoad>
BuildingQuality BQCalculator::operator()(const MapPoint pt, T_IsOnRoad isOnRoad, bool flagOnly /*= false*/) const
{
    // Cannot build on blocking objects
    if(world.GetNO(pt)->GetBM() != BlockingManner::None)
        return BQ_NOTHING;

    //////////////////////////////////////////////////////////////////////////
    // 1. Check maximum allowed BQ on terrain

    unsigned building_hits = 0;
    unsigned mine_hits = 0;
    unsigned flag_hits = 0;

    const WorldDescription& desc = world.GetDescription();
    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        TerrainBQ bq = desc.get(world.GetRightTerrain(pt, dir)).GetBQ();
        if(bq == TerrainBQ::CASTLE)
            ++building_hits;
        else if(bq == TerrainBQ::MINE)
            ++mine_hits;
        else if(bq == TerrainBQ::FLAG)
            ++flag_hits;
        else if(bq == TerrainBQ::DANGER)
            return BQ_NOTHING;
    }

    BuildingQuality curBQ;
    if(mine_hits == 6)
        curBQ = BQ_MINE;
    else if(building_hits == 6)
        curBQ = BQ_CASTLE;
    else if(flag_hits || mine_hits || building_hits)
        curBQ = BQ_FLAG;
    else
        return BQ_NOTHING;

    RTTR_Assert(curBQ == BQ_FLAG || curBQ == BQ_MINE || curBQ == BQ_CASTLE);

    //////////////////////////////////////////////////////////////////////////
    // 2. Reduce BQ based on altitude

    unsigned char curAltitude = world.GetNode(pt).altitude;
    // Restraints for buildings
    if(curBQ == BQ_CASTLE)
    {
        // First check the height of the (possible) buildings flag
        // flag point more than 1 higher? -> Flag
        unsigned char otherAltitude = world.GetNeighbourNode(pt, Direction::SOUTHEAST).altitude;
        if(otherAltitude > curAltitude + 1)
            curBQ = BQ_FLAG;
        else
        {
            // Direct neighbours: Flag for altitude diff > 3
            for(const auto dir : helpers::EnumRange<Direction>{})
            {
                otherAltitude = world.GetNeighbourNode(pt, dir).altitude;
                if(safeDiff(curAltitude, otherAltitude) > 3)
                {
                    curBQ = BQ_FLAG;
                    break;
                }
            }

            if(curBQ == BQ_CASTLE)
            {
                // Radius-2 neighbours: Hut for altitude diff > 2
                for(unsigned i = 0; i < 12; ++i)
                {
                    otherAltitude = world.GetNode(world.GetNeighbour2(pt, i)).altitude;
                    if(safeDiff(curAltitude, otherAltitude) > 2)
                    {
                        curBQ = BQ_HUT;
                        break;
                    }
                }
            }
        }

    } else if(curBQ == BQ_MINE && world.GetNeighbourNode(pt, Direction::SOUTHEAST).altitude > curAltitude + 3)
    {
        // Mines only possible till altitude diff of 3
        curBQ = BQ_FLAG;
    }

    //////////////////////////////////////////////////////////////////////////
    // 3. Check neighbouring objects that make building impossible

    // Blocking manners of neighbours (cache for reuse)
    helpers::EnumArray<BlockingManner, Direction> neighbourBlocks;
    for(const auto dir : helpers::EnumRange<Direction>{})
        neighbourBlocks[dir] = world.GetNO(world.GetNeighbour(pt, dir))->GetBM();

    // Don't build anything around charburner piles
    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        if(neighbourBlocks[dir] == BlockingManner::NothingAround)
            return BQ_NOTHING;
    }

    if(flagOnly)
    {
        // Only remaining check for flags is for neighbouring flags
        // So do this there and skip the rest
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            if(neighbourBlocks[dir] == BlockingManner::Flag)
                return BQ_NOTHING;
        }

        return BQ_FLAG;
    }

    // Build nothing if we have a flag EAST or SW
    if(neighbourBlocks[Direction::EAST] == BlockingManner::Flag
       || neighbourBlocks[Direction::SOUTHWEST] == BlockingManner::Flag)
        return BQ_NOTHING;

    //////////////////////////////////////////////////////////////////////////
    // 4. Potentially reduce BQ if some objects are nearby

    // Trees allow only huts and mines around
    if(curBQ > BQ_HUT && curBQ != BQ_MINE)
    {
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            if(neighbourBlocks[dir] == BlockingManner::Tree)
            {
                curBQ = BQ_HUT;
                break;
            }
        }
    }

    // Granite type block (stones, fire, grain fields) -> Flag around
    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        if(neighbourBlocks[dir] == BlockingManner::FlagsAround)
        {
            curBQ = BQ_FLAG;
            break;
        }
    }

    // Castle-sized buildings have extensions -> Need non-blocking object there so it can be removed
    // Note: S2 allowed blocking environment objects here which leads to visual bugs and problems as we can't place the
    // extensions
    if(curBQ == BQ_CASTLE)
    {
        for(const Direction i : {Direction::WEST, Direction::NORTHWEST, Direction::NORTHEAST})
        {
            if(neighbourBlocks[i] != BlockingManner::None)
                curBQ = BQ_HOUSE;
        }
    }

    // Check for buildings in a range of 2 -> House only
    // Note: This is inconsistent (as in the original) as it allows building a castle then a house, but not the other
    // way round
    // --> Remove this check? Only possible reason why castles could not be build should be the extensions
    if(curBQ == BQ_CASTLE)
    {
        for(unsigned i = 0; i < 12; ++i)
        {
            BlockingManner bm = world.GetNO(world.GetNeighbour2(pt, i))->GetBM();

            if(bm == BlockingManner::Building)
            {
                curBQ = BQ_HOUSE;
                break;
            }
        }
    }

    // Road at attachment -> No castle
    if(curBQ == BQ_CASTLE)
    {
        for(const Direction i : {Direction::WEST, Direction::NORTHWEST, Direction::NORTHEAST})
        {
            if(isOnRoad(world.GetNeighbour(pt, i)))
            {
                curBQ = BQ_HOUSE;
                break;
            }
        }
    }

    // If point is on a road -> Flag only
    if(curBQ != BQ_FLAG && isOnRoad(pt))
        curBQ = BQ_FLAG;

    if(curBQ == BQ_FLAG)
    {
        // If any neighbour is a flag -> Flag is impossible
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            if(neighbourBlocks[dir] == BlockingManner::Flag)
                return BQ_NOTHING;
        }
        // Else -> Flag
        return BQ_FLAG;
    }

    // If we can build a castle and this is a harbor point -> Allow harbor
    if(curBQ == BQ_CASTLE && world.GetNode(pt).harborId)
        curBQ = BQ_HARBOR;

    //////////////////////////////////////////////////////////////////////////
    // At this point we can still build a building/mine

    // If there is a flag where the house flag would be -> OK
    if(neighbourBlocks[Direction::SOUTHEAST] == BlockingManner::Flag)
        return curBQ;

    // If we can build the house flag -> OK
    if((*this)(world.GetNeighbour(pt, Direction::SOUTHEAST), isOnRoad, true) != BQ_NOTHING)
        return curBQ;

    // If not, we could still build a flag, unless there is another one around
    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        if(neighbourBlocks[dir] == BlockingManner::Flag)
            return BQ_NOTHING;
    }
    return BQ_FLAG;
}
