// $Id: nobMilitary.cpp 9587 2015-02-01 09:37:07Z marcus $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your oposion) any later version.
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
#include "nobMilitary.h"
#include "GameClient.h"
#include "GameClientPlayer.h"
#include "GameWorld.h"
#include "helpers/PointDistance.h"
#include "gameData/MilitaryConsts.h"
#include "Ware.h"
#include "figures/nofPassiveSoldier.h"
#include "figures/nofDefender.h"
#include "figures/nofAggressiveDefender.h"
#include "figures/nofAttacker.h"
#include "Loader.h"
#include "macros.h"
#include "EventManager.h"
#include "Random.h"
#include "nobBaseWarehouse.h"

#include "WindowManager.h"

#include "SerializedGameData.h"
#include "MapGeometry.h"
#include "ai/AIEventManager.h"
#include "Point.h"
#include "Log.h"

#include <limits>
#include <stdexcept>

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

nobMilitary::nobMilitary(const BuildingType type, const MapPoint pos, const unsigned char player, const Nation nation)
    : nobBaseMilitary(type, pos, player, nation), new_built(true), coins(0), disable_coins(false),
      disable_coins_virtual(false), capturing(false), capturing_soldiers(0), goldorder_event(0), upgrade_event(0), is_regulating_troops(false), captured_not_built(false)
{
    // GebÃ¤ude entsprechend als MilitÃ¤rgebÃ¤ude registrieren und in ein MilitÃ¤rquadrat eintragen
    gwg->GetPlayer(player)->AddMilitaryBuilding(this);
    gwg->GetMilitarySquare(pos).push_back(this);

    // GrÃ¶ÃŸe ermitteln
    switch(type)
    {
        case BLD_BARRACKS: size = 0; break;
        case BLD_GUARDHOUSE: size = 1; break;
        case BLD_WATCHTOWER: size = 2; break;
        case BLD_FORTRESS: size = 3; break;
        default: size = 0xFF; break;
    }

    LookForEnemyBuildings();

    // TÃ¼r aufmachen, bis GebÃ¤ude besetzt ist
    OpenDoor();

    // Wenn kein Gold in neu gebaute MilitÃ¤rgebÃ¤ude eingeliefert werden soll, wird die Goldzufuhr gestoppos
    // Ansonsten neue GoldmÃ¼nzen anfordern
    if(GAMECLIENT.GetGGS().isEnabled(ADDON_NO_COINS_DEFAULT))
    {
        disable_coins = true;
        disable_coins_virtual = true;
    }
    else
        SearchCoins();
}

nobMilitary::~nobMilitary()
{
    // Soldaten vernichten
    for(std::list<nofPassiveSoldier*>::iterator it = troops.begin(); it != troops.end(); ++it)
        delete (*it);
}

size_t nobMilitary::GetTotalSoldiers() const
{
    size_t sum = troops.size() + ordered_troops.size() + troops_on_mission.size();
    if(defender && (defender->IsWaitingAtFlag() || defender->IsFightingAtFlag()))
        sum++;
    sum += /* capturing_soldiers*/ + far_away_capturers.size();
    return sum;
}

void nobMilitary::Destroy_nobMilitary()
{

    // Bestellungen stornieren
    CancelOrders();

    // Soldaten rausschicken
    for(std::list<nofPassiveSoldier*>::iterator it = troops.begin(); it != troops.end(); ++it)
        (*it)->InBuildingDestroyed();

    // Inform far-away capturers
    for(std::list<nofAttacker*>::iterator it = far_away_capturers.begin(); it != far_away_capturers.end(); ++it)
        (*it)->AttackedGoalDestroyed();

    troops.clear();

    // Events ggf. entfernen
    em->RemoveEvent(goldorder_event);
    em->RemoveEvent(upgrade_event);

    // Ã¼briggebliebene GoldmÃ¼nzen in der Inventur abmelden
    gwg->GetPlayer(player)->DecreaseInventoryWare(GD_COINS, coins);

    Destroy_nobBaseMilitary();

    // Wieder aus dem MilitÃ¤rquadrat rauswerfen
    gwg->GetPlayer(player)->RemoveMilitaryBuilding(this);
    gwg->GetMilitarySquare(pos).remove(this);

    // Land drumherum neu berechnen (nur wenn es schon besetzt wurde!)
    // Nach dem BaseDestroy erst, da in diesem erst das Feuer gesetzt, die StraÃŸe gelÃ¶scht wird usw.
    if(!new_built)
        gwg->RecalcTerritory(this, MILITARY_RADIUS[size], true, false);

    GAMECLIENT.SendAIEvent(new AIEvent::Building(AIEvent::BuildingLost, pos, type), player);

}

void nobMilitary::Serialize_nobMilitary(SerializedGameData* sgd) const
{
    Serialize_nobBaseMilitary(sgd);

    unsigned char bitfield = 0;

    if (new_built)
    {
        bitfield |= (1 << 0);
    }

    // reverse for compatibility :)
    if (!captured_not_built)
    {
        bitfield |= (1 << 1);
    }

    sgd->PushUnsignedChar(bitfield);

    sgd->PushUnsignedChar(coins);
    sgd->PushBool(disable_coins);
    sgd->PushBool(disable_coins_virtual);
    sgd->PushUnsignedChar(frontier_distance);
    sgd->PushUnsignedChar(size);
    sgd->PushBool(capturing);
    sgd->PushUnsignedInt(capturing_soldiers);
    sgd->PushObject(goldorder_event, true);
    sgd->PushObject(upgrade_event, true);

    sgd->PushObjectList(ordered_troops, true);
    sgd->PushObjectList(ordered_coins, true);
    sgd->PushObjectList(troops, true);
    sgd->PushObjectList(far_away_capturers, true);
}

nobMilitary::nobMilitary(SerializedGameData* sgd, const unsigned obj_id) : nobBaseMilitary(sgd, obj_id),
    is_regulating_troops(false)
{
    // use a bitfield instead of 1 unsigned char per boolean
    // mainly for compatibility :-)

    unsigned char bitfield = sgd->PopUnsignedChar();

    new_built = bitfield & (1 << 0);
    captured_not_built = !(bitfield & (1 << 1));

    coins = sgd->PopUnsignedChar();
    disable_coins = sgd->PopBool();
    disable_coins_virtual = sgd->PopBool();
    frontier_distance = sgd->PopUnsignedChar();
    size = sgd->PopUnsignedChar();
    capturing = sgd->PopBool();
    capturing_soldiers = sgd->PopUnsignedInt();
    goldorder_event = sgd->PopObject<EventManager::Event>(GOT_EVENT);
    upgrade_event = sgd->PopObject<EventManager::Event>(GOT_EVENT);


    sgd->PopObjectList(ordered_troops, GOT_NOF_PASSIVESOLDIER);
    sgd->PopObjectList(ordered_coins, GOT_WARE);
    sgd->PopObjectList(troops, GOT_NOF_PASSIVESOLDIER);
    sgd->PopObjectList(far_away_capturers, GOT_NOF_ATTACKER);

    // ins MilitÃ¤rquadrat einfÃ¼gen
    gwg->GetMilitarySquare(pos).push_back(this);
}


