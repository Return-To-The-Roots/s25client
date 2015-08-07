// $Id: AIInterface.cpp
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h"
#include "AIInterface.h"

#include "buildings/nobHarborBuilding.h"
#include "buildings/nobHQ.h"
#include "nodeObjs/noTree.h"
#include "GameWorld.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// from Pathfinding.cpp TODO: in nice
bool IsPointOK_RoadPath(const GameWorldBase& gwb, const MapPoint pt, const unsigned char dir, const void* param);
bool IsPointOK_RoadPathEvenStep(const GameWorldBase& gwb, const MapPoint pt, const unsigned char dir, const void* param);

AIJH::Resource AIInterface::GetSubsurfaceResource(const MapPoint pt) const
{
    unsigned char subres = gwb.GetNode(pt).resources;

    if (subres > 0x40 + 0 * 8 && subres < 0x48 + 0 * 8)
        return AIJH::COAL;
    else if (subres > 0x40 + 1 * 8 && subres < 0x48 + 1 * 8)
        return AIJH::IRONORE;
    else if (subres > 0x40 + 2 * 8 && subres < 0x48 + 2 * 8)
        return AIJH::GOLD;
    else if (subres > 0x40 + 3 * 8 && subres < 0x48 + 3 * 8)
        return AIJH::GRANITE;
    else if (subres > 0x80 && subres < 0x90)
        return AIJH::FISH;
    else
        return AIJH::NOTHING;
}


AIJH::Resource AIInterface::GetSurfaceResource(const MapPoint pt) const
{
    NodalObjectType no = gwb.GetNO(pt)->GetType();
    unsigned char t1 = gwb.GetNode(pt).t1;
    //valid terrain?
    if(t1 != TT_WATER && t1 != TT_LAVA && t1 != TT_SWAMPLAND && t1 != TT_SNOW)
    {
        if (no == NOP_TREE)
        {
            //exclude pineapple because it's not a real tree
            if ((gwb.GetSpecObj<noTree>(pt))->type != 5)
                return AIJH::WOOD;
            else
                return AIJH::BLOCKED;
        }
        else if(no == NOP_GRANITE)
            return AIJH::STONES;
        else if (no == NOP_NOTHING || no == NOP_ENVIRONMENT)
            return AIJH::NOTHING;
        else
            return AIJH::BLOCKED;
    }
    else
        return AIJH::BLOCKED;
}

