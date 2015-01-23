// $Id: Ware.cpp 9573 2015-01-23 08:25:35Z marcus $
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

Ware::Ware(const GoodType type, noBaseBuilding* goal, noRoadNode* location) :
    next_dir(255), state(STATE_WAITINWAREHOUSE), location(location),
    type(type == GD_SHIELDROMANS ? SHIELD_TYPES[GameClient::inst().GetPlayer(location->GetPlayer())->nation] : type ),// Bin ich ein Schild? Dann evtl. Typ nach Nation anpassen
    goal(goal)
{
    // Ware in den Index mit eintragen
    gwg->GetPlayer(location->GetPlayer())->RegisterWare(this);
}

Ware::~Ware()
{
    /*assert(!gwg->GetPlayer((location->GetPlayer()].IsWareRegistred(this));*/
    //if(location)
    //  assert(!gwg->GetPlayer((location->GetPlayer())->IsWareDependent(this));
}

void Ware::Destroy(void)
{
	
}

void Ware::Serialize_Ware(SerializedGameData* sgd) const
{
    Serialize_GameObject(sgd);

    sgd->PushUnsignedChar(next_dir);
    sgd->PushUnsignedChar(static_cast<unsigned char>(state));
    sgd->PushObject(location, false);
    sgd->PushUnsignedChar(static_cast<unsigned char>(type));
    sgd->PushObject(goal, false);
    sgd->PushUnsignedShort(next_harbor.x);
    sgd->PushUnsignedShort(next_harbor.y);
}

Ware::Ware(SerializedGameData* sgd, const unsigned obj_id) : GameObject(sgd, obj_id),
    next_dir(sgd->PopUnsignedChar()),
    state(State(sgd->PopUnsignedChar())),
    location(sgd->PopObject<noRoadNode>(GOT_UNKNOWN)),
    type(GoodType(sgd->PopUnsignedChar())),
    goal(sgd->PopObject<noBaseBuilding>(GOT_UNKNOWN))

{
    next_harbor.x = sgd->PopUnsignedShort();
    next_harbor.y = sgd->PopUnsignedShort();
    //assert(obj_id != 1197877);
}


