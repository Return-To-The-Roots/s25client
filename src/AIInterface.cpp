// $Id: AIInterface.cpp
//
// Copyright (c) 2005 - 2010 Settlers Freaks (sf-team at siedler25.org)
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
#include "stdafx.h"
#include "AIInterface.h"
#include "nobHarborBuilding.h"
#include "nobHQ.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
	#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif


// from Pathfinding.cpp TODO: in nice
bool IsPointOK_RoadPath(const GameWorldBase& gwb, const MapCoord x, const MapCoord y, const unsigned char dir, const void *param);

AIJH::Resource AIInterface::GetSubsurfaceResource(MapCoord x, MapCoord y) const
{
	unsigned char subres = gwb->GetNode(x,y).resources;

	if (subres > 0x40+0*8 && subres < 0x48+0*8)
		return AIJH::COAL;
	else if (subres > 0x40+1*8 && subres < 0x48+1*8)
		return AIJH::IRONORE;
	else if (subres > 0x40+2*8 && subres < 0x48+2*8)
		return AIJH::GOLD;
	else if (subres > 0x40+3*8 && subres < 0x48+3*8)
		return AIJH::GRANITE;
	else if (subres > 0x80 && subres < 0x90)
		return AIJH::FISH;
	else
		return AIJH::NOTHING;
}


AIJH::Resource AIInterface::GetSurfaceResource(MapCoord x, MapCoord y) const
{
	NodalObjectType no = gwb->GetNO(x,y)->GetType();

	if (no == NOP_TREE)
		return AIJH::WOOD;
	else if(no == NOP_GRANITE)
		return AIJH::STONES;
	else if (no == NOP_NOTHING || no == NOP_ENVIRONMENT)
		return AIJH::NOTHING;
	else
		return AIJH::BLOCKED;
}


bool AIInterface::IsRoadPoint(MapCoord x, MapCoord y) const
{
	for(unsigned char i = 0;i<6;++i)
	{
		if (gwb->GetPointRoad(x,y,i))
		{
			return true;
		}
	}
	return false;
}


bool AIInterface::FindFreePathForNewRoad(MapCoord startX, MapCoord startY, MapCoord targetX, MapCoord targetY, std::vector<Direction> *route, 
		unsigned *length) const
{
	bool boat = false;
	return gwb->FindFreePath(startX, startY, targetX, targetY, false, 100, route, length, NULL, IsPointOK_RoadPath, NULL, (void *) &boat);
}


bool AIInterface::FindPathOnRoads(const noRoadNode *start, const noRoadNode *target, unsigned *length) const
{
	return gwb->FindPathOnRoads(start, target, false, length, NULL, NULL, NULL);
}

const nobHQ *AIInterface::GetHeadquarter() const
{
	return gwb->GetSpecObj<nobHQ>(player->hqx, player->hqy);
}

bool AIInterface::IsExplorationDirectionPossible(MapCoord x, MapCoord y, const nobHarborBuilding *originHarbor, Direction direction) const
{ 
	return gwb->GetNextFreeHarborPoint(x, y, originHarbor->GetHarborPosID(), direction, playerID) > 0;
}

bool AIInterface::IsExplorationDirectionPossible(MapCoord x, MapCoord y, unsigned int originHarborID, Direction direction) const
{ 
	return gwb->GetNextFreeHarborPoint(x, y, originHarborID, direction, playerID) > 0;
}