int AIInterface::CalcResourceValue(const MapPoint pt, AIJH::Resource res, char direction, int lastval) const
{
    int returnval = 0;
    if(direction == -1) //calculate complete value from scratch (3n^2+3n+1)
    {
        for(MapCoord tx = gwb.GetXA(pt, 0), r = 1; r <= AIJH::RES_RADIUS[res]; tx = gwb.GetXA(tx, pt.y, 0), ++r)
        {
            MapPoint tP2(tx, pt.y);
            for(unsigned i = 2; i < 8; ++i)
            {
                for(MapCoord r2 = 0; r2 < r; tP2 = gwb.GetNeighbour(tP2, i % 6), ++r2)
                {
                    //surface resource?
                    if(res == AIJH::PLANTSPACE || res == AIJH::BORDERLAND || res == AIJH::WOOD || res == AIJH::STONES)
                    {
                        AIJH::Resource tres = GetSurfaceResource(tP2);
                        unsigned char t1 = gwb.GetNode(tP2).t1, t2 = gwb.GetNode(tP2).t2;
                        if (tres == res || (res == AIJH::PLANTSPACE && tres == AIJH::NOTHING && t1 != TT_DESERT && t1 != TT_MOUNTAINMEADOW && t1 != TT_MOUNTAIN1 && t1 != TT_MOUNTAIN2 && t1 != TT_MOUNTAIN3 && t1 != TT_MOUNTAIN4) || (res == AIJH::BORDERLAND && (IsBorder(tP2) || !IsOwnTerritory(tP2)) && ((t1 != TT_SNOW && t1 != TT_LAVA && t1 != TT_SWAMPLAND && t1 != TT_WATER) || (t2 != TT_SNOW && t2 != TT_LAVA && t2 != TT_SWAMPLAND && t2 != TT_WATER))))
                        {
                            returnval += (AIJH::RES_RADIUS[res]);
                        }
                        //another building using our "resource"? reduce rating!
                        if(res == AIJH::WOOD || res == AIJH::PLANTSPACE)
                        {
                            if((res == AIJH::WOOD && IsBuildingOnNode(tP2, BLD_WOODCUTTER)) || (res == AIJH::PLANTSPACE && IsBuildingOnNode(tP2, BLD_FORESTER)))
                                returnval -= (40);
                        }
                    }
                    //so it's a subsurface resource or something we dont calculate (multiple,blocked,nothing)
                    else
                    {
                        if (GetSubsurfaceResource(tP2) == res)
                        {
                            returnval += (AIJH::RES_RADIUS[res]);
                        }
                    }
                }
            }
        }
        //add the center point value
        //surface resource?
        if(res == AIJH::PLANTSPACE || res == AIJH::BORDERLAND || res == AIJH::WOOD || res == AIJH::STONES)
        {
            AIJH::Resource tres = GetSurfaceResource(pt);
            unsigned char t1 = gwb.GetNode(pt).t1, t2 = gwb.GetNode(pt).t2;
            if (tres == res || (res == AIJH::PLANTSPACE && tres == AIJH::NOTHING && t1 != TT_DESERT && t1 != TT_MOUNTAINMEADOW && t1 != TT_MOUNTAIN1 && t1 != TT_MOUNTAIN2 && t1 != TT_MOUNTAIN3 && t1 != TT_MOUNTAIN4) || (res == AIJH::BORDERLAND && (IsBorder(pt) || !IsOwnTerritory(pt)) && ((t1 != TT_SNOW && t1 != TT_LAVA && t1 != TT_SWAMPLAND && t1 != TT_WATER) || (t2 != TT_SNOW && t2 != TT_LAVA && t2 != TT_SWAMPLAND && t2 != TT_WATER))))
            {
                returnval += (AIJH::RES_RADIUS[res]);
            }
            //another building using our "resource"? reduce rating!
            if(res == AIJH::WOOD || res == AIJH::PLANTSPACE)
            {
                if((res == AIJH::WOOD && IsBuildingOnNode(pt, BLD_WOODCUTTER)) || (res == AIJH::PLANTSPACE && IsBuildingOnNode(pt, BLD_FORESTER)))
                    returnval -= (40);
            }
        }
        //so it's a subsurface resource or something we dont calculate (multiple,blocked,nothing)
        else
        {
            if (GetSubsurfaceResource(pt) == res)
            {
                returnval += (AIJH::RES_RADIUS[res]);
            }
        }
    }
    else//calculate different nodes only (4n+2 ?anyways much faster)
    {
        returnval += lastval;
        //add new points
        //first: go radius steps towards direction-1
        MapPoint t(pt);
        for(unsigned i = 0; i < AIJH::RES_RADIUS[res]; i++)
            t = gwb.GetNeighbour(t, (direction + 5) % 6);
        //then clockwise around at radius distance to get all new points
        for(int i = direction + 1; i < (direction + 3); ++i)
        {
            //add 1 extra step on the second side we check to complete the side
            for(MapCoord r2 = 0; r2 < AIJH::RES_RADIUS[res] || (r2 < AIJH::RES_RADIUS[res] + 1 && i == direction + 2); ++r2)
            {
                //surface resource?
                if(res == AIJH::PLANTSPACE || res == AIJH::BORDERLAND || res == AIJH::WOOD || res == AIJH::STONES)
                {
                    AIJH::Resource tres = GetSurfaceResource(t);
                    unsigned char t1 = gwb.GetNode(t).t1, t2 = gwb.GetNode(t).t2;
                    if (tres == res || (res == AIJH::PLANTSPACE && tres == AIJH::NOTHING && t1 != TT_DESERT && t1 != TT_MOUNTAINMEADOW && t1 != TT_MOUNTAIN1 && t1 != TT_MOUNTAIN2 && t1 != TT_MOUNTAIN3 && t1 != TT_MOUNTAIN4) || (res == AIJH::BORDERLAND && (IsBorder(t) || !IsOwnTerritory(t)) && ((t1 != TT_SNOW && t1 != TT_LAVA && t1 != TT_SWAMPLAND && t1 != TT_WATER) || (t2 != TT_SNOW && t2 != TT_LAVA && t2 != TT_SWAMPLAND && t2 != TT_WATER))))
                    {
                        returnval += (AIJH::RES_RADIUS[res]);
                    }
                    //another building using our "resource"? reduce rating!
                    if(res == AIJH::WOOD || res == AIJH::PLANTSPACE)
                    {
                        if((res == AIJH::WOOD && IsBuildingOnNode(t, BLD_WOODCUTTER)) || (res == AIJH::PLANTSPACE && IsBuildingOnNode(t, BLD_FORESTER)))
                            returnval -= (40);
                    }
                }
                //so it's a subsurface resource or something we dont calculate (multiple,blocked,nothing)
                else
                {
                    if (GetSubsurfaceResource(t) == res)
                    {
                        returnval += (AIJH::RES_RADIUS[res]);
                    }
                }
                t = gwb.GetNeighbour(t, i % 6);
            }
        }
        //now substract old points not in range of new point
        //go to old center point:
        t = pt;
        t = gwb.GetNeighbour(t, (direction + 3) % 6);
        //next: go to the first old point we have to substract
        for(unsigned i = 0; i < AIJH::RES_RADIUS[res]; i++)
            t = gwb.GetNeighbour(t, (direction + 2) % 6);
        //now clockwise around at radius distance to remove all old points
        for(int i = direction + 4; i < (direction + 6); ++i)
        {
            for(MapCoord r2 = 0; r2 < AIJH::RES_RADIUS[res] || (r2 < AIJH::RES_RADIUS[res] + 1 && i == direction + 5); ++r2)
            {
                //surface resource?
                if(res == AIJH::PLANTSPACE || res == AIJH::BORDERLAND || res == AIJH::WOOD || res == AIJH::STONES)
                {
                    AIJH::Resource tres = GetSurfaceResource(t);
                    unsigned char t1 = gwb.GetNode(t).t1, t2 = gwb.GetNode(t).t2;
                    if (tres == res || (res == AIJH::PLANTSPACE && tres == AIJH::NOTHING && t1 != TT_DESERT && t1 != TT_MOUNTAINMEADOW && t1 != TT_MOUNTAIN1 && t1 != TT_MOUNTAIN2 && t1 != TT_MOUNTAIN3 && t1 != TT_MOUNTAIN4) || (res == AIJH::BORDERLAND && (IsBorder(t) || !IsOwnTerritory(t)) && ((t1 != TT_SNOW && t1 != TT_LAVA && t1 != TT_SWAMPLAND && t1 != TT_WATER) || (t2 != TT_SNOW && t2 != TT_LAVA && t2 != TT_SWAMPLAND && t2 != TT_WATER))))
                    {
                        returnval -= (AIJH::RES_RADIUS[res]);
                    }
                    //another building using our "resource"? reduce rating!
                    if(res == AIJH::WOOD || res == AIJH::PLANTSPACE)
                    {
                        if((res == AIJH::WOOD && IsBuildingOnNode(t, BLD_WOODCUTTER)) || (res == AIJH::PLANTSPACE && IsBuildingOnNode(t, BLD_FORESTER)))
                            returnval += (40);
                    }
                }
                //so it's a subsurface resource or something we dont calculate (multiple,blocked,nothing)
                else
                {
                    if (GetSubsurfaceResource(t) == res)
                    {
                        returnval -= (AIJH::RES_RADIUS[res]);
                    }
                }
                t = gwb.GetNeighbour(t, i % 6);
            }
        }
    }
    //if(returnval<0&&lastval>=0&&res==AIJH::BORDERLAND)
    //LOG.lprintf("AIInterface::CalcResourceValue - warning: negative returnvalue direction %i oldval %i\n", direction, lastval);
    return returnval;
}