void nobMilitary::Draw(int x, int y)
{
    // GebÃ¤ude an sich zeichnen
    DrawBaseBuilding(x, y);


    // (max 4) Besatzungs-FÃ¤hnchen zeichnen
    unsigned flags = min<unsigned>(troops.size() + this->leave_house.size(), 4);

    for(unsigned i = 0; i < flags; ++i)
        LOADER.GetMapImageN(3162 + GAMECLIENT.GetGlobalAnimation(8, 2, 1, pos.x * pos.y * i))->Draw(x + TROOPS_FLAGS[nation][size][0], y + TROOPS_FLAGS[nation][size][1] + (i) * 3, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);

    // Die Fahne, die anzeigt wie weit das GebÃ¤ude von der Grenze entfernt ist, zeichnen
    unsigned frontier_distance_tmp = frontier_distance;
    glArchivItem_Bitmap* bitmap = NULL;
    if(frontier_distance_tmp == 2)
    {
        // todo Hafenflagge
        bitmap = LOADER.GetImageN("map_new", 3150 + GAMECLIENT.GetGlobalAnimation(4, 1, 1, pos.x * pos.y * age));

    }
    else
    {
        if(frontier_distance_tmp == 3) frontier_distance_tmp = 2;
        bitmap = LOADER.GetMapImageN(3150 + frontier_distance_tmp * 4 + GAMECLIENT.GetGlobalAnimation(4, 1, 1, GetX() * GetY() * age));
    }
    if(bitmap)
        bitmap->Draw(x + BORDER_FLAGS[nation][size][0], y + BORDER_FLAGS[nation][size][1], 0, 0, 0, 0, 0, 0);

    // Wenn Goldzufuhr gestoppos ist, Schild auÃŸen am GebÃ¤ude zeichnen zeichnen
    if(disable_coins_virtual)
        LOADER.GetMapImageN(46)->Draw(x + BUILDING_SIGN_CONSTS[nation][type].x, y + BUILDING_SIGN_CONSTS[nation][type].y, 0, 0, 0, 0, 0, 0);



    //char number[256];
    //sprintf(number,"%u",this->obj_id);
    //NormalFont->Draw(x,y,number,0,0xFF00FF00);
}

void nobMilitary::HandleEvent(const unsigned int id)
{
    switch(id)
    {
            // "Rausgeh-Event"
        case 0:
        {
            leaving_event = 0;

            // Sind Leute da, die noch rausgehen wollen?
            if(!leave_house.empty())
            {
                // Dann raus mit denen
                noFigure* soldier = *leave_house.begin();
                gwg->AddFigure(soldier, pos);

                soldier->ActAtFirst();
                leave_house.pop_front();
            }
            else
            {
                go_out = false;
            }

            // Wenn noch weitere drin sind, die mÃ¼ssen auch noch raus
            if(!leave_house.empty())
                leaving_event = em->AddEvent(this, 30 + RANDOM.Rand(__FILE__, __LINE__, obj_id, 10));
            else
                go_out = false;

        } break;
        // Goldbestell-Event
        case 1:
        {
            goldorder_event = 0;

            // ggf. nach neuen GoldmÃ¼nzen suchen
            SearchCoins();
        } break;
        // BefÃ¶rderungs-Event
        case 2:
        {
            upgrade_event = 0;

            // Soldaten befÃ¶rdern
            // Von hinten durchgehen
            // Wenn der nachfolgende (schwÃ¤chere) Soldat einen niedrigeren Rang hat,
            // wird dieser ebenfalls befÃ¶rdert usw.!

            // Rang des letzten befÃ¶rderten Soldaten, 4-MaxRank am Anfang setzen, damit keiner Ã¼ber den maximalen Rang befÃ¶rdert wird
            unsigned char last_rank = MAX_MILITARY_RANK - GAMECLIENT.GetGGS().getSelection(ADDON_MAX_RANK);

            for(std::list<nofPassiveSoldier*>::reverse_iterator it = troops.rbegin(); it != troops.rend(); ++it)
            {
                // Es wurde schon einer befÃ¶rdert, dieser Soldat muss nun einen niedrigeren Rang
                // als der letzte haben, damit er auch noch befÃ¶rdert werden kann
                if((*it)->GetRank() < last_rank)
                {
                    // Rang merken
                    last_rank = (*it)->GetRank();
                    // Dann befÃ¶rdern
                    (*it)->Upgrade();
                }
            }

            // Wurde jemand befÃ¶rdert?
            if(last_rank < MAX_MILITARY_RANK - GAMECLIENT.GetGGS().getSelection(ADDON_MAX_RANK))
            {
                // GoldmÃ¼nze verbrauchen
                --coins;
                gwg->GetPlayer(player)->DecreaseInventoryWare(GD_COINS, 1);

                // Evtl neues BefÃ¶rderungsevent anmelden
                PrepareUpgrading();

                // Ggf. neue GoldmÃ¼nzen bestellen
                SearchCoins();
            }

        } break;
    }
}

unsigned short nobMilitary::GetMilitaryRadius() const
{
    return MILITARY_RADIUS[size];
}

