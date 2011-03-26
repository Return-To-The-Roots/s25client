// $Id: Ware.cpp 7084 2011-03-26 21:31:12Z OLiver $
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
#include "Ware.h"

#include "GameWorld.h"
#include "GameClientPlayer.h"
#include "GameConsts.h"
#include "nobBaseWarehouse.h"
#include "nofCarrier.h"
#include "SerializedGameData.h"
#include "nobHarborBuilding.h"
#include "GameClient.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
	#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif

Ware::Ware(const GoodType type, noBaseBuilding * goal, noRoadNode * location) :
next_dir(255), state(STATE_WAITINWAREHOUSE), location(location),
type(type == GD_SHIELDROMANS ? SHIELD_TYPES[GameClient::inst().GetPlayer(location->GetPlayer())->nation] : type ),// Bin ich ein Schild? Dann evtl. Typ nach Nation anpassen
goal(goal)
{
	// Ware in den Index mit eintragen
	gwg->GetPlayer(location->GetPlayer())->RegisterWare(this);
	
	
	//assert(obj_id != 610542);
}

Ware::~Ware()
{
	/*assert(!gwg->GetPlayer((location->GetPlayer()].IsWareRegistred(this));*/
	//if(location)
	//	assert(!gwg->GetPlayer((location->GetPlayer())->IsWareDependent(this));
}

void Ware::Destroy(void)
{
}

void Ware::Serialize_Ware(SerializedGameData * sgd) const
{
	Serialize_GameObject(sgd);

	sgd->PushUnsignedChar(next_dir);
	sgd->PushUnsignedChar(static_cast<unsigned char>(state));
	sgd->PushObject(location,false);
	sgd->PushUnsignedChar(static_cast<unsigned char>(type));
	sgd->PushObject(goal,false);
	sgd->PushUnsignedShort(next_harbor.x);
	sgd->PushUnsignedShort(next_harbor.y);
}

Ware::Ware(SerializedGameData * sgd, const unsigned obj_id) : GameObject(sgd,obj_id),
next_dir(sgd->PopUnsignedChar()),
state(State(sgd->PopUnsignedChar())),
location(sgd->PopObject<noRoadNode>(GOT_UNKNOWN)),
type(GoodType(sgd->PopUnsignedChar())),
goal(sgd->PopObject<noBaseBuilding>(GOT_UNKNOWN))

{
	next_harbor.x = sgd->PopUnsignedShort();
	next_harbor.y = sgd->PopUnsignedShort();
}


void Ware::RecalcRoute()
{
	// Nächste Richtung nehmen
	next_dir = gwg->FindPathForWareOnRoads(location,goal,NULL,&next_harbor);

	// Evtl gibts keinen Weg mehr? Dann wieder zurück ins Lagerhaus (wenns vorher überhaupt zu nem Ziel ging)
	if(next_dir == 0xFF && goal)
	{
		// meinem Ziel Becheid sagen
		goal->WareLost(this);

		nobBaseWarehouse * wh = gwg->GetPlayer(location->GetPlayer())->FindWarehouse(
			location,FW::Condition_StoreWare,0,true,&type,true);
		if(wh)
		{
			// Lagerhaus ist unser neues Ziel
			goal = wh;
			// Weg berechnen
			next_dir = gwg->FindPathForWareOnRoads(location,goal,NULL,&next_harbor);

			wh->TakeWare(this);
		}
		else
		{


			// Es gibt auch kein Weg zu einem Lagerhaus, tja dann ist es wohl vorbei erstmal
			goal = 0;

			return;
		}
	}
	
		
	// If we waited in the harbor for the ship before and don't want to travel now
	// -> inform the harbor so that it can remove us from its list
	if(state == STATE_WAITFORSHIP && next_dir != SHIP_DIR)
	{
		assert(location);
		assert(location->GetGOT() == GOT_NOB_HARBORBUILDING);
		
		static_cast<nobHarborBuilding*>(location)->WareDontWantToTravelByShip(this);

		state = STATE_WAITINWAREHOUSE;
	}

	//// Es wurde ein gültiger Weg gefunden! Dann muss aber noch dem nächsten Träger Bescheid gesagt werden
	//location->routes[next_dir]->AddWareJob(location);
}

