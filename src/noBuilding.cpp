// $Id: noBuilding.cpp 7084 2011-03-26 21:31:12Z OLiver $
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
#include "main.h"
#include "noBuilding.h"

#include "noFire.h"

#include "GameWorld.h"
#include "SerializedGameData.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
	#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
noBuilding::noBuilding(const BuildingType type,
					   const unsigned short x, 
					   const unsigned short y,
					   const unsigned char player,
					   const Nation nation)
	: noBaseBuilding(NOP_BUILDING,type, x, y, player),
	opendoor(0)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void noBuilding::Destroy_noBuilding()
{
	// Feuer erzeugen (bei Hütten und Bergwerken kleine Feuer, bei allen anderen große!)
	// Feuer setzen
	gwg->SetNO(new noFire(x,y,(GetSize() == BQ_HUT || GetSize() == BQ_MINE)?0:1),x,y);

	Destroy_noBaseBuilding();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void noBuilding::Serialize_noBuilding(SerializedGameData * sgd) const
{
	Serialize_noBaseBuilding(sgd);

	sgd->PushUnsignedChar(opendoor);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
noBuilding::noBuilding(SerializedGameData * sgd, const unsigned obj_id) : noBaseBuilding(sgd,obj_id),
opendoor(sgd->PopUnsignedChar())
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void noBuilding::DrawBaseBuilding(int x,int y)
{
	GetBuildingImage()->Draw(x,y,0,0,0,0,0,0);
	
	// ACHTUNG nicht jedes Gebäude hat einen Schatten !!
	if(GetBuildingImageShadow())
		GetBuildingImageShadow()->Draw(x,y,0,0,0,0,0,0,COLOR_SHADOW);
	// Falls Tür offen ist und es ein Tür-Bild gibt, offene Tür zeichnen
	if(opendoor && GetDoorImage())
		GetDoorImage()->Draw(x,y,0,0,0,0,0,0);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void noBuilding::GotWorker(Job job, noFigure * worker)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
FOWObject * noBuilding::CreateFOWObject() const
{
	return new fowBuilding(type,nation);
}