void nobMilitary::LookForEnemyBuildings(const nobBaseMilitary* const exceposion)
{
    // Umgebung nach MilitÃ¤rgebÃ¤uden absuchen
    std::set<nobBaseMilitary*> buildings = gwg->LookForMilitaryBuildings(pos, 3);
    frontier_distance = 0;

    for(std::set<nobBaseMilitary*>::iterator it = buildings.begin(); it != buildings.end(); ++it)
    {
        // feindliches MilitÃ¤rgebÃ¤ude?
        if(*it != exceposion && (*it)->GetPlayer() != player && gwg->GetPlayer((*it)->GetPlayer())->IsPlayerAttackable(player))
        {
            unsigned distance = gwg->CalcDistance(pos, (*it)->GetPos());

            // in nahem Umkreis, also Grenzen berÃ¼hren sich
            if(distance <= MILITARY_RADIUS[size] + (*it)->GetMilitaryRadius()) // warum erzeugtn das ne warning in vs2008?
            {
                // GrenznÃ¤he entsprechend setzen
                frontier_distance = 3;

                // Wenns ein richtiges MilitÃ¤rgebÃ¤ude ist, dann dort auch entsprechend setzen
                if((*it)->GetBuildingType() >= BLD_BARRACKS && (*it)->GetBuildingType() <= BLD_FORTRESS)
                    static_cast<nobMilitary*>(*it)->NewEnemyMilitaryBuilding(3);
            }
            // in mittlerem Umkreis, also theoretisch angreifbar?
            else if(distance < BASE_ATTACKING_DISTANCE
                    + (TROOPS_COUNT[nation][size] - 1) * EXTENDED_ATTACKING_DISTANCE)
            {
                // GrenznÃ¤he entsprechend setzen
                if(!frontier_distance)
                    frontier_distance = 1;

                // Wenns ein richtiges MilitÃ¤rgebÃ¤ude ist, dann dort auch entsprechend setzen
                if((*it)->GetBuildingType() >= BLD_BARRACKS && (*it)->GetBuildingType() <= BLD_FORTRESS)
                    static_cast<nobMilitary*>(*it)->NewEnemyMilitaryBuilding(1);
            }
            // andere Richtung muss auch getestet werden, zumindest wenns eine normaler MilitÃ¤rgebÃ¤ude ist, Bug 389843
            else if ((*it)->GetGOT() == GOT_NOB_MILITARY)
            {
                nobMilitary* mil = dynamic_cast<nobMilitary*>(*it);
                if(distance < BASE_ATTACKING_DISTANCE + (TROOPS_COUNT[mil->nation][mil->size] - 1) * EXTENDED_ATTACKING_DISTANCE)
                {
                    // GrenznÃ¤he entsprechend setzen
                    if(!frontier_distance)
                        frontier_distance = 1;

                    // dort auch entsprechend setzen
                    mil->NewEnemyMilitaryBuilding(1);
                }
            }
        }
    }

    // Evtl. Hafenpunkte in der N? mit ber?htigen
    if(frontier_distance <= 1)
        if(gwg->CalcDistanceToNearestHarbor(pos) < SEAATTACK_DISTANCE + 2)
        {
            //if(gwg->IsAHarborInSeaAttackDistance(MapPoint(x,y)))
            frontier_distance = 2;
        }

    // Truppen schicken
    RegulateTroops();

}


void nobMilitary::NewEnemyMilitaryBuilding(const unsigned short distance)
{
    // Neues GrenzgebÃ¤ude in der NÃ¤he --> Distanz entsprechend setzen
    if(distance == 3)
    {
        // Nah
        frontier_distance = 3;
    }
    // in mittlerem Umkreis?
    else if(distance == 1)
    {
        // Mittel (nur wenns vorher auf weit weg war)
        if(!frontier_distance)
            frontier_distance = 1;
    }

    RegulateTroops();

    // KI-Event senden
    //GAMECLIENT.SendAIEvent(new AIEvent::Building(AIEvent::BorderChanged, pos, type), player);
}


void nobMilitary::RegulateTroops()
{
    // Wenn das GebÃ¤ude eingenommen wird, erstmal keine neuen Truppen und warten, wieviele noch reinkommen
    if(capturing)
        return;

    // Already regulate its troops => Don't call this method again
    if(is_regulating_troops)
        return;

    is_regulating_troops = true;

    // Zu viele oder zu wenig Truppen?
    int diff = CalcTroopsCount() - static_cast<int>(GetTotalSoldiers());
    if(diff < 0) //poc: this should only be >0 if we are being captured. capturing should be true until its the last soldier and this last one would count twice here and result in a returning soldier that shouldnt return.
    {
        // Zu viel --> Ã¼berflÃ¼ssige Truppen nach Hause schicken
        // Zuerst die bestellten Soldaten wegschicken
        // Weak ones first
        std::list<nofPassiveSoldier*> notNeededSoldiers;
        if (gwg->GetPlayer(player)->military_settings[1] > MILITARY_SETTINGS_SCALE[1] / 2)
        {
            for(std::list<nofPassiveSoldier*>::iterator it = ordered_troops.begin(); diff && !ordered_troops.empty(); ++diff)
            {
                notNeededSoldiers.push_back(*it);
                it = ordered_troops.erase(it);
            }
        }
        // Strong ones first
        else
        {
            for(std::list<nofPassiveSoldier*>::reverse_iterator it = ordered_troops.rbegin(); diff && !ordered_troops.empty(); ++diff)
            {
                notNeededSoldiers.push_back(*it);
                // To delete a reverse iterator increment it and remove the base iterator
                it = std::list<nofPassiveSoldier*>::reverse_iterator(ordered_troops.erase((++it).base()));
            }
        }

        // send the not-needed-soldiers away
        for (std::list<nofPassiveSoldier*>::iterator it = notNeededSoldiers.begin(); it != notNeededSoldiers.end(); it++)
        {
            (*it)->NotNeeded();
        }

        // Nur rausschicken, wenn es einen Weg zu einem Lagerhaus gibt!
        if(gwg->GetPlayer(player)->FindWarehouse(this, FW::NoCondition, 0, true, 0, false))
        {
            // Dann den Rest (einer muss immer noch drinbleiben!)
            // erst die schwachen Soldaten raus
            if (gwg->GetPlayer(player)->military_settings[1] > MILITARY_SETTINGS_SCALE[1] / 2)
            {
                for(std::list<nofPassiveSoldier*>::iterator it = troops.begin(); diff && troops.size() > 1; ++diff)
                {
                    (*it)->LeaveBuilding();
                    AddLeavingFigure(*it);
                    it = troops.erase(it);
                }
            }
            // erst die starken Soldaten raus
            else
            {
                for(std::list<nofPassiveSoldier*>::reverse_iterator it = troops.rbegin(); diff && troops.size() > 1; ++diff)
                {
                    (*it)->LeaveBuilding();
                    AddLeavingFigure(*it);
                    it = std::list<nofPassiveSoldier*>::reverse_iterator(troops.erase((++it).base()));
                }
            }
        }

    }
    else if(diff > 0)
    {
        // Zu wenig Truppen

        // GebÃ¤ude wird angegriffen und
        // Addon aktiv, nur soviele Leute zum Nachbesetzen schicken wie Verteidiger eingestellt
        if (aggressors.size() > 0 && GAMECLIENT.GetGGS().getSelection(ADDON_DEFENDER_BEHAVIOR) == 2)
        {
            diff = (gwg->GetPlayer(player)->military_settings[2] * diff) / MILITARY_SETTINGS_SCALE[2];
        }
		//only order new troops if there is a chance that there is a path - pathfinding from each warehouse with soldiers to this mil building will start at the warehouse and cost time
        bool mightHaveRoad=false;
		for(unsigned i=2; i<7; i++) //every direction but 1 because 1 is the building connection so it doesnt count for this check
		{
			if(GetFlag()->routes[i%6])
			{
			    mightHaveRoad=true;
				break;
			}
		}
		if(mightHaveRoad)
			gwg->GetPlayer(player)->OrderTroops(this, diff);
    }

    is_regulating_troops = false;
}