void Ware::GoalDestroyed()
{

	if(state == STATE_WAITINWAREHOUSE)
	{
		// Ware ist noch im Lagerhaus auf der Warteliste
	}
	// Ist sie evtl. gerade mit dem Schiff unterwegs?
	else if(state == STATE_ONSHIP)
	{
		// Ziel zunächst auf NULL setzen, was dann vom Zielhafen erkannt wird,
		// woraufhin dieser die Ware gleich in sein Inventar mit übernimmt
		goal = NULL;
	}
	// Oder wartet sie im Hafen noch auf ein Schiff
	else if(state == STATE_WAITFORSHIP)
	{
		// Dann dem Hafen Bescheid sagen
		dynamic_cast<nobHarborBuilding*>(location)->CancelWareForShip(this);
		GAMECLIENT.GetPlayer(location->GetPlayer())->RemoveWare(this);
		em->AddToKillList(this);
	}
	else
	{
		// Ware ist unterwegs, Lagerhaus finden und Ware dort einliefern
		assert(location);
		assert(location->GetPlayer() < 7);

		// Wird sie gerade aus einem Lagerhaus rausgetragen?
		if(location->GetGOT() == GOT_NOB_STOREHOUSE || 
			/*location->GetGOT() == GOT_NOB_HARBOUR || */
		   location->GetGOT() == GOT_NOB_HQ)
		{
			if(location != goal)
			{
				goal = static_cast<noBaseBuilding*>(location);
				// Lagerhaus ggf. Bescheid sagen
				goal->TakeWare(this);
			}
			else
			{
				goal = NULL;
				next_dir = 0xFF;
			}
		}
		// Wenn sie an einer Flagge liegt, muss der Weg neu berechnet werden und dem Träger Bescheid gesagt werden
		else if(state == STATE_WAITATFLAG)
		{
			goal = gwg->GetPlayer(location->GetPlayer())->FindWarehouse(location,FW::Condition_StoreWare,0,true,&type,true);

			unsigned char last_next_dir = next_dir;
			next_dir = gwg->FindPathForWareOnRoads(location,goal,NULL,&next_harbor);
			RemoveWareJobForCurrentDir(last_next_dir);


			// Kein Lagerhaus gefunden bzw kein Weg dorthin?
			if(!goal || next_dir == 0xFF)
			{
				//// Mich aus der globalen Warenliste rausnehmen und in die WareLost Liste einfügen
				//gwg->GetPlayer((location->GetPlayer()].RemoveWare(this);
				//gwg->GetPlayer((location->GetPlayer()].RegisterLostWare(this);
				return;
			}

			// Es wurde ein gültiger Weg gefunden! Dann muss aber noch dem nächsten Träger Bescheid gesagt werden
			location->routes[next_dir]->AddWareJob(location);

			// Lagerhaus Bescheid sagen
			goal->TakeWare(this);
		}
		else if(state == STATE_CARRIED)
		{
			// Ziel = aktuelle Position, d.h. der Träger, der die Ware trägt, geht gerade in das Zielgebäude rein
			// d.h. es ist sowieso zu spät
			if(goal != location)
			{
				goal = gwg->GetPlayer(location->GetPlayer())->FindWarehouse(location,FW::Condition_StoreWare,0,true,&type,true);

				if(goal)
					// Lagerhaus ggf. Bescheid sagen
					goal->TakeWare(this);
			}
		}
	}
}

/// Gibt dem Ziel der Ware bekannt, dass diese nicht mehr kommen kann
void Ware::NotifyGoalAboutLostWare()
{
	// Meinem Ziel Bescheid sagen, dass ich weg vom Fenster bin (falls ich ein Ziel habe!)
	if(goal)
		goal->WareLost(this);
}

/// Wenn die Ware vernichtet werden muss
void Ware::WareLost(const unsigned char player)
{
	// Inventur verringern
	gwg->GetPlayer(player)->DecreaseInventoryWare(type,1);
	// Ziel der Ware Bescheid sagen
	NotifyGoalAboutLostWare();
	// Zentrale Registrierung der Ware löschen
	gwg->GetPlayer(player)->RemoveWare(this);
}