void Ware::RecalcRoute()
{
	
    // Nächste Richtung nehmen
    next_dir = gwg->FindPathForWareOnRoads(location, goal, NULL, &next_harbor);

    // Evtl gibts keinen Weg mehr? Dann wieder zurück ins Lagerhaus (wenns vorher überhaupt zu nem Ziel ging)
    if(next_dir == 0xFF && goal)
    {
        // meinem Ziel Becheid sagen
        goal->WareLost(this);
		if(state==STATE_WAITFORSHIP)
		{
			assert(location);
			assert(location->GetGOT() == GOT_NOB_HARBORBUILDING);
			state = STATE_WAITINWAREHOUSE;
			static_cast<nobHarborBuilding*>(location)->WareDontWantToTravelByShip(this);			
		}
		else
		{
			nobBaseWarehouse* wh = gwg->GetPlayer(location->GetPlayer())->FindWarehouse(location, FW::Condition_StoreWare, 0, true, &type, true);
			if(wh)
			{
				// Lagerhaus ist unser neues Ziel
				goal = wh;
				// Weg berechnen
				next_dir = gwg->FindPathForWareOnRoads(location, goal, NULL, &next_harbor);

				wh->TakeWare(this);
			}
			else
			{
				// Es gibt auch kein Weg zu einem Lagerhaus, tja dann ist es wohl vorbei erstmal
				goal = 0;

				return;
			}
		}
    }


    // If we waited in the harbor for the ship before and don't want to travel now
    // -> inform the harbor so that it can remove us from its list
    if(state == STATE_WAITFORSHIP && next_dir != SHIP_DIR)
    {
        assert(location);
        assert(location->GetGOT() == GOT_NOB_HARBORBUILDING);
		state = STATE_WAITINWAREHOUSE;
        static_cast<nobHarborBuilding*>(location)->WareDontWantToTravelByShip(this);        
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
        assert(location->GetPlayer() < MAX_PLAYERS);

        // Wird sie gerade aus einem Lagerhaus rausgetragen?
        if(location->GetGOT() == GOT_NOB_STOREHOUSE || location->GetGOT() == GOT_NOB_HARBORBUILDING || location->GetGOT() == GOT_NOB_HQ)
        {
            if(location != goal)
            {
                goal = static_cast<noBaseBuilding*>(location);
                // Lagerhaus ggf. Bescheid sagen
                goal->TakeWare(this);
            }
            else //at the goal (which was just destroyed) and get carried out right now? -> we are about to get destroyed...
            {
                goal = NULL;
                next_dir = 0xFF;
            }
        }
        // Wenn sie an einer Flagge liegt, muss der Weg neu berechnet werden und dem Träger Bescheid gesagt werden
        else if(state == STATE_WAITATFLAG)
        {
            goal = gwg->GetPlayer(location->GetPlayer())->FindWarehouse(location, FW::Condition_StoreWare, 0, true, &type, true);

            unsigned char last_next_dir = next_dir;
            next_dir = gwg->FindPathForWareOnRoads(location, goal, NULL, &next_harbor);
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
            // if goal = current location -> too late to do anything our road will be removed and ware destroyed when the carrier starts walking about
			// goal != current location -> find a warehouse for us (if we are entering a warehouse already set this as new goal (should only happen if its a harbor for shipping as the building wasnt our goal))

            if(goal != location)
            {
				if(location->GetGOT() == GOT_NOB_STOREHOUSE || location->GetGOT() == GOT_NOB_HARBORBUILDING || location->GetGOT() == GOT_NOB_HQ) //currently carried into a warehouse? -> add ware (pathfinding will not return this wh because of path lengths 0)
				{
					if(location->GetGOT()!=GOT_NOB_HARBORBUILDING)
						LOG.lprintf("WARNING: Ware::GoalDestroyed() -- ware is currently being carried into warehouse or hq that was not it's goal! ware id %i, type %i, player %i, wareloc %i,%i, goal loc %i,%i \n",GetObjId(),type,location->GetPlayer(),GetLocation()->GetX(),GetLocation()->GetY(), goal->GetX(),goal->GetY());
					goal = static_cast<noBaseBuilding*>(location);
					goal->TakeWare(this);
				}
				else
				{
					goal = gwg->GetPlayer(location->GetPlayer())->FindWarehouse(location, FW::Condition_StoreWare, 0, true, &type, true);

					if(goal)
						// Lagerhaus ggf. Bescheid sagen
						goal->TakeWare(this);
				}
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
    gwg->GetPlayer(player)->DecreaseInventoryWare(type, 1);
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
        //  location->routes[last_next_dir]->carrier->RemoveWareJob();
        //// Wenn nicht, könntes ja sein, dass die Straße in ein Lagerhaus führt, dann muss dort Bescheid gesagt werden
        //else if(location->routes[1])
        //{
        //  if(location->routes[1]->f2->GetType() == NOP_BUILDING)
        //  {
        //      if(static_cast<noBuilding*>(location->routes[1]->f2)->GetBuildingType() == BLD_HEADQUARTERS ||
        //         static_cast<noBuilding*>(location->routes[1]->f2)->GetBuildingType() == BLD_STOREHOUSE ||
        //         static_cast<noBuilding*>(location->routes[1]->f2)->GetBuildingType() == BLD_HARBORBUILDING)
        //         static_cast<nobBaseWarehouse*>(location->routes[1]->f2)->DontFetchNextWare();
        //      else
        //          LOG.lprintf("Ware::RemoveWareJobForCurrentDi: WARNING: Ware in front of building!\n");
        //  }
        //  else
        //      LOG.lprintf("Ware::RemoveWareJobForCurrentDir: WARNING: Ware in front of building site (gf: %lu)!\n",GAMECLIENT.GetGFNumber());
        //}
    }
}

void Ware::FindRouteToWarehouse()
{
    goal = gwg->GetPlayer(location->GetPlayer())->FindWarehouse(location, FW::Condition_StoreWare, 0, true, &type, true);

    if(goal && state == STATE_WAITATFLAG)
    {
        //// Bin nun keine LostWare mehr, sondern steige in die Liste der richtigen Waren auf, da ich nun in ein Lagerhaus komme
        //gwg->GetPlayer(location->GetPlayer()].RemoveLostWare(this);
        //gwg->GetPlayer(location->GetPlayer()].RegisterWare(this);

        // Weg suchen
        next_dir = gwg->FindPathForWareOnRoads(location, goal);

        // Es wurde ein gültiger Weg gefunden! Dann muss aber noch dem nächsten Träger Bescheid gesagt werden
        location->routes[next_dir]->AddWareJob(location);

        // Lagerhaus auch Bescheid sagen
        static_cast<nobBaseWarehouse*>(goal)->TakeWare(this);
    }
}

///a lost ware got ordered
unsigned Ware::CheckNewGoalForLostWare(noBaseBuilding* newgoal)
{
	unsigned tlength = 0xFFFFFFFF;
	if (state!=STATE_WAITATFLAG) //todo: check all special cases for wares being carried right now and where possible allow them to be ordered
		return 0xFFFFFFFF;
	unsigned char possibledir=gwg->FindPathForWareOnRoads(location, newgoal,&tlength);
	if(possibledir!=0xFF) //there is a valid path to the goal? -> ordered!
	{
		//in case the ware is right in front of the goal building the ware has to be moved away 1 flag and then back because non-warehouses cannot just carry in new wares they need a helper to do this
		if(possibledir==1 && newgoal->GetFlag()->GetX()==location->GetX() && newgoal->GetFlag()->GetY()==location->GetY())
		{
			for(unsigned i=0;i<6;i++)
			{
				if(i!=1 && location->routes[i])
					{
						possibledir=i;
						break;
					}
			}
			if(possibledir==1) //got no other route from the flag -> impossible
				return 0xFFFFFFFF;					
		}
		//at this point there either is a road to the goal or if we are at the flag of the goal we have a road to a different flag to bounce off of to get to the goal		
		return tlength;		
	}
	else
		return 0xFFFFFFFF;
}

/// this assumes that the ware is at a flag (todo: handle carried wares) and that there is a valid path to the goal
void Ware::SetNewGoalForLostWare(noBaseBuilding* newgoal)
{
	unsigned char possibledir=gwg->FindPathForWareOnRoads(location, newgoal);
	if(possibledir!=0xFF) //there is a valid path to the goal? -> ordered!
	{
		//in case the ware is right in front of the goal building the ware has to be moved away 1 flag and then back because non-warehouses cannot just carry in new wares they need a helper to do this
		if(possibledir==1 && newgoal->GetFlag()->GetX()==location->GetX() && newgoal->GetFlag()->GetY()==location->GetY())
		{
			for(unsigned i=0;i<6;i++)
			{
				if(i!=1 && location->routes[i])
					{
						possibledir=i;
						break;
					}
			}
			if(possibledir==1) //got no other route from the flag -> impossible
				return;					
		}
		//at this point there either is a road to the goal or if we are at the flag of the goal we have a road to a different flag to bounce off of to get to the goal
		next_dir=possibledir;	
		goal=newgoal;	
		location->routes[next_dir]->AddWareJob(location);	
	}
}

bool Ware::FindRouteFromWarehouse()
{
    if(type < 0)
        return 0;
    return (gwg->FindPathForWareOnRoads(location, goal) != 0xFF);
}

/// Informiert Ware, dass eine Schiffsreise beginnt
void Ware::StartShipJourney()
{
    state = STATE_ONSHIP;
    location = 0;
}

/// Informiert Ware, dass Schiffsreise beendet ist und die Ware nun in einem Hafengebäude liegt
bool Ware::ShipJorneyEnded(nobHarborBuilding* hb)
{

    state = STATE_WAITINWAREHOUSE;
    location = hb;

    if (goal == NULL)
    {
        return(false);
    }

    next_dir = gwg->FindPathForWareOnRoads(location, goal, NULL, &next_harbor);

// TODO: SHIP_DIR? order ship etc.
    if ((next_dir == 0xFF) || (next_dir == SHIP_DIR))
    {
        goal->WareLost(this);
        goal = NULL;
        return(false);
    }

    return(true);
}

/// Beginnt damit auf ein Schiff im Hafen zu warten
void Ware::WaitForShip(nobHarborBuilding* hb)
{
    state = STATE_WAITFORSHIP;
    location = hb;
}