int nobMilitary::CalcTroopsCount()
{
    return (TROOPS_COUNT[nation][size] - 1) * gwg->GetPlayer(player)->military_settings[4 + frontier_distance] / MILITARY_SETTINGS_SCALE[4 + frontier_distance] + 1;
}

void nobMilitary::SendSoldiersHome()
{
	int diff = 1 - static_cast<int>(GetTotalSoldiers());
    if(diff < 0) //poc: this should only be >0 if we are being captured. capturing should be true until its the last soldier and this last one would count twice here and result in a returning soldier that shouldnt return.
	{
		// Nur rausschicken, wenn es einen Weg zu einem Lagerhaus gibt!
        if(!gwg->GetPlayer(player)->FindWarehouse(this, FW::NoCondition, 0, true, 0, false))
            return;
        int mrank=-1;
        for(std::list<nofPassiveSoldier*>::reverse_iterator it = troops.rbegin(); diff && troops.size() > 1; ++diff)
        {
            if(mrank<0) //set mrank = highest rank
                mrank=(*it)->GetRank();
            else if (mrank>(*it)->GetRank()) //if the current soldier is of lower rank than what we started with -> send no more troops out
                return;
            (*it)->LeaveBuilding();
            AddLeavingFigure(*it);
            it = std::list<nofPassiveSoldier*>::reverse_iterator(troops.erase((++it).base()));
        }
	}
}

//used by the ai to refill the upgradebuilding with low rank soldiers! - normal orders for soldiers are done in RegulateTroops!
void nobMilitary::OrderNewSoldiers()
{
	//cancel all max ranks on their way to this building
	std::list<nofPassiveSoldier*> noNeed;
	for(std::list<nofPassiveSoldier*>::iterator it = ordered_troops.begin(); it != ordered_troops.end(); )
    {
		if((*it)->GetRank() >= MAX_MILITARY_RANK - GAMECLIENT.GetGGS().getSelection(ADDON_MAX_RANK))
		{
			nofPassiveSoldier* soldier = *it;
			it = ordered_troops.erase(it);
			noNeed.push_back(soldier);
		}else
		    ++it;
    }

	int diff = CalcTroopsCount() - static_cast<int>(GetTotalSoldiers());
	//order new troops now
    if(diff > 0) //poc: this should only be >0 if we are being captured. capturing should be true until its the last soldier and this last one would count twice here and result in a returning soldier that shouldnt return.
	{
		// Zu wenig Truppen
        // GebÃ¤ude wird angegriffen und
        // Addon aktiv, nur soviele Leute zum Nachbesetzen schicken wie Verteidiger eingestellt
        if (aggressors.size() > 0 && GAMECLIENT.GetGGS().getSelection(ADDON_DEFENDER_BEHAVIOR) == 2)
        {
            diff = (gwg->GetPlayer(player)->military_settings[2] * diff) / MILITARY_SETTINGS_SCALE[2];
        }
        gwg->GetPlayer(player)->OrderTroops(this, diff,true);
	} 
	//now notify the max ranks we no longer wanted (they will pick a new target which may be the same building that is why we cancel them after ordering new ones in the hope to get low ranks instead)
	for (std::list<nofPassiveSoldier*>::const_iterator it=noNeed.begin();it!=noNeed.end();it++)
		(*it)->NotNeeded();
}

bool nobMilitary::IsUseless() const
{
    if(frontier_distance || new_built)
    {
        return false;
    }
    return(gwg->TerritoryChange(this, MILITARY_RADIUS[size], true, false));

}

void nobMilitary::TakeWare(Ware* ware)
{
    // GoldmÃ¼nze in Bestellliste aufnehmen
    ordered_coins.push_back(ware);
}


void nobMilitary::AddWare(Ware* ware)
{
    // Ein GolstÃ¼ck mehr
    ++coins;
    // aus der Bestellliste raushaun
    ordered_coins.remove(ware);

    // Ware vernichten
    gwg->GetPlayer(player)->RemoveWare(ware);
    delete ware;

    // Evtl. Soldaten befÃ¶rdern
    PrepareUpgrading();
}

void nobMilitary::WareLost(Ware* ware)
{
    // Ein GoldstÃ¼ck konnte nicht kommen --> aus der Bestellliste entfernen
    ordered_coins.remove(ware);
}

bool nobMilitary::FreePlaceAtFlag()
{
    return false;
}
void nobMilitary::GotWorker(Job job, noFigure* worker)
{
    // Insert soldiers sorted. Weak ones first
    nofPassiveSoldier* soldier = static_cast<nofPassiveSoldier*>(worker);

    for(std::list<nofPassiveSoldier*>::iterator it = ordered_troops.begin(); it != ordered_troops.end(); ++it) {
        // Insert before current, if rank is smaller
        if(soldier->GetRank() < (*it)->GetRank()) {
            ordered_troops.insert(it, soldier);
            return;
        }
    }

    // Stronger then all others
    ordered_troops.push_back(soldier);
}

void nobMilitary::CancelOrders()
{
    // Soldaten zurÃ¼ckschicken
    for(std::list<nofPassiveSoldier*>::iterator it = ordered_troops.begin(); it != ordered_troops.end(); ++it)
        (*it)->NotNeeded();

    ordered_troops.clear();

    // GoldmÃ¼nzen zurÃ¼ckschicken
    for(std::list<Ware*>::iterator it = ordered_coins.begin(); it != ordered_coins.end(); ++it)
        WareNotNeeded(*it);

    ordered_coins.clear();
}