bool AIInterface::IsRoadPoint(const MapPoint pt) const
{
    for(unsigned char i = 0; i < 6; ++i)
    {
        if (gwb.GetPointRoad(pt, i))
        {
            return true;
        }
    }
    return false;
}


bool AIInterface::FindFreePathForNewRoad(MapPoint start, MapPoint target, std::vector<unsigned char> *route,
        unsigned* length) const
{
    bool boat = false;
    return gwb.FindFreePathAlternatingConditions(start, target, false, 100, route, length, NULL, IsPointOK_RoadPath,IsPointOK_RoadPathEvenStep, NULL, (void*) &boat, false);
}

/// player.FindWarehouse
nobBaseWarehouse* AIInterface::FindWarehouse(const noRoadNode* const start, bool (*IsWarehouseGood)(nobBaseWarehouse*, const void*), const RoadSegment* const forbidden, const bool to_wh, const void* param, const bool use_boat_roads, unsigned* const length)
{
	 nobBaseWarehouse* best = 0;

	//  unsigned char path = 0xFF, tpath = 0xFF;
    unsigned tlength = 0xFFFFFFFF, best_length = 0xFFFFFFFF;

	for(std::list<nobBaseWarehouse*>::const_iterator w = player.GetStorehouses().begin(); w != player.GetStorehouses().end(); ++w)
    {
        // Lagerhaus geeignet?
        if(IsWarehouseGood(*w, param))
        {
            if(gwb.FindPathOnRoads(to_wh ? start : *w, to_wh ? *w : start, use_boat_roads, &tlength, NULL, NULL, forbidden))
            {
                if(tlength < best_length || !best)
                {
                    best_length = tlength;
                    best = (*w);
                }
            }
        }
    }

    if(length)
        *length = best_length;

    return best;
}