void Ware::RemoveWareJobForCurrentDir(const unsigned char last_next_dir)
{
	// last_next_dir war die letzte Richtung, in die die Ware eigentlich wollte,
	// aber nun nicht mehr will, deshalb muss dem Träger Bescheid gesagt werden

	// War's überhaupt ne richtige Richtung?
	if(last_next_dir < 6)
	{
		// Existiert da noch ne Straße?
		if(location->routes[last_next_dir])
		{
			// Den Trägern Bescheid sagen
			location->routes[last_next_dir]->WareJobRemoved(0);
			// Wenn nicht, könntes ja sein, dass die Straße in ein Lagerhaus führt, dann muss dort Bescheid gesagt werden
			if(location->routes[last_next_dir]->GetF2()->GetType() == NOP_BUILDING)
			{
				if(static_cast<noBuilding*>(location->routes[1]->GetF2())->GetBuildingType() == BLD_HEADQUARTERS ||
				   static_cast<noBuilding*>(location->routes[1]->GetF2())->GetBuildingType() == BLD_STOREHOUSE ||
				   static_cast<noBuilding*>(location->routes[1]->GetF2())->GetBuildingType() == BLD_HARBORBUILDING)
				   static_cast<nobBaseWarehouse*>(location->routes[1]->GetF2())->DontFetchNextWare();
			}
		}

		
			//// Und stand ein Träger drauf?
			//if(location->routes[last_next_dir]->carrier)
			//	location->routes[last_next_dir]->carrier->RemoveWareJob();
			//// Wenn nicht, könntes ja sein, dass die Straße in ein Lagerhaus führt, dann muss dort Bescheid gesagt werden
			//else if(location->routes[1])
			//{
			//	if(location->routes[1]->f2->GetType() == NOP_BUILDING)
			//	{
			//		if(static_cast<noBuilding*>(location->routes[1]->f2)->GetBuildingType() == BLD_HEADQUARTERS ||
			//		   static_cast<noBuilding*>(location->routes[1]->f2)->GetBuildingType() == BLD_STOREHOUSE ||
			//		   static_cast<noBuilding*>(location->routes[1]->f2)->GetBuildingType() == BLD_HARBORBUILDING)
			//		   static_cast<nobBaseWarehouse*>(location->routes[1]->f2)->DontFetchNextWare();
			//		else
			//			LOG.lprintf("Ware::RemoveWareJobForCurrentDi: WARNING: Ware in front of building!\n");
			//	}
			//	else
			//		LOG.lprintf("Ware::RemoveWareJobForCurrentDir: WARNING: Ware in front of building site (gf: %lu)!\n",GAMECLIENT.GetGFNumber());
			//}
	}
}

void Ware::FindRouteToWarehouse()
{
	goal = gwg->GetPlayer(location->GetPlayer())->FindWarehouse(location,FW::Condition_StoreWare,0,true,&type,true);

	if(goal && state == STATE_WAITATFLAG)
	{
		//// Bin nun keine LostWare mehr, sondern steige in die Liste der richtigen Waren auf, da ich nun in ein Lagerhaus komme
		//gwg->GetPlayer(location->GetPlayer()].RemoveLostWare(this);
		//gwg->GetPlayer(location->GetPlayer()].RegisterWare(this);

		// Weg suchen
		next_dir = gwg->FindPathForWareOnRoads(location,goal);

		// Es wurde ein gültiger Weg gefunden! Dann muss aber noch dem nächsten Träger Bescheid gesagt werden
		location->routes[next_dir]->AddWareJob(location);

		// Lagerhaus auch Bescheid sagen
		static_cast<nobBaseWarehouse*>(goal)->TakeWare(this);
	}
}

bool Ware::FindRouteFromWarehouse()
{
	if(type < 0) 
		return 0;
	return (gwg->FindPathForWareOnRoads(location,goal) != 0xFF);
}

/// Informiert Ware, dass eine Schiffsreise beginnt
void Ware::StartShipJourney()
{
	state = STATE_ONSHIP;
}

/// Informiert Ware, dass Schiffsreise beendet ist und die Ware nun in einem Hafengebäude liegt
bool Ware::ShipJorneyEnded(nobHarborBuilding * hb)
{
	state = STATE_WAITINWAREHOUSE;
	location = hb;
	return (goal != NULL);
	
}

/// Beginnt damit auf ein Schiff im Hafen zu warten
void Ware::WaitForShip(nobHarborBuilding * hb)
{
	state = STATE_WAITFORSHIP;
	location = hb;
}