void nobMilitary::AddActiveSoldier(nofActiveSoldier* soldier)
{


    // aktiver Soldat, eingetroffen werden --> dieser muss erst in einen passiven Soldaten
    // umoperiert werden (neu erzeugt und alter zerstÃ¶rt) werden
    nofPassiveSoldier* passive_soldier = new nofPassiveSoldier(*soldier);

    // neuen Soldaten einhÃ¤ngen
    AddPassiveSoldier(passive_soldier);

    // alten Soldaten spÃ¤ter vernichten
    em->AddToKillList(soldier);

    // Soldat ist wie tot, d.h. er muss aus allen Missionslisten etc. wieder rausgenommen werden
    SoldierLost(soldier);
}

void nobMilitary::AddPassiveSoldier(nofPassiveSoldier* soldier)
{
    assert(soldier->GetPlayer() == player);
    assert(troops.size() < unsigned(TROOPS_COUNT[nation][size]));

    bool inserted = false;

    for(std::list<nofPassiveSoldier*>::iterator it = troops.begin(); it != troops.end(); ++it) {
        // Insert before current, if rank is smaller
        if(soldier->GetRank() < (*it)->GetRank()) {
            troops.insert(it, soldier);
            inserted = true;
            break;
        }
    }

    if(!inserted){
        // Stronger then all others
        troops.push_back(soldier);
    }

    // und aus den bestllten Truppen raushauen, da er ja jetzt hier ist
    ordered_troops.remove(soldier);

    // Wurde dieses GebÃ¤ude zum ersten Mal besetzt?
    if(new_built)
    {
        if(GAMECLIENT.GetPlayerID() == this->player)
            GAMECLIENT.SendPostMessage(new ImagePostMsgWithLocation(_("Military building occupied"), PMC_MILITARY, pos, this->type, this->nation));
        // Ist nun besetzt
        new_built = false;
        // Landgrenzen verschieben
        gwg->RecalcTerritory(this, MILITARY_RADIUS[size], false, true);
        // TÃ¼r zumachen
        CloseDoor();
        // Fanfarensound abspieln, falls das MilitÃ¤rgebÃ¤ude im Sichtbereich ist und unseres ist
        gwg->MilitaryBuildingCaptured(pos, player);
        // AIEvent senden an besitzer
        GAMECLIENT.SendAIEvent(new AIEvent::Building(AIEvent::BuildingConquered, pos, type), player);
    }
    else
    {
        // Evtl. Soldaten befÃ¶rdern
        PrepareUpgrading();
    }

    // GoldmÃ¼nzen suchen, evtl sinds ja neue Soldaten
    SearchCoins();
}


void nobMilitary::SoldierLost(nofSoldier* soldier)
{
    // Soldat konnte nicht (mehr) kommen --> rauswerfen und ggf. neue Soldaten rufen
    ordered_troops.remove(static_cast<nofPassiveSoldier*>(soldier));
    troops_on_mission.remove(static_cast<nofActiveSoldier*>(soldier));
    RegulateTroops();
}

void nobMilitary::SoldierOnMission(nofPassiveSoldier* passive_soldier, nofActiveSoldier* active_soldier)
{
    // Aus der Besatzungsliste raushauen, aber noch mit merken
    troops.remove(passive_soldier);
    troops_on_mission.push_back(active_soldier);
}

nofPassiveSoldier* nobMilitary::ChooseSoldier()
{
    if(troops.empty())
        return 0;

    nofPassiveSoldier* candidates[5] = {NULL, NULL, NULL, NULL, NULL}; // candidates per rank

    // how many ranks
    unsigned rank_count = 0;

    for(std::list<nofPassiveSoldier*>::iterator it = troops.begin(); it != troops.end(); ++it)
    {
        if(!candidates[(*it)->GetRank()])
        {
            ++rank_count;
            candidates[(*it)->GetRank()] = *it;
        }
    }

    // ID ausrechnen
    unsigned rank = ((rank_count - 1) * gwg->GetPlayer(player)->military_settings[1]) / MILITARY_SETTINGS_SCALE[1];

    unsigned r = 0;

    // richtigen Rang suchen
    for(unsigned i = 0; i < 5; ++i)
    {
        if(candidates[i])
        {
            if(r == rank)
                // diesen Soldaten wollen wir
                return candidates[i];

            ++r;
        }
    }

    return NULL;
}

nofAggressiveDefender* nobMilitary::SendDefender(nofAttacker* attacker)
{
    // Sind noch Soldaten da?
    if(troops.size() > 1)
    {
        // Verteidiger auswÃ¤hlen
        nofPassiveSoldier* soldier = ChooseSoldier();
        // neuen aggressiven Verteidiger daraus erzeugen
        nofAggressiveDefender* ad = new nofAggressiveDefender(soldier, attacker);
        // soll rausgehen
        AddLeavingFigure(ad);
        // auf die Missionsliste setzen
        troops_on_mission.push_back(ad);
        // aus den Truppen rauswerfen
        troops.remove(soldier);
        // alten passiven Soldaten vernichten
        soldier->Destroy();
        delete soldier;

        return ad;
    }
    else
        return 0;
}

/// Gibt die Anzahl der Soldaten zurÃ¼ck, die fÃ¼r einen Angriff auf ein bestimmtes Ziel zur VerfÃ¼gung stehen
unsigned nobMilitary::GetSoldiersForAttack(const MapPoint dest, const unsigned char player_attacker) const
{
    // Soldaten ausrechnen, wie viel man davon nehmen kÃ¶nnte, je nachdem wie viele in den
    // MilitÃ¤reinstellungen zum Angriff eingestellt wurden
    unsigned short soldiers_count =
        (GetTroopsCount() > 1) ?
        ((GetTroopsCount() - 1) * players->getElement(player_attacker)->military_settings[3] / 5) : 0;

    unsigned int distance = gwg->CalcDistance(pos, dest);

    // Falls Entfernung grÃ¶ÃŸer als Basisreichweite, Soldaten subtrahieren
    if (distance > BASE_ATTACKING_DISTANCE)
    {
        // je einen soldaten zum entfernen vormerken fÃ¼r jeden EXTENDED_ATTACKING_DISTANCE groÃŸen Schritt
        unsigned short soldiers_to_remove = ((distance - BASE_ATTACKING_DISTANCE + EXTENDED_ATTACKING_DISTANCE - 1) / EXTENDED_ATTACKING_DISTANCE);
        if (soldiers_to_remove < soldiers_count)
            soldiers_count -= soldiers_to_remove;
        else
            return 0;
    }

    // und auch der Weg zu FuÃŸ darf dann nicht so weit sein, wenn das alles bestanden ist, kÃ¶nnen wir ihn nehmen..
    if(soldiers_count && gwg->FindHumanPath(pos, dest, MAX_ATTACKING_RUN_DISTANCE, false, NULL, false) != 0xFF)
        // Soldaten davon nehmen
        return soldiers_count;
    else
        return 0;
}

