// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "World.h"
#include "helpers/containerUtils.h"
#include "gameData/TerrainDesc.h"

struct BQCalculator
{
    BQCalculator(const World& world) : world(world) {}

    template<typename T_IsOnRoad>
    BuildingQuality operator()(MapPoint pt, T_IsOnRoad isOnRoad, bool flagOnly = false) const;

private:
    const World& world;
};

template<typename T_IsOnRoad>
BuildingQuality BQCalculator::operator()(const MapPoint pt, T_IsOnRoad isOnRoad, const bool flagOnly /*= false*/) const
{
    // Cannot build on blocking objects
    if(world.GetNO(pt)->GetBM() != BlockingManner::None)
        return BuildingQuality::Nothing;

    //////////////////////////////////////////////////////////////////////////
    // 1. Check maximum allowed BQ on terrain

    unsigned building_hits = 0;
    unsigned mine_hits = 0;
    unsigned flag_hits = 0;

    const WorldDescription& desc = world.GetDescription();
    BuildingQuality curBQ;
    if(flagOnly)
    {
        bool flagPossible = false;
        for(const DescIdx<TerrainDesc> tIdx : world.GetTerrainsAround(pt))
        {
            TerrainBQ bq = desc.get(tIdx).GetBQ();
            if(bq == TerrainBQ::Danger)
                return BuildingQuality::Nothing;
            else if(bq != TerrainBQ::Nothing)
                flagPossible = true;
        }
        if(flagPossible)
            curBQ = BuildingQuality::Flag;
        else
            return BuildingQuality::Nothing;
    } else
    {
        for(const DescIdx<TerrainDesc> tIdx : world.GetTerrainsAround(pt))
        {
            TerrainBQ bq = desc.get(tIdx).GetBQ();
            if(bq == TerrainBQ::Castle)
                ++building_hits;
            else if(bq == TerrainBQ::Mine)
                ++mine_hits;
            else if(bq == TerrainBQ::Flag)
                ++flag_hits;
            else if(bq == TerrainBQ::Danger)
                return BuildingQuality::Nothing;
        }

        if(mine_hits == 6)
            curBQ = BuildingQuality::Mine;
        else if(building_hits == 6)
            curBQ = BuildingQuality::Castle;
        else if(flag_hits || mine_hits || building_hits)
            curBQ = BuildingQuality::Flag;
        else
            return BuildingQuality::Nothing;
    }
    RTTR_Assert(curBQ == BuildingQuality::Flag || curBQ == BuildingQuality::Mine || curBQ == BuildingQuality::Castle);
    const auto neighbours = world.GetNeighbours(pt);

    //////////////////////////////////////////////////////////////////////////
    // 2. Reduce BQ based on altitude

    // Restraints for buildings
    if(curBQ == BuildingQuality::Castle)
    {
        const unsigned char curAltitude = world.GetNode(pt).altitude;
        // First check the height of the (possible) buildings flag
        // flag point more than 1 higher? -> Flag
        unsigned char otherAltitude = world.GetNode(neighbours[Direction::SouthEast]).altitude;
        if(otherAltitude > curAltitude + 1)
            curBQ = BuildingQuality::Flag;
        else
        {
            // Direct neighbours: Flag for altitude difference > 3
            for(const MapPoint nb : neighbours)
            {
                otherAltitude = world.GetNode(nb).altitude;
                if(absDiff(curAltitude, otherAltitude) > 3)
                {
                    curBQ = BuildingQuality::Flag;
                    break;
                }
            }

            if(curBQ == BuildingQuality::Castle)
            {
                // Radius-2 neighbours: Hut for altitude difference > 2
                for(unsigned i = 0; i < 12; ++i)
                {
                    otherAltitude = world.GetNode(world.GetNeighbour2(pt, i)).altitude;
                    if(absDiff(curAltitude, otherAltitude) > 2)
                    {
                        curBQ = BuildingQuality::Hut;
                        break;
                    }
                }
            }
        }

    } else if(curBQ == BuildingQuality::Mine
              && world.GetNode(neighbours[Direction::SouthEast]).altitude > world.GetNode(pt).altitude + 3)
    {
        // Mines only possible till altitude diff of 3
        curBQ = BuildingQuality::Flag;
    }

    //////////////////////////////////////////////////////////////////////////
    // 3. Check neighbouring objects that make building impossible

    // Blocking manners of neighbours (cache for reuse)
    helpers::EnumArray<BlockingManner, Direction> neighbourBlocks;
    for(const auto dir : helpers::EnumRange<Direction>{})
        neighbourBlocks[dir] = world.GetNO(neighbours[dir])->GetBM();

    // Don't build anything around charburner piles
    if(helpers::contains(neighbourBlocks, BlockingManner::NothingAround))
        return BuildingQuality::Nothing;

    if(flagOnly)
    {
        // Only remaining check for flags is for neighbouring flags
        // So do this there and skip the rest
        if(helpers::contains(neighbourBlocks, BlockingManner::Flag))
            return BuildingQuality::Nothing;

        return BuildingQuality::Flag;
    }

    // Build nothing if we have a flag East or SW
    if(neighbourBlocks[Direction::East] == BlockingManner::Flag
       || neighbourBlocks[Direction::SouthWest] == BlockingManner::Flag)
        return BuildingQuality::Nothing;

    //////////////////////////////////////////////////////////////////////////
    // 4. Potentially reduce BQ if some objects are nearby

    // Granite type block (stones, fire, grain fields) -> Flag around
    if(helpers::contains(neighbourBlocks, BlockingManner::FlagsAround))
        curBQ = BuildingQuality::Flag;
    else if(curBQ > BuildingQuality::Hut && helpers::contains(neighbourBlocks, BlockingManner::Tree))
    {
        static_assert(BuildingQuality::Hut > BuildingQuality::Mine, "Used this assumption for above check");
        // Trees allow only huts and mines around
        curBQ = BuildingQuality::Hut;
    } else if(curBQ == BuildingQuality::Castle)
    {
        // Castle-sized buildings have extensions -> Need non-blocking object there so it can be removed
        // Note: S2 allowed blocking environment objects here which leads to visual bugs and problems as we can't place
        // the extensions
        for(const Direction i : {Direction::West, Direction::NorthWest, Direction::NorthEast})
        {
            if(neighbourBlocks[i] != BlockingManner::None)
                curBQ = BuildingQuality::House;
        }
    }

    // Check for buildings in a range of 2 -> House only
    // Note: This is inconsistent (as in the original) as it allows building a castle then a house, but not the other
    // way round
    // --> Remove this check? Only possible reason why castles could not be build should be the extensions
    if(curBQ == BuildingQuality::Castle)
    {
        for(unsigned i = 0; i < 12; ++i)
        {
            BlockingManner bm = world.GetNO(world.GetNeighbour2(pt, i))->GetBM();

            if(bm == BlockingManner::Building)
            {
                curBQ = BuildingQuality::House;
                break;
            }
        }
    }

    // Road at attachment -> No castle
    if(curBQ == BuildingQuality::Castle)
    {
        for(const Direction i : {Direction::West, Direction::NorthWest, Direction::NorthEast})
        {
            if(isOnRoad(neighbours[i]))
            {
                curBQ = BuildingQuality::House;
                break;
            }
        }
    }

    // If point is on a road -> Flag only
    if(curBQ != BuildingQuality::Flag && isOnRoad(pt))
        curBQ = BuildingQuality::Flag;

    if(curBQ == BuildingQuality::Flag)
    {
        // If any neighbour is a flag -> Flag is impossible
        if(helpers::contains(neighbourBlocks, BlockingManner::Flag))
            return BuildingQuality::Nothing;
        // Else -> Flag
        return BuildingQuality::Flag;
    }

    // If we can build a castle and this is a harbor point -> Allow harbor
    if(curBQ == BuildingQuality::Castle && world.GetNode(pt).harborId)
        curBQ = BuildingQuality::Harbor;

    //////////////////////////////////////////////////////////////////////////
    // At this point we can still build a building/mine

    // If there is a flag where the house flag would be -> OK
    if(neighbourBlocks[Direction::SouthEast] == BlockingManner::Flag)
        return curBQ;

    // If we can build the house flag -> OK
    if((*this)(neighbours[Direction::SouthEast], isOnRoad, true) != BuildingQuality::Nothing)
        return curBQ;

    // If not, we could still build a flag, unless there is another one around
    if(helpers::contains(neighbourBlocks, BlockingManner::Flag))
        return BuildingQuality::Nothing;
    return BuildingQuality::Flag;
}
