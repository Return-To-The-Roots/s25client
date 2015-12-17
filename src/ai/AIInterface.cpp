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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h"
#include "AIInterface.h"

#include "buildings/nobHarborBuilding.h"
#include "buildings/nobHQ.h"
#include "nodeObjs/noTree.h"
#include "gameData/TerrainData.h"
#include "pathfinding/RoadPathFinder.h"
#include "pathfinding/FreePathFinder.h"

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
    TerrainType t1 = gwb.GetNode(pt).t1;
    //valid terrain?
    if(TerrainData::IsUseable(t1))
    {
        if (no == NOP_TREE)
        {
            //exclude pineapple because it's not a real tree
            if (gwb.GetSpecObj<noTree>(pt)->type != 5)
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

int AIInterface::GetResourceRating(const MapPoint pt, AIJH::Resource res) const
{
    //surface resource?
    if(res == AIJH::PLANTSPACE || res == AIJH::BORDERLAND || res == AIJH::WOOD || res == AIJH::STONES)
    {
        AIJH::Resource surfaceRes = GetSurfaceResource(pt);
        TerrainType t1 = gwb.GetNode(pt).t1, t2 = gwb.GetNode(pt).t2;
        if (surfaceRes == res ||
            (res == AIJH::PLANTSPACE && surfaceRes == AIJH::NOTHING && TerrainData::IsVital(t1)) ||
            (res == AIJH::BORDERLAND && (IsBorder(pt) || !IsOwnTerritory(pt)) && (TerrainData::IsUseable(t1) || TerrainData::IsUseable(t2))))
        {
           return AIJH::RES_RADIUS[res];
        }
        //another building using our "resource"? reduce rating!
        if(res == AIJH::WOOD && IsBuildingOnNode(pt, BLD_WOODCUTTER))
            return -40;
        if(res == AIJH::PLANTSPACE && IsBuildingOnNode(pt, BLD_FORESTER))
            return -40;
    }
    //so it's a subsurface resource or something we dont calculate (multiple,blocked,nothing)
    else
    {
        if (GetSubsurfaceResource(pt) == res)
            return AIJH::RES_RADIUS[res];
    }
    return 0;
}

int AIInterface::CalcResourceValue(const MapPoint pt, AIJH::Resource res, char direction, int lastval) const
{
    int returnVal = 0;
    if(direction == -1) //calculate complete value from scratch (3n^2+3n+1)
    {
        MapPoint tmpPt = pt;
        for(unsigned r = 1; r <= AIJH::RES_RADIUS[res]; ++r)
        {
            tmpPt = gwb.GetNeighbour(tmpPt, 0);
            MapPoint tmpPt2 = tmpPt;
            for(unsigned i = 2; i < 8; ++i)
            {
                for(MapCoord r2 = 0; r2 < r; ++r2)
                {
                    returnVal += GetResourceRating(tmpPt2, res);
                    tmpPt2 = gwb.GetNeighbour(tmpPt2, i % 6);
                }
            }
        }
        //add the center point value
        returnVal += GetResourceRating(pt, res);
    }
    else//calculate different nodes only (4n+2 ?anyways much faster)
    {
        returnVal += lastval;
        //add new points
        //first: go radius steps towards direction-1
        MapPoint tmpPt(pt);
        for(unsigned i = 0; i < AIJH::RES_RADIUS[res]; i++)
            tmpPt = gwb.GetNeighbour(tmpPt, (direction + 5) % 6);
        //then clockwise around at radius distance to get all new points
        for(int i = direction + 1; i < (direction + 3); ++i)
        {
            int resRadius = AIJH::RES_RADIUS[res];
            //add 1 extra step on the second side we check to complete the side
            if(i == direction + 2)
                ++resRadius;
            for(MapCoord r2 = 0; r2 < resRadius; ++r2)
            {
                returnVal += GetResourceRating(tmpPt, res);
                tmpPt = gwb.GetNeighbour(tmpPt, i % 6);
            }
        }
        //now substract old points not in range of new point
        //go to old center point:
        tmpPt = pt;
        tmpPt = gwb.GetNeighbour(tmpPt, (direction + 3) % 6);
        //next: go to the first old point we have to substract
        for(unsigned i = 0; i < AIJH::RES_RADIUS[res]; i++)
            tmpPt = gwb.GetNeighbour(tmpPt, (direction + 2) % 6);
        //now clockwise around at radius distance to remove all old points
        for(int i = direction + 4; i < (direction + 6); ++i)
        {
            int resRadius = AIJH::RES_RADIUS[res];
            if(i == direction + 5)
                ++resRadius;
            for(MapCoord r2 = 0; r2 < resRadius; ++r2)
            {
                returnVal -= GetResourceRating(tmpPt, res);
                tmpPt = gwb.GetNeighbour(tmpPt, i % 6);
            }
        }
    }
    //if(returnval<0&&lastval>=0&&res==AIJH::BORDERLAND)
    //LOG.lprintf("AIInterface::CalcResourceValue - warning: negative returnvalue direction %i oldval %i\n", direction, lastval);
    return returnVal;
}

bool AIInterface::IsRoadPoint(const MapPoint pt) const
{
    for(unsigned char i = 0; i < 6; ++i)
    {
        if (gwb.GetPointRoad(pt, i))
            return true;
    }
    return false;
}


bool AIInterface::FindFreePathForNewRoad(MapPoint start, MapPoint target, std::vector<unsigned char> *route,
        unsigned* length) const
{
    bool boat = false;
    return gwb.GetFreePathFinder().FindPathAlternatingConditions(start, target, false, 100, route, length, NULL, IsPointOK_RoadPath,IsPointOK_RoadPathEvenStep, NULL, (void*) &boat, false);
}

/// player.FindWarehouse
nobBaseWarehouse* AIInterface::FindWarehouse(const noRoadNode& start, bool (*IsWarehouseGood)(nobBaseWarehouse*, const void*), const RoadSegment* const forbidden, const bool to_wh, const void* param, const bool use_boat_roads, unsigned* const length)
{
    return player_.FindWarehouse(start, IsWarehouseGood, forbidden, to_wh, param, use_boat_roads, length, false);
}

bool AIInterface::CalcBQSumDifference(const MapPoint pt, const MapPoint t)
{
    unsigned s1 = 0, s2 = 0;
    if(gwb.CalcBQ(pt, playerID_) != BQ_DANGER)
        s1 += gwb.CalcBQ(pt, playerID_);
    if(gwb.CalcBQ(t, playerID_) != BQ_DANGER)
        s2 += gwb.CalcBQ(t, playerID_);
    //LOG.lprintf("AIInterface::bqdiff - s1 %i,%i,%i s2 %i,%i,%i\n", pt,s1,tx,ty,s2);
    return s2 < s1;
}

bool AIInterface::FindPathOnRoads(const noRoadNode& start, const noRoadNode& target, unsigned* length) const
{
    if(length)
        return gwb.GetRoadPathFinder().FindPath(start, target, false, false, std::numeric_limits<unsigned>::max(), NULL, length);
    else
        return gwb.GetRoadPathFinder().PathExists(start, target, false, false);
}

const nobHQ* AIInterface::GetHeadquarter() const
{
    return gwb.GetSpecObj<nobHQ>(player_.hqPos);
}

bool AIInterface::IsExplorationDirectionPossible(const MapPoint pt, const nobHarborBuilding* originHarbor, ShipDirection direction) const
{
    return gwb.GetNextFreeHarborPoint(pt, originHarbor->GetHarborPosID(), direction.toUInt(), playerID_) > 0;
}

bool AIInterface::IsExplorationDirectionPossible(const MapPoint pt, unsigned int originHarborID, ShipDirection direction) const
{
    return gwb.GetNextFreeHarborPoint(pt, originHarborID, direction.toUInt(), playerID_) > 0;
}