/// Gibt die Soldaten zurÃ¼ck, die fÃ¼r einen Angriff auf ein bestimmtes Ziel zur VerfÃ¼gung stehen
void nobMilitary::GetSoldiersForAttack(const MapPoint dest,
                                       const unsigned char player_attacker, std::vector<nofPassiveSoldier*>& soldiers) const
{
    unsigned soldiers_count = GetSoldiersForAttack(dest, player_attacker);
    for(std::list<nofPassiveSoldier*>::const_reverse_iterator it = troops.rbegin(); it != troops.rend() && soldiers_count; ++it, --soldiers_count)
    {
        soldiers.push_back(*it);
    }

}

/// Gibt die StÃ¤rke der Soldaten zurÃ¼ck, die fÃ¼r einen Angriff auf ein bestimmtes Ziel zur VerfÃ¼gung stehen
unsigned nobMilitary::GetSoldiersStrengthForAttack(const MapPoint dest,
        const unsigned char player_attacker, unsigned& count) const
{
    unsigned strength = 0;

    unsigned soldiers_count = GetSoldiersForAttack(dest, player_attacker);
    count = soldiers_count;

    for(std::list<nofPassiveSoldier*>::const_reverse_iterator it = troops.rbegin(); it != troops.rend() && soldiers_count; ++it, --soldiers_count)
    {
        strength += HITPOINTS[nation][(*it)->GetRank()];
    }

    return(strength);
}

/// Gibt die StÃ¤rke eines MilitÃ¤rgebÃ¤udes zurÃ¼ck
unsigned nobMilitary::GetSoldiersStrength() const
{
    unsigned strength = 0;

    for(std::list<nofPassiveSoldier*>::const_iterator it = troops.begin(); it != troops.end(); ++it)
    {
        strength += HITPOINTS[nation][(*it)->GetRank()];
    }

    return(strength);
}

/// is there a max rank soldier in the building?
unsigned nobMilitary::HasMaxRankSoldier() const
{
	unsigned count=0;
    for(std::list<nofPassiveSoldier*>::const_reverse_iterator it = troops.rbegin(); it != troops.rend(); ++it)
    {
		if ((*it)->GetRank() >= (MAX_MILITARY_RANK - GAMECLIENT.GetGGS().getSelection(ADDON_MAX_RANK)))
			count++;
    }
	return count;
}

nofDefender* nobMilitary::ProvideDefender(nofAttacker* const attacker)
{
    // Ãœberhaupos Soldaten da?
    if(troops.empty())
    {
        /// Soldaten, die noch auf Mission gehen wollen, canceln und fÃ¼r die Verteidigung mit einziehen
        CancelJobs();
        // Nochmal versuchen
        if(troops.empty())
            return NULL;
    }


    nofPassiveSoldier* soldier = ChooseSoldier();

    // neuen Verteidiger erzeugen
    nofDefender* defender = new nofDefender(soldier, attacker);

    // aus der Liste entfernen
    troops.remove(soldier);

    // und vernichten
    soldier->Destroy();
    delete soldier;

    return defender;
}

void nobMilitary::Capture(const unsigned char new_owner)
{
    captured_not_built = true;

    // GoldmÃ¼nzen in der Inventur vom alten Spieler abziehen und dem neuen hinzufÃ¼gen
    gwg->GetPlayer(player)->DecreaseInventoryWare(GD_COINS, coins);
    gwg->GetPlayer(new_owner)->IncreaseInventoryWare(GD_COINS, coins);

    // Soldaten, die auf Mission sind, Bescheid sagen
    for(std::list<nofActiveSoldier*>::iterator it = troops_on_mission.begin(); it != troops_on_mission.end(); ++it)
        (*it)->HomeDestroyed();

    // Bestellungen die hierher unterwegs sind canceln
    CancelOrders();

    // Aggressiv-Verteidigenden Soldaten Bescheid sagen, dass sie nach Hause gehen kÃ¶nnen
    for(std::list<nofAggressiveDefender*>::iterator it = aggressive_defenders.begin(); it != aggressive_defenders.end(); ++it)
        (*it)->AttackedGoalDestroyed();

    troops_on_mission.clear();
    aggressive_defenders.clear();

    // In der Wirtschaftsverwaltung dieses GebÃ¤ude jetzt zum neuen Spieler zÃ¤hlen und beim alten raushauen
    gwg->GetPlayer(player)->RemoveMilitaryBuilding(this);
    gwg->GetPlayer(new_owner)->AddMilitaryBuilding(this);

    // Alten Besitzer merken
    unsigned char old_player = player;

    // neuer Spieler
    player = new_owner;

    // Flagge davor auch Ã¼bernehmen
    GetFlag()->Capture(new_owner);

    // Territorium neu berechnen
    gwg->RecalcTerritory(this, MILITARY_RADIUS[size], false, false);

    // Sichtbarkeiten berechnen fÃ¼r alten Spieler
    gwg->RecalcVisibilitiesAroundPoint(pos, GetMilitaryRadius() + VISUALRANGE_MILITARY + 1, old_player, NULL);

    // Grenzflagge entsprechend neu setzen von den Feinden
    LookForEnemyBuildings();
    // und von den VerbÃ¼ndeten (da ja ein FeindgebÃ¤ude weg ist)!
    std::set<nobBaseMilitary*> buildings = gwg->LookForMilitaryBuildings(pos, 4);

    for(std::set<nobBaseMilitary*>::iterator it = buildings.begin(); it != buildings.end(); ++it)
    {
        // verbÃ¼ndetes GebÃ¤ude?
        if(gwg->GetPlayer((*it)->GetPlayer())->IsPlayerAttackable(old_player)
                && (*it)->GetBuildingType() >= BLD_BARRACKS && (*it)->GetBuildingType() <= BLD_FORTRESS)
            // Grenzflaggen von dem neu berechnen
            static_cast<nobMilitary*>(*it)->LookForEnemyBuildings();
    }

    // ehemalige Leute dieses GebÃ¤udes nach Hause schicken, die ggf. grad auf dem Weg rein/raus waren
    MapPoint coords[2] = {pos, MapPoint(gwg->GetNeighbour(pos, 4))};
    for(unsigned short i = 0; i < 2; ++i)
    {
        const std::list<noBase*>& figures = gwg->GetFigures(coords[i]);
        for(std::list<noBase*>::const_iterator it = figures.begin(); it != figures.end(); ++it)
        {
            if((*it)->GetType() == NOP_FIGURE)
            {
                if(static_cast<noFigure*>(*it)->GetCurrentRoad() == routes[4] && static_cast<noFigure*>(*it)->GetPlayer() != new_owner)
                {
                    static_cast<noFigure*>(*it)->Abrogate();
                    static_cast<noFigure*>(*it)->StartWandering();
                }
            }
        }
    }

    // GebÃ¤ude wird nun eingenommen
    capturing = true;

    // Soldat, der zum Erobern reinlÃ¤uft, ist nun drinne --> Anzahl der erobernden Soldaten entsprechend verringern
    assert(capturing_soldiers);
    --capturing_soldiers;

    // Fanfarensound abspieln, falls das MilitÃ¤rgebÃ¤ude im Sichtbereich ist und unseres ist
    gwg->MilitaryBuildingCaptured(pos, player);

    // Post verschicken, an den alten Besitzer und an den neuen Besitzer
    if(GAMECLIENT.GetPlayerID() == old_player)
        GAMECLIENT.SendPostMessage(
            new ImagePostMsgWithLocation(_("Military building lost"), PMC_MILITARY, pos, GetBuildingType(), GetNation()));
    if(GAMECLIENT.GetPlayerID() == this->player)
        GAMECLIENT.SendPostMessage(
            new ImagePostMsgWithLocation(_("Military building captured"), PMC_MILITARY, pos, GetBuildingType(), GetNation()));

    // ggf. Fenster schlieÃŸen vom alten Spieler
    gwg->ImportantObjectDestroyed(pos);

    // AIEvent senden an gewinner&verlierer
    GAMECLIENT.SendAIEvent(new AIEvent::Building(AIEvent::BuildingConquered, pos, type), player);
    GAMECLIENT.SendAIEvent(new AIEvent::Building(AIEvent::BuildingLost, pos, type), old_player);

}

void nobMilitary::NeedOccupyingTroops(const unsigned char new_owner)
{
    // Brauchen wir noch Soldaten (ein Soldat kommt ja noch rein), keine Soldaten von anderen Spielern
    // wÃ¤hlen (z.B. "Kollektivangriffen"), manchmal ist es egal, wer reinkommt (new_owner == 0xFF)

    // Soldaten wÃ¤hlen, der am nÃ¤chsten an der Flagge steht, damit nicht welche von ganze hinten ewige Zeit vor
    // latschen mÃ¼ssen
    nofAttacker* best_attacker = 0;
    unsigned best_radius = std::numeric_limits<unsigned>::max();

    unsigned needed_soldiers = unsigned(CalcTroopsCount());

    if(needed_soldiers > troops.size() + capturing_soldiers + troops_on_mission.size() + ordered_troops.size())
    {
        // Soldaten absuchen
        for(std::list<nofAttacker*>::iterator it = aggressors.begin(); it != aggressors.end(); ++it)
        {
            // Steht der Soldat Ã¼berhaupos um das GebÃ¤ude rum?
            if((*it)->IsAttackerReady() && ((*it)->GetPlayer() == new_owner || new_owner == 0xFF))
            {
                // NÃ¤her als der bisher beste?
                if((*it)->GetRadius() < best_radius)
                {
                    // Und kommt er Ã¼berhaupos zur Flagge (kÃ¶nnte ja in der 2. Reihe stehen, sodass die
                    // vor ihm ihn den Weg versperren)?
                    if(gwg->FindHumanPath((*it)->GetPos(), gwg->GetNeighbour(pos, 4), 10, false) != 0xFF)
                    {
                        // Dann is das der bisher beste
                        best_attacker = *it;
                        best_radius = best_attacker->GetRadius();
                    }
                }
            }
        }

        // Einen gefunden?
        if(best_attacker)
        {
            // Dann soll der hingehen
            best_attacker->CaptureBuilding();
            ++capturing_soldiers;
            // und raus hier
            return;
        }

        // keine Soldaten mehr gefunden, der Rest (der noch nicht da ist) kann wieder nach Hause gehen
        for(std::list<nofAttacker*>::iterator it = aggressors.begin(); it != aggressors.end(); )
        {
            nofAttacker* attacker = *it;

            // If necessary look for further soldiers who are not standing around the building
            if(needed_soldiers > troops.size() + capturing_soldiers +
                    troops_on_mission.size() + ordered_troops.size() + far_away_capturers.size()
                    && attacker->GetPlayer() == player)
            {
                // Ask attacker if this is possible
                if(attacker->TryToStartFarAwayCapturing(this))
                {
                    it = aggressors.erase(it);
                    far_away_capturers.push_back(attacker);
                    continue;
                }
            }

			//LOG.lprintf("no more capture troops detected: rest can go home (target was: %i,%i) ",x,y);
            // Nicht gerade Soldaten lÃ¶schen, die das GebÃ¤ude noch einnehmen!
            if(attacker->GetState() != nofActiveSoldier::STATE_ATTACKING_CAPTURINGNEXT && !gwg->GetPlayer(attacker->GetPlayer())->IsPlayerAttackable(player))
            {
                // Attention: Get new it first as attacker can be deleted in CapturedBuildingFull
                it = aggressors.erase(it);
                attacker->CapturedBuildingFull();
            }else
                ++it;
        }

        // Einnahme beendet
        capturing = false;
    }
    else
    {
        // keine Soldaten mehr benÃ¶tigt, der Rest kann wieder nach Hause gehen
		//LOG.lprintf("building full: remaining attackers can go home (target was: %i,%i) \n ",x,y);t;
        for(std::list<nofAttacker*>::iterator it = aggressors.begin(); it != aggressors.end();)
        {
            nofAttacker* attacker = *it;
            // Nicht gerade Soldaten lÃ¶schen, die das GebÃ¤ude noch einnehmen!
			//also: dont remove attackers owned by players not allied with the new owner!
			if(attacker->GetState() != nofActiveSoldier::STATE_ATTACKING_CAPTURINGNEXT && !gwg->GetPlayer(attacker->GetPlayer())->IsPlayerAttackable(player))
            {
			    // Attention: Get new it first as attacker can be deleted in CapturedBuildingFull
			    it = aggressors.erase(it);
                attacker->CapturedBuildingFull();
            }else
                ++it;
        }

        // Einnahme beendet
        capturing = false;

        // Nun die Besetzung prÃ¼fen
        RegulateTroops();
    }
}

void nobMilitary::StopGold()
{
    // Umstellen
    disable_coins = !disable_coins;
    // Wenn das von einem fremden Spieler umgestellt wurde (oder vom Replay), muss auch das visuelle umgestellt werden
    if(GAMECLIENT.GetPlayerID() != player || GAMECLIENT.IsReplayModeOn())
        disable_coins_virtual = !disable_coins_virtual;

    if(!disable_coins)
        SearchCoins(); // Order coins if we just enabled it
    else
    {
        // send coins back if just deactivated
        for(std::list<Ware*>::iterator it = ordered_coins.begin(); it != ordered_coins.end(); ++it)
            WareNotNeeded(*it);

        ordered_coins.clear();
    }
}


unsigned nobMilitary::CalcCoinsPoints()
{
    // Will ich Ã¼berhaupos GoldmÃ¼nzen, wenn nich, sofort raus
    if(!WantCoins())
        return 0;

    // 10000 als Basis wÃ¤hlen, damit man auch noch was abziehen kann
    int points = 10000;

    // Wenn hier schon MÃ¼nzen drin sind oder welche bestellt sind, wirkt sich das natÃ¼rlich negativ auf die "Wichtigkeit" aus
    points -= (coins + ordered_coins.size()) * 30;

    // BefÃ¶rderbare Soldaten zÃ¤hlen
    for(std::list<nofPassiveSoldier*>::iterator it = troops.begin(); it != troops.end(); ++it)
    {
        // Solange es kein Max Rank (default 4) ist, kann der Soldat noch befÃ¶rdert werden
        if((*it)->GetRank() < 4 - GAMECLIENT.GetGGS().getSelection(ADDON_MAX_RANK))
            points += 20;
    }

    if(points < 0)
        throw std::logic_error("Negative points are not allowed");

    return static_cast<unsigned>(points);
}

bool nobMilitary::WantCoins()
{
    // Wenn die Goldzufuhr gestoppos wurde oder MÃ¼nzvorrat voll ist, will ich gar keine GoldmÃ¼nzen
    return (!disable_coins && coins + ordered_coins.size() != GOLD_COUNT[nation][size] && !new_built);
}

void nobMilitary::SearchCoins()
{
    // Brauche ich Ã¼berhaupos GoldmÃ¼nzen bzw. hab ich vielleicht schon ein Event angemeldet?
    if(WantCoins() && !goldorder_event)
    {
        // Lagerhaus mit GoldmÃ¼nzen suchen
        FW::Param_Ware p = {GD_COINS, 1};
        if(nobBaseWarehouse* wh = gwg->GetPlayer(player)->FindWarehouse(this, FW::Condition_Ware, 0, false, &p, false))
        {
            // Wenns eins gibt, dort eine GoldmÃ¼nze bestellen
            Ware* ware = wh->OrderWare(GD_COINS, this);

            if(!ware)
            {
                // Ware dÃ¼rfte nicht 0 werden, da ja ein Lagerhaus MIT GOLDMÃœNZEN bereits gesucht wird
                LOG.lprintf("nobMilitary::SearchCoins: WARNING: ware = 0. Bug alarm!\n");
                return;
            }

            // GoldmÃ¼nze zu den Bestellungen hinzufÃ¼gen
            ordered_coins.push_back(ware);

            // Nach einer Weile nochmal nach evtl neuen GoldmÃ¼nzen gucken
            goldorder_event = em->AddEvent(this, 200 + RANDOM.Rand(__FILE__, __LINE__, obj_id, 400), 1);
        }
    }
}

void nobMilitary::PrepareUpgrading()
{
    // GoldmÃ¼nzen da?
    if(!coins)
        return;

    // Gibts auch noch kein BefÃ¶rderungsevent?
    if(upgrade_event)
        return;

    // Noch Soldaten, die befÃ¶rdert werden kÃ¶nnen?
    bool soldiers_available = false;

    for(std::list<nofPassiveSoldier*>::iterator it = troops.begin(); it != troops.end(); ++it)
    {
        if((*it)->GetRank() < 4 - GAMECLIENT.GetGGS().getSelection(ADDON_MAX_RANK))
        {
            // es wurde ein Soldat gefunden, der befÃ¶rdert werden kann
            soldiers_available = true;
            break;
        }
    }

    if(!soldiers_available)
        return;

    // Alles da --> BefÃ¶rderungsevent anmelden
    upgrade_event = em->AddEvent(this, UPGRADE_TIME + RANDOM.Rand(__FILE__, __LINE__, obj_id, UPGRADE_TIME_RANDOM), 2);
}

void nobMilitary::HitOfCatapultStone()
{
    // Ein Soldat weniger, falls es noch welche gibt
    if(!troops.empty())
    {
        troops.front()->Die();
        troops.pop_front();
    }

    // Kein Soldat mehr da? Haus abfackeln
    if(troops.empty())
        Destroy();
    else
        // ansonsten noch neue Soldaten ggf. bestellen
        RegulateTroops();

    // Post verschicken
    if(GAMECLIENT.GetPlayerID() == this->player)
        GAMECLIENT.SendPostMessage(
            new ImagePostMsgWithLocation(_("A catapult is firing upon us!"), PMC_MILITARY, pos, GetBuildingType(), GetNation()));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Darf das MilitÃ¤rgebÃ¤ude abgerissen werden (Abriss-Verbot berÃ¼cksichtigen)?
 *
 *  @author OLiver
 */
bool nobMilitary::IsDemolitionAllowed() const
{
    switch(GAMECLIENT.GetGGS().getSelection(ADDON_DEMOLITION_PROHIBITION))
    {
        default: // off
            break;
        case 1: // under attack
        {
            // PrÃ¼fen, ob das GebÃ¤ude angegriffen wird
            if(!aggressors.empty())
                return false;
        } break;
        case 2: // near frontiers
        {
            // PrÃ¼fen, ob es in GrenznÃ¤he steht
            if(frontier_distance == 3)
                return false;
        } break;
    }

    return true;
}

void nobMilitary::UnlinkAggressor(nofAttacker* soldier)
{
    aggressors.remove(soldier);
    far_away_capturers.remove(soldier);

    if (aggressors.size() == 0)
        RegulateTroops();
}


/// A far-away capturer arrived at the building/flag and starts the capturing
void nobMilitary::FarAwayAttackerReachedGoal(nofAttacker* attacker)
{
    far_away_capturers.remove(attacker);
    aggressors.push_back(attacker);
    capturing_soldiers++;
}