bool AIInterface::CalcBQSumDifference(const MapPoint pt, const MapPoint t)
{
    unsigned s1 = 0, s2 = 0;
    if(gwb.CalcBQ(pt, playerID) != BQ_DANGER)
        s1 += gwb.CalcBQ(pt, playerID);
    if(gwb.CalcBQ(t, playerID) != BQ_DANGER)
        s2 += gwb.CalcBQ(t, playerID);
    //LOG.lprintf("AIInterface::bqdiff - s1 %i,%i,%i s2 %i,%i,%i\n", pt,s1,tx,ty,s2);
    return s2 < s1;
}

bool AIInterface::FindPathOnRoads(const noRoadNode* start, const noRoadNode* target, unsigned* length) const
{
    return gwb.FindPathOnRoads(start, target, false, length, NULL, NULL, NULL, false);
}

const nobHQ* AIInterface::GetHeadquarter() const
{
    return gwb.GetSpecObj<nobHQ>(player.hqPos);
}

bool AIInterface::IsExplorationDirectionPossible(const MapPoint pt, const nobHarborBuilding* originHarbor, Direction direction) const
{
    return gwb.GetNextFreeHarborPoint(pt, originHarbor->GetHarborPosID(), direction.toUInt(), playerID) > 0;
}

bool AIInterface::IsExplorationDirectionPossible(const MapPoint pt, unsigned int originHarborID, Direction direction) const
{
    return gwb.GetNextFreeHarborPoint(pt, originHarborID, direction.toUInt(), playerID) > 0;
}
