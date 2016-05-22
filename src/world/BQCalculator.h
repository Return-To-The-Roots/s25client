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
 
#include "World.h"
#include "gameData/TerrainData.h"

#ifndef BQCalculator_h__
#define BQCalculator_h__

struct BQCalculator
{
    BQCalculator(const World& world): world(world){}

    template<typename T_IsOnRoad>
    inline BuildingQuality operator()(const MapPoint pt, T_IsOnRoad isOnRoad, bool flagOnly = false) const;

private:
    const World& world;
};

template<typename T_IsOnRoad>
BuildingQuality BQCalculator::operator()(const MapPoint pt, T_IsOnRoad isOnRoad, bool flagOnly /*= false*/) const
{
    //////////////////////////////////////////////////////////////////////////
    // 1. Check maximum allowed BQ on terrain

    unsigned building_hits = 0;
    unsigned mine_hits = 0;
    unsigned flag_hits = 0;

    // bebaubar?
    for(unsigned char i = 0; i < 6; ++i)
    {
        TerrainBQ bq = TerrainData::GetBuildingQuality(world.GetTerrainAround(pt, i));
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
    if(flag_hits)
        curBQ = BQ_FLAG;
    else if(mine_hits == 6)
        curBQ = BQ_MINE;
    else if(mine_hits)
        curBQ = BQ_FLAG;
    else if(building_hits == 6)
        curBQ = BQ_CASTLE;
    else if(building_hits)
        curBQ = BQ_FLAG;
    else
        return BQ_NOTHING;


    //////////////////////////////////////////////////////////////////////////
    // 2. Reduce BQ based on altitude

    RTTR_Assert(curBQ == BQ_FLAG || curBQ == BQ_MINE || curBQ == BQ_CASTLE);

    unsigned char curAltitude = world.GetNode(pt).altitude;
    // Bergwerke anders handhaben
    if(curBQ == BQ_CASTLE)
    {
        // First check the height of the (possible) buildings flag
        // flag point more than 1 higher? -> Flag
        unsigned char otherAltitude = world.GetNeighbourNode(pt, 4).altitude;
        if(otherAltitude > curAltitude + 1)
            curBQ = BQ_FLAG;
        else
        {
            // Direct neighbours: Flag for altitude diff > 3
            for(unsigned i = 0; i < 6; ++i)
            {
                otherAltitude = world.GetNeighbourNode(pt, i).altitude;
                if(SafeDiff(curAltitude, otherAltitude) > 3)
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
                    if(SafeDiff(curAltitude, otherAltitude) > 2)
                    {
                        curBQ = BQ_HUT;
                        break;
                    }
                }
            }
        }

    } else if(curBQ == BQ_MINE && world.GetNeighbourNode(pt, 4).altitude > curAltitude + 3)
    {
        // Mines only possible till altitude diff of 3
        curBQ = BQ_FLAG;
    }

    //////////////////////////////////////////////////////////////////////////
    // 3. Check neighbouring objects that make building impossible

    // Cannot build on blocking objects
    if(world.GetNO(pt)->GetBM() != noBase::BM_NOTBLOCKING)
        return BQ_NOTHING;

    // Blocking manners of neighbours (cache for reuse)
    boost::array<noBase::BlockingManner, 6> neighbourBlocks;
    for(unsigned i = 0; i < 6; ++i)
        neighbourBlocks[i] = world.GetNO(world.GetNeighbour(pt, i))->GetBM();

    // Don't build anything around charburner piles
    for(unsigned i = 0; i < 6; ++i)
    {
        if(neighbourBlocks[i] == noBase::BM_CHARBURNERPILE)
            return BQ_NOTHING;
    }

    // Build nothing if we have a flag EAST or SW
    if(neighbourBlocks[3] == noBase::BM_FLAG || neighbourBlocks[5] == noBase::BM_FLAG)
        return BQ_NOTHING;

    if(flagOnly)
    {
        // Only remaining check for flags is for neighbouring flags
        // So do this there and skip the rest
        for(unsigned i = 0; i < 6; ++i)
        {
            if(neighbourBlocks[i] == noBase::BM_FLAG)
                return BQ_NOTHING;
        }

        return BQ_FLAG;
    }

    //////////////////////////////////////////////////////////////////////////
    // 4. Potentially reduce BQ if some objects are nearby

    if(curBQ > BQ_HUT && curBQ != BQ_MINE)
    {
        for(unsigned i = 0; i < 6; ++i)
        {
            // Baum --> around = hut
            if(neighbourBlocks[i] == noBase::BM_TREE)
            {
                curBQ = BQ_HUT;
                break;
            }

            /*// StaticObject --> rundrum flag/hut
            else if(GetNO(GetXA(x, y, i), GetYA(x, y, i))->GetType() == NOP_OBJECT)
            {
            const noStaticObject *obj = GetSpecObj<noStaticObject>(GetXA(x, y, i), GetYA(x, y, i));
            if(obj->GetSize() == 2)
            val = BQ_FLAG;
            else
            val = BQ_HUT;

            break;
            }*/
        }
    }

    // Stein, Feuer und Getreidefeld --> rundrum Flagge
    for(unsigned i = 0; i < 6; ++i)
    {
        if(neighbourBlocks[i] == noBase::BM_GRANITE)
        {
            curBQ = BQ_FLAG;
            break;
        }
    }

    // Flagge
    if(curBQ == BQ_CASTLE)
    {
        for(unsigned i = 0; i < 3; ++i)
        {
            if(neighbourBlocks[i] == noBase::BM_FLAG)
                curBQ = BQ_HOUSE;
        }
    }

    // Buildings
    if(curBQ == BQ_CASTLE)
    {
        for(unsigned i = 0; i < 12; ++i)
        {
            noBase::BlockingManner bm = world.GetNO(world.GetNeighbour2(pt, i))->GetBM();

            if(bm >= noBase::BM_HUT && bm <= noBase::BM_MINE)
                curBQ = BQ_HOUSE;
        }
    }

    if(curBQ == BQ_CASTLE)
    {
        for(unsigned i = 0; i < 3; ++i)
        {
            if(isOnRoad(world.GetNeighbour(pt, i)))
            {
                curBQ = BQ_HUT;
                break;
            }
        }
    }

    if(curBQ != BQ_FLAG)
    {
        // If point is on a road -> Flag only
        if(isOnRoad(pt))
            curBQ = BQ_FLAG;
    }

    if(curBQ == BQ_FLAG)
    {
        // If any neighbour is a flag -> Flag is impossible
        for(unsigned i = 0; i < 6; ++i)
        {
            if(neighbourBlocks[i] == noBase::BM_FLAG)
                return BQ_NOTHING;
        }
        // Else -> Flag
        return BQ_FLAG;
    }

    // Schloss bis hierhin und ist hier ein Hafenplatz?
    if(curBQ == BQ_CASTLE && world.GetNode(pt).harbor_id)
        // Dann machen wir einen Hafen draus
        curBQ = BQ_HARBOR;

    //////////////////////////////////////////////////////////////////////////
    // At this point we can still build a building/mine

    // If there is a flag where the house flag would be -> OK
    if(neighbourBlocks[4] == noBase::BM_FLAG)
        return curBQ;

    // If we can build the house flag -> OK
    if((*this)(world.GetNeighbour(pt, 4), isOnRoad, true) != BQ_NOTHING)
        return curBQ;

    // If not, we could still build a flag, unless there is another one around
    for(unsigned i = 0; i < 3; ++i)
        if(neighbourBlocks[i] == noBase::BM_FLAG)
            return BQ_NOTHING;
    return BQ_FLAG;
}

#endif // BQCalculator_h__