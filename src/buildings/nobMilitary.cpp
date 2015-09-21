// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#include "helpers/containerUtils.h"

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
    // Gebäude entsprechend als Militärgebäude registrieren und in ein Militärquadrat eintragen
    gwg->GetPlayer(player)->AddMilitaryBuilding(this);
    gwg->GetMilitarySquare(pos).push_back(this);

    // Größe ermitteln
    switch(type)
    {
        case BLD_BARRACKS: size = 0; break;
        case BLD_GUARDHOUSE: size = 1; break;
        case BLD_WATCHTOWER: size = 2; break;
        case BLD_FORTRESS: size = 3; break;
        default: size = 0xFF; break;
    }

    LookForEnemyBuildings();

    // Tür aufmachen, bis Gebäude besetzt ist
    OpenDoor();

    // Wenn kein Gold in neu gebaute Militärgebäude eingeliefert werden soll, wird die Goldzufuhr gestoppos
    // Ansonsten neue Goldmünzen anfordern
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
    for(SortedTroops::iterator it = troops.begin(); it != troops.end(); ++it)
        delete (*it);
}

size_t nobMilitary::GetTotalSoldiers() const
{
    size_t sum = troops.size() + ordered_troops.size() + troops_on_mission.size();
    if(defender_ && (defender_->IsWaitingAtFlag() || defender_->IsFightingAtFlag()))
        sum++;
    sum += /* capturing_soldiers*/ + far_away_capturers.size();
    return sum;
}

void nobMilitary::Destroy_nobMilitary()
{

    // Bestellungen stornieren
    CancelOrders();

    // Soldaten rausschicken
    for(SortedTroops::iterator it = troops.begin(); it != troops.end(); ++it)
        (*it)->InBuildingDestroyed();

    // Inform far-away capturers
    for(std::list<nofAttacker*>::iterator it = far_away_capturers.begin(); it != far_away_capturers.end(); ++it)
        (*it)->AttackedGoalDestroyed();

    troops.clear();

    // Events ggf. entfernen
    em->RemoveEvent(goldorder_event);
    em->RemoveEvent(upgrade_event);

    // übriggebliebene Goldmünzen in der Inventur abmelden
    gwg->GetPlayer(player)->DecreaseInventoryWare(GD_COINS, coins);

    Destroy_nobBaseMilitary();

    // Wieder aus dem Militärquadrat rauswerfen
    gwg->GetPlayer(player)->RemoveMilitaryBuilding(this);
    gwg->GetMilitarySquare(pos).remove(this);

    // Land drumherum neu berechnen (nur wenn es schon besetzt wurde!)
    // Nach dem BaseDestroy erst, da in diesem erst das Feuer gesetzt, die Straße gelöscht wird usw.
    if(!new_built)
        gwg->RecalcTerritory(this, MILITARY_RADIUS[size], true, false);

    GAMECLIENT.SendAIEvent(new AIEvent::Building(AIEvent::BuildingLost, pos, type_), player);

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

    sgd->PushObjectContainer(ordered_troops, true);
    sgd->PushObjectContainer(ordered_coins, true);
    sgd->PushObjectContainer(troops, true);
    sgd->PushObjectContainer(far_away_capturers, true);
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


    sgd->PopObjectContainer(ordered_troops, GOT_NOF_PASSIVESOLDIER);
    sgd->PopObjectContainer(ordered_coins, GOT_WARE);
    sgd->PopObjectContainer(troops, GOT_NOF_PASSIVESOLDIER);
    sgd->PopObjectContainer(far_away_capturers, GOT_NOF_ATTACKER);

    // ins Militärquadrat einfügen
    gwg->GetMilitarySquare(pos).push_back(this);
}


void nobMilitary::Draw(int x, int y)
{
    // Gebäude an sich zeichnen
    DrawBaseBuilding(x, y);


    // (max 4) Besatzungs-Fähnchen zeichnen
    unsigned flags = min<unsigned>(troops.size() + this->leave_house.size(), 4);

    for(unsigned i = 0; i < flags; ++i)
        LOADER.GetMapImageN(3162 + GAMECLIENT.GetGlobalAnimation(8, 2, 1, pos.x * pos.y * i))->Draw(x + TROOPS_FLAGS[nation][size][0], y + TROOPS_FLAGS[nation][size][1] + (i) * 3, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);

    // Die Fahne, die anzeigt wie weit das Gebäude von der Grenze entfernt ist, zeichnen
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

    // Wenn Goldzufuhr gestoppos ist, Schild außen am Gebäude zeichnen zeichnen
    if(disable_coins_virtual)
        LOADER.GetMapImageN(46)->Draw(x + BUILDING_SIGN_CONSTS[nation][type_].x, y + BUILDING_SIGN_CONSTS[nation][type_].y, 0, 0, 0, 0, 0, 0);



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

            // Wenn noch weitere drin sind, die müssen auch noch raus
            if(!leave_house.empty())
                leaving_event = em->AddEvent(this, 30 + RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 10));
            else
                go_out = false;

        } break;
        // Goldbestell-Event
        case 1:
        {
            goldorder_event = 0;

            // ggf. nach neuen Goldmünzen suchen
            SearchCoins();
        } break;
        // Beförderungs-Event
        case 2:
        {
            upgrade_event = 0;

            // Soldaten befördern
            // Von hinten durchgehen
            // Wenn der nachfolgende (schwächere) Soldat einen niedrigeren Rang hat,
            // wird dieser ebenfalls befördert usw.!
            std::vector<nofPassiveSoldier*> upgradedSoldiers;
            // Rang des letzten beförderten Soldaten, 4-MaxRank am Anfang setzen, damit keiner über den maximalen Rang befördert wird
            unsigned char last_rank = GAMECLIENT.GetGGS().GetMaxMilitaryRank();
            for(SortedTroops::reverse_iterator it = troops.rbegin(); it != troops.rend();)
            {
                // Es wurde schon einer befördert, dieser Soldat muss nun einen niedrigeren Rang
                // als der letzte haben, damit er auch noch befördert werden kann
                if((*it)->GetRank() < last_rank)
                {
                    nofPassiveSoldier* soldier = *it;
                    // Rang merken
                    last_rank = soldier->GetRank();
                    // Remove from sorted container as changing it breaks sorting
                    it = helpers::erase(troops, it);
                    // Dann befördern
                    soldier->Upgrade();
                    upgradedSoldiers.push_back(soldier);
                }else
                    ++it;
            }

            // Wurde jemand befördert?
            if(!upgradedSoldiers.empty())
            {
                // Reinsert upgraded soldiers
                for(std::vector<nofPassiveSoldier*>::iterator it = upgradedSoldiers.begin(); it != upgradedSoldiers.end(); ++it)
                    troops.insert(*it);

                // Goldmünze verbrauchen
                --coins;
                gwg->GetPlayer(player)->DecreaseInventoryWare(GD_COINS, 1);

                // Evtl neues Beförderungsevent anmelden
                PrepareUpgrading();

                // Ggf. neue Goldmünzen bestellen
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
    // Umgebung nach Militärgebäuden absuchen
    sortedMilitaryBlds buildings = gwg->LookForMilitaryBuildings(pos, 3);
    frontier_distance = 0;

    for(sortedMilitaryBlds::iterator it = buildings.begin(); it != buildings.end(); ++it)
    {
        // feindliches Militärgebäude?
        if(*it != exceposion && (*it)->GetPlayer() != player && gwg->GetPlayer((*it)->GetPlayer())->IsPlayerAttackable(player))
        {
            unsigned distance = gwg->CalcDistance(pos, (*it)->GetPos());

            // in nahem Umkreis, also Grenzen berühren sich
            if(distance <= MILITARY_RADIUS[size] + (*it)->GetMilitaryRadius()) // warum erzeugtn das ne warning in vs2008?
            {
                // Grenznähe entsprechend setzen
                frontier_distance = 3;

                // Wenns ein richtiges Militärgebäude ist, dann dort auch entsprechend setzen
                if((*it)->GetBuildingType() >= BLD_BARRACKS && (*it)->GetBuildingType() <= BLD_FORTRESS)
                    static_cast<nobMilitary*>(*it)->NewEnemyMilitaryBuilding(3);
            }
            // in mittlerem Umkreis, also theoretisch angreifbar?
            else if(distance < BASE_ATTACKING_DISTANCE
                    + (TROOPS_COUNT[nation][size] - 1) * EXTENDED_ATTACKING_DISTANCE)
            {
                // Grenznähe entsprechend setzen
                if(!frontier_distance)
                    frontier_distance = 1;

                // Wenns ein richtiges Militärgebäude ist, dann dort auch entsprechend setzen
                if((*it)->GetBuildingType() >= BLD_BARRACKS && (*it)->GetBuildingType() <= BLD_FORTRESS)
                    static_cast<nobMilitary*>(*it)->NewEnemyMilitaryBuilding(1);
            }
            // andere Richtung muss auch getestet werden, zumindest wenns eine normaler Militärgebäude ist, Bug 389843
            else if ((*it)->GetGOT() == GOT_NOB_MILITARY)
            {
                nobMilitary* mil = dynamic_cast<nobMilitary*>(*it);
                if(distance < BASE_ATTACKING_DISTANCE + (TROOPS_COUNT[mil->nation][mil->size] - 1) * EXTENDED_ATTACKING_DISTANCE)
                {
                    // Grenznähe entsprechend setzen
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
    // Neues Grenzgebäude in der Nähe --> Distanz entsprechend setzen
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
    // Wenn das Gebäude eingenommen wird, erstmal keine neuen Truppen und warten, wieviele noch reinkommen
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
        // Zu viel --> überflüssige Truppen nach Hause schicken
        // Zuerst die bestellten Soldaten wegschicken
        // Weak ones first
        std::vector<nofPassiveSoldier*> notNeededSoldiers;
        if (gwg->GetPlayer(player)->militarySettings_[1] > MILITARY_SETTINGS_SCALE[1] / 2)
        {
            for(SortedTroops::iterator it = ordered_troops.begin(); diff && !ordered_troops.empty(); ++diff)
            {
                notNeededSoldiers.push_back(*it);
                it = helpers::erase(ordered_troops, it);
            }
        }
        // Strong ones first
        else
        {
            for(SortedTroops::reverse_iterator it = ordered_troops.rbegin(); diff && !ordered_troops.empty(); ++diff)
            {
                notNeededSoldiers.push_back(*it);
                it = helpers::erase(ordered_troops, it);
            }
        }

        // send the not-needed-soldiers away
        for (std::vector<nofPassiveSoldier*>::iterator it = notNeededSoldiers.begin(); it != notNeededSoldiers.end(); ++it)
        {
            (*it)->NotNeeded();
        }

        // Nur rausschicken, wenn es einen Weg zu einem Lagerhaus gibt!
        if(gwg->GetPlayer(player)->FindWarehouse(this, FW::NoCondition, 0, true, 0, false))
        {
            // Dann den Rest (einer muss immer noch drinbleiben!)
            // erst die schwachen Soldaten raus
            if (gwg->GetPlayer(player)->militarySettings_[1] > MILITARY_SETTINGS_SCALE[1] / 2)
            {
                for(SortedTroops::iterator it = troops.begin(); diff && troops.size() > 1; ++diff)
                {
                    (*it)->LeaveBuilding();
                    AddLeavingFigure(*it);
                    it = helpers::erase(troops, it);
                }
            }
            // erst die starken Soldaten raus
            else
            {
                for(SortedTroops::reverse_iterator it = troops.rbegin(); diff && troops.size() > 1; ++diff)
                {
                    (*it)->LeaveBuilding();
                    AddLeavingFigure(*it);
                    it = helpers::erase(troops, it);
                }
            }
        }

    }
    else if(diff > 0)
    {
        // Zu wenig Truppen

        // Gebäude wird angegriffen und
        // Addon aktiv, nur soviele Leute zum Nachbesetzen schicken wie Verteidiger eingestellt
        if (aggressors.size() > 0 && GAMECLIENT.GetGGS().getSelection(ADDON_DEFENDER_BEHAVIOR) == 2)
        {
            diff = (gwg->GetPlayer(player)->militarySettings_[2] * diff) / MILITARY_SETTINGS_SCALE[2];
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
    return (TROOPS_COUNT[nation][size] - 1) * gwg->GetPlayer(player)->militarySettings_[4 + frontier_distance] / MILITARY_SETTINGS_SCALE[4 + frontier_distance] + 1;
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
        for(SortedTroops::reverse_iterator it = troops.rbegin(); diff && troops.size() > 1; ++diff)
        {
            if(mrank<0) //set mrank = highest rank
                mrank=(*it)->GetRank();
            else if (mrank>(*it)->GetRank()) //if the current soldier is of lower rank than what we started with -> send no more troops out
                return;
            (*it)->LeaveBuilding();
            AddLeavingFigure(*it);
            it = helpers::erase(troops, it);
        }
	}
}

//used by the ai to refill the upgradebuilding with low rank soldiers! - normal orders for soldiers are done in RegulateTroops!
void nobMilitary::OrderNewSoldiers()
{
	//cancel all max ranks on their way to this building
	std::vector<nofPassiveSoldier*> noNeed;
	for(SortedTroops::iterator it = ordered_troops.begin(); it != ordered_troops.end(); )
    {
		if((*it)->GetRank() >= GAMECLIENT.GetGGS().GetMaxMilitaryRank())
		{
			nofPassiveSoldier* soldier = *it;
			it = helpers::erase(ordered_troops, it);
			noNeed.push_back(soldier);
		}else
		    ++it;
    }

	int diff = CalcTroopsCount() - static_cast<int>(GetTotalSoldiers());
	//order new troops now
    if(diff > 0) //poc: this should only be >0 if we are being captured. capturing should be true until its the last soldier and this last one would count twice here and result in a returning soldier that shouldnt return.
	{
		// Zu wenig Truppen
        // Gebäude wird angegriffen und
        // Addon aktiv, nur soviele Leute zum Nachbesetzen schicken wie Verteidiger eingestellt
        if (aggressors.size() > 0 && GAMECLIENT.GetGGS().getSelection(ADDON_DEFENDER_BEHAVIOR) == 2)
        {
            diff = (gwg->GetPlayer(player)->militarySettings_[2] * diff) / MILITARY_SETTINGS_SCALE[2];
        }
        gwg->GetPlayer(player)->OrderTroops(this, diff,true);
	} 
	//now notify the max ranks we no longer wanted (they will pick a new target which may be the same building that is why we cancel them after ordering new ones in the hope to get low ranks instead)
	for (std::vector<nofPassiveSoldier*>::const_iterator it=noNeed.begin(); it!=noNeed.end(); ++it)
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
    // Goldmünze in Bestellliste aufnehmen
    ordered_coins.push_back(ware);
}


void nobMilitary::AddWare(Ware* ware)
{
    // Ein Golstück mehr
    ++coins;
    // aus der Bestellliste raushaun
    ordered_coins.remove(ware);

    // Ware vernichten
    gwg->GetPlayer(player)->RemoveWare(ware);
    delete ware;

    // Evtl. Soldaten befördern
    PrepareUpgrading();
}

void nobMilitary::WareLost(Ware* ware)
{
    // Ein Goldstück konnte nicht kommen --> aus der Bestellliste entfernen
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
    ordered_troops.insert(soldier);
}

void nobMilitary::CancelOrders()
{
    // Soldaten zurückschicken
    for(SortedTroops::iterator it = ordered_troops.begin(); it != ordered_troops.end(); ++it)
        (*it)->NotNeeded();

    ordered_troops.clear();

    // Goldmünzen zurückschicken
    for(std::list<Ware*>::iterator it = ordered_coins.begin(); it != ordered_coins.end(); ++it)
        WareNotNeeded(*it);

    ordered_coins.clear();
}

void nobMilitary::AddActiveSoldier(nofActiveSoldier* soldier)
{


    // aktiver Soldat, eingetroffen werden --> dieser muss erst in einen passiven Soldaten
    // umoperiert werden (neu erzeugt und alter zerstört) werden
    nofPassiveSoldier* passive_soldier = new nofPassiveSoldier(*soldier);

    // neuen Soldaten einhängen
    AddPassiveSoldier(passive_soldier);

    // alten Soldaten später vernichten
    em->AddToKillList(soldier);

    // Soldat ist wie tot, d.h. er muss aus allen Missionslisten etc. wieder rausgenommen werden
    SoldierLost(soldier);
}

void nobMilitary::AddPassiveSoldier(nofPassiveSoldier* soldier)
{
    assert(soldier->GetPlayer() == player);
    assert(troops.size() < unsigned(TROOPS_COUNT[nation][size]));

    troops.insert(soldier);

    // und aus den bestllten Truppen raushauen, da er ja jetzt hier ist
    ordered_troops.erase(soldier);

    // Wurde dieses Gebäude zum ersten Mal besetzt?
    if(new_built)
    {
        if(GAMECLIENT.GetPlayerID() == this->player)
            GAMECLIENT.SendPostMessage(new ImagePostMsgWithLocation(_("Military building occupied"), PMC_MILITARY, pos, this->type_, this->nation));
        // Ist nun besetzt
        new_built = false;
        // Landgrenzen verschieben
        gwg->RecalcTerritory(this, MILITARY_RADIUS[size], false, true);
        // Tür zumachen
        CloseDoor();
        // Fanfarensound abspieln, falls das Militärgebäude im Sichtbereich ist und unseres ist
        gwg->MilitaryBuildingCaptured(pos, player);
        // AIEvent senden an besitzer
        GAMECLIENT.SendAIEvent(new AIEvent::Building(AIEvent::BuildingConquered, pos, type_), player);
    }
    else
    {
        // Evtl. Soldaten befördern
        PrepareUpgrading();
    }

    // Goldmünzen suchen, evtl sinds ja neue Soldaten
    SearchCoins();
}


void nobMilitary::SoldierLost(nofSoldier* soldier)
{
    // Soldat konnte nicht (mehr) kommen --> rauswerfen und ggf. neue Soldaten rufen
    ordered_troops.erase(static_cast<nofPassiveSoldier*>(soldier));
    troops_on_mission.remove(static_cast<nofActiveSoldier*>(soldier));
    RegulateTroops();
}

void nobMilitary::SoldierOnMission(nofPassiveSoldier* passive_soldier, nofActiveSoldier* active_soldier)
{
    // Aus der Besatzungsliste raushauen, aber noch mit merken
    troops.erase(passive_soldier);
    troops_on_mission.push_back(active_soldier);
}

nofPassiveSoldier* nobMilitary::ChooseSoldier()
{
    if(troops.empty())
        return 0;

    nofPassiveSoldier* candidates[5] = {NULL, NULL, NULL, NULL, NULL}; // candidates per rank

    // how many ranks
    unsigned rank_count = 0;

    for(SortedTroops::iterator it = troops.begin(); it != troops.end(); ++it)
    {
        if(!candidates[(*it)->GetRank()])
        {
            ++rank_count;
            candidates[(*it)->GetRank()] = *it;
        }
    }

    // ID ausrechnen
    unsigned rank = ((rank_count - 1) * gwg->GetPlayer(player)->militarySettings_[1]) / MILITARY_SETTINGS_SCALE[1];

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
        // Verteidiger auswählen
        nofPassiveSoldier* soldier = ChooseSoldier();
        // neuen aggressiven Verteidiger daraus erzeugen
        nofAggressiveDefender* ad = new nofAggressiveDefender(soldier, attacker);
        // soll rausgehen
        AddLeavingFigure(ad);
        // auf die Missionsliste setzen
        troops_on_mission.push_back(ad);
        // aus den Truppen rauswerfen
        troops.erase(soldier);
        // alten passiven Soldaten vernichten
        soldier->Destroy();
        delete soldier;

        return ad;
    }
    else
        return 0;
}

/// Gibt die Anzahl der Soldaten zurück, die für einen Angriff auf ein bestimmtes Ziel zur Verfügung stehen
unsigned nobMilitary::GetNumSoldiersForAttack(const MapPoint dest, const unsigned char player_attacker) const
{
    // Soldaten ausrechnen, wie viel man davon nehmen könnte, je nachdem wie viele in den
    // Militäreinstellungen zum Angriff eingestellt wurden
    unsigned short soldiers_count =
        (GetTroopsCount() > 1) ?
        ((GetTroopsCount() - 1) * players->getElement(player_attacker)->militarySettings_[3] / 5) : 0;

    unsigned int distance = gwg->CalcDistance(pos, dest);

    // Falls Entfernung größer als Basisreichweite, Soldaten subtrahieren
    if (distance > BASE_ATTACKING_DISTANCE)
    {
        // je einen soldaten zum entfernen vormerken für jeden EXTENDED_ATTACKING_DISTANCE großen Schritt
        unsigned short soldiers_to_remove = ((distance - BASE_ATTACKING_DISTANCE + EXTENDED_ATTACKING_DISTANCE - 1) / EXTENDED_ATTACKING_DISTANCE);
        if (soldiers_to_remove < soldiers_count)
            soldiers_count -= soldiers_to_remove;
        else
            return 0;
    }

    // und auch der Weg zu Fuß darf dann nicht so weit sein, wenn das alles bestanden ist, können wir ihn nehmen..
    if(soldiers_count && gwg->FindHumanPath(pos, dest, MAX_ATTACKING_RUN_DISTANCE, false, NULL, false) != 0xFF)
        // Soldaten davon nehmen
        return soldiers_count;
    else
        return 0;
}

/// Gibt die Soldaten zurück, die für einen Angriff auf ein bestimmtes Ziel zur Verfügung stehen
std::vector<nofPassiveSoldier*> nobMilitary::GetSoldiersForAttack(const MapPoint dest, const unsigned char player_attacker) const
{
    std::vector<nofPassiveSoldier*> soldiers;
    unsigned soldiers_count = GetNumSoldiersForAttack(dest, player_attacker);
    for(SortedTroops::const_reverse_iterator it = troops.rbegin(); it != troops.rend() && soldiers_count; ++it, --soldiers_count)
    {
        soldiers.push_back(*it);
    }
    return soldiers;
}

/// Gibt die Stärke der Soldaten zurück, die für einen Angriff auf ein bestimmtes Ziel zur Verfügung stehen
unsigned nobMilitary::GetSoldiersStrengthForAttack(const MapPoint dest, const unsigned char player_attacker, unsigned& count) const
{
    unsigned strength = 0;

    unsigned soldiers_count = GetNumSoldiersForAttack(dest, player_attacker);
    count = soldiers_count;

    for(SortedTroops::const_reverse_iterator it = troops.rbegin(); it != troops.rend() && soldiers_count; ++it, --soldiers_count)
    {
        strength += HITPOINTS[nation][(*it)->GetRank()];
    }

    return(strength);
}

/// Gibt die Stärke eines Militärgebäudes zurück
unsigned nobMilitary::GetSoldiersStrength() const
{
    unsigned strength = 0;

    for(SortedTroops::const_iterator it = troops.begin(); it != troops.end(); ++it)
    {
        strength += HITPOINTS[nation][(*it)->GetRank()];
    }

    return(strength);
}

/// is there a max rank soldier in the building?
unsigned nobMilitary::HasMaxRankSoldier() const
{
	unsigned count=0;
    for(SortedTroops::const_reverse_iterator it = troops.rbegin(); it != troops.rend(); ++it)
    {
		if ((*it)->GetRank() >= GAMECLIENT.GetGGS().GetMaxMilitaryRank())
			count++;
    }
	return count;
}

nofDefender* nobMilitary::ProvideDefender(nofAttacker* const attacker)
{
    // Überhaupos Soldaten da?
    if(troops.empty())
    {
        /// Soldaten, die noch auf Mission gehen wollen, canceln und für die Verteidigung mit einziehen
        CancelJobs();
        // Nochmal versuchen
        if(troops.empty())
            return NULL;
    }


    nofPassiveSoldier* soldier = ChooseSoldier();

    // neuen Verteidiger erzeugen
    nofDefender* defender = new nofDefender(soldier, attacker);

    // aus der Liste entfernen
    troops.erase(soldier);

    // und vernichten
    soldier->Destroy();
    delete soldier;

    return defender;
}

void nobMilitary::Capture(const unsigned char new_owner)
{
    captured_not_built = true;

    // Goldmünzen in der Inventur vom alten Spieler abziehen und dem neuen hinzufügen
    gwg->GetPlayer(player)->DecreaseInventoryWare(GD_COINS, coins);
    gwg->GetPlayer(new_owner)->IncreaseInventoryWare(GD_COINS, coins);

    // Soldaten, die auf Mission sind, Bescheid sagen
    for(std::list<nofActiveSoldier*>::iterator it = troops_on_mission.begin(); it != troops_on_mission.end(); ++it)
        (*it)->HomeDestroyed();

    // Bestellungen die hierher unterwegs sind canceln
    CancelOrders();

    // Aggressiv-Verteidigenden Soldaten Bescheid sagen, dass sie nach Hause gehen können
    for(std::list<nofAggressiveDefender*>::iterator it = aggressive_defenders.begin(); it != aggressive_defenders.end(); ++it)
        (*it)->AttackedGoalDestroyed();

    troops_on_mission.clear();
    aggressive_defenders.clear();

    // In der Wirtschaftsverwaltung dieses Gebäude jetzt zum neuen Spieler zählen und beim alten raushauen
    gwg->GetPlayer(player)->RemoveMilitaryBuilding(this);
    gwg->GetPlayer(new_owner)->AddMilitaryBuilding(this);

    // Alten Besitzer merken
    unsigned char old_player = player;

    // neuer Spieler
    player = new_owner;

    // Flagge davor auch übernehmen
    GetFlag()->Capture(new_owner);

    // Territorium neu berechnen
    gwg->RecalcTerritory(this, MILITARY_RADIUS[size], false, false);

    // Sichtbarkeiten berechnen für alten Spieler
    gwg->RecalcVisibilitiesAroundPoint(pos, GetMilitaryRadius() + VISUALRANGE_MILITARY + 1, old_player, NULL);

    // Grenzflagge entsprechend neu setzen von den Feinden
    LookForEnemyBuildings();
    // und von den Verbündeten (da ja ein Feindgebäude weg ist)!
    sortedMilitaryBlds buildings = gwg->LookForMilitaryBuildings(pos, 4);
    for(sortedMilitaryBlds::iterator it = buildings.begin(); it != buildings.end(); ++it)
    {
        // verbündetes Gebäude?
        if(gwg->GetPlayer((*it)->GetPlayer())->IsPlayerAttackable(old_player)
                && (*it)->GetBuildingType() >= BLD_BARRACKS && (*it)->GetBuildingType() <= BLD_FORTRESS)
            // Grenzflaggen von dem neu berechnen
            static_cast<nobMilitary*>(*it)->LookForEnemyBuildings();
    }

    // ehemalige Leute dieses Gebäudes nach Hause schicken, die ggf. grad auf dem Weg rein/raus waren
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

    // Gebäude wird nun eingenommen
    capturing = true;

    // Soldat, der zum Erobern reinläuft, ist nun drinne --> Anzahl der erobernden Soldaten entsprechend verringern
    assert(capturing_soldiers);
    --capturing_soldiers;

    // Fanfarensound abspieln, falls das Militärgebäude im Sichtbereich ist und unseres ist
    gwg->MilitaryBuildingCaptured(pos, player);

    // Post verschicken, an den alten Besitzer und an den neuen Besitzer
    if(GAMECLIENT.GetPlayerID() == old_player)
        GAMECLIENT.SendPostMessage(
            new ImagePostMsgWithLocation(_("Military building lost"), PMC_MILITARY, pos, GetBuildingType(), GetNation()));
    if(GAMECLIENT.GetPlayerID() == this->player)
        GAMECLIENT.SendPostMessage(
            new ImagePostMsgWithLocation(_("Military building captured"), PMC_MILITARY, pos, GetBuildingType(), GetNation()));

    // ggf. Fenster schließen vom alten Spieler
    gwg->ImportantObjectDestroyed(pos);

    // AIEvent senden an gewinner&verlierer
    GAMECLIENT.SendAIEvent(new AIEvent::Building(AIEvent::BuildingConquered, pos, type_), player);
    GAMECLIENT.SendAIEvent(new AIEvent::Building(AIEvent::BuildingLost, pos, type_), old_player);

}

void nobMilitary::NeedOccupyingTroops(const unsigned char new_owner)
{
    // Brauchen wir noch Soldaten (ein Soldat kommt ja noch rein), keine Soldaten von anderen Spielern
    // wählen (z.B. "Kollektivangriffen"), manchmal ist es egal, wer reinkommt (new_owner == 0xFF)

    // Soldaten wählen, der am nächsten an der Flagge steht, damit nicht welche von ganze hinten ewige Zeit vor
    // latschen müssen
    nofAttacker* best_attacker = 0;
    unsigned best_radius = std::numeric_limits<unsigned>::max();

    unsigned needed_soldiers = unsigned(CalcTroopsCount());

    if(needed_soldiers > troops.size() + capturing_soldiers + troops_on_mission.size() + ordered_troops.size())
    {
        // Soldaten absuchen
        for(std::list<nofAttacker*>::iterator it = aggressors.begin(); it != aggressors.end(); ++it)
        {
            // Steht der Soldat überhaupos um das Gebäude rum?
            if((*it)->IsAttackerReady() && ((*it)->GetPlayer() == new_owner || new_owner == 0xFF))
            {
                // Näher als der bisher beste?
                if((*it)->GetRadius() < best_radius)
                {
                    // Und kommt er überhaupos zur Flagge (könnte ja in der 2. Reihe stehen, sodass die
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
            // Nicht gerade Soldaten löschen, die das Gebäude noch einnehmen!
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
        // keine Soldaten mehr benötigt, der Rest kann wieder nach Hause gehen
		//LOG.lprintf("building full: remaining attackers can go home (target was: %i,%i) \n ",x,y);t;
        for(std::list<nofAttacker*>::iterator it = aggressors.begin(); it != aggressors.end();)
        {
            nofAttacker* attacker = *it;
            // Nicht gerade Soldaten löschen, die das Gebäude noch einnehmen!
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

        // Nun die Besetzung prüfen
        RegulateTroops();
    }
}

void nobMilitary::ToggleCoins()
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
    // Will ich überhaupos Goldmünzen, wenn nich, sofort raus
    if(!WantCoins())
        return 0;

    // 10000 als Basis wählen, damit man auch noch was abziehen kann
    int points = 10000;

    // Wenn hier schon Münzen drin sind oder welche bestellt sind, wirkt sich das natürlich negativ auf die "Wichtigkeit" aus
    points -= (coins + ordered_coins.size()) * 30;

    // Beförderbare Soldaten zählen
    for(SortedTroops::iterator it = troops.begin(); it != troops.end(); ++it)
    {
        // Solange es kein Max Rank ist, kann der Soldat noch befördert werden
        if((*it)->GetRank() < GAMECLIENT.GetGGS().GetMaxMilitaryRank())
            points += 20;
    }

    if(points < 0)
        throw std::logic_error("Negative points are not allowed");

    return static_cast<unsigned>(points);
}

bool nobMilitary::WantCoins()
{
    // Wenn die Goldzufuhr gestoppos wurde oder Münzvorrat voll ist, will ich gar keine Goldmünzen
    return (!disable_coins && coins + ordered_coins.size() != GOLD_COUNT[nation][size] && !new_built);
}

void nobMilitary::SearchCoins()
{
    // Brauche ich überhaupos Goldmünzen bzw. hab ich vielleicht schon ein Event angemeldet?
    if(WantCoins() && !goldorder_event)
    {
        // Lagerhaus mit Goldmünzen suchen
        FW::Param_Ware p = {GD_COINS, 1};
        if(nobBaseWarehouse* wh = gwg->GetPlayer(player)->FindWarehouse(this, FW::Condition_Ware, 0, false, &p, false))
        {
            // Wenns eins gibt, dort eine Goldmünze bestellen
            Ware* ware = wh->OrderWare(GD_COINS, this);

            if(!ware)
            {
                // Ware dürfte nicht 0 werden, da ja ein Lagerhaus MIT GOLDMÜNZEN bereits gesucht wird
                LOG.lprintf("nobMilitary::SearchCoins: WARNING: ware = 0. Bug alarm!\n");
                return;
            }

            // Goldmünze zu den Bestellungen hinzufügen
            ordered_coins.push_back(ware);

            // Nach einer Weile nochmal nach evtl neuen Goldmünzen gucken
            goldorder_event = em->AddEvent(this, 200 + RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 400), 1);
        }
    }
}

void nobMilitary::PrepareUpgrading()
{
    // Goldmünzen da?
    if(!coins)
        return;

    // Gibts auch noch kein Beförderungsevent?
    if(upgrade_event)
        return;

    // Noch Soldaten, die befördert werden können?
    bool soldiers_available = false;

    for(SortedTroops::iterator it = troops.begin(); it != troops.end(); ++it)
    {
        if((*it)->GetRank() < GAMECLIENT.GetGGS().GetMaxMilitaryRank())
        {
            // es wurde ein Soldat gefunden, der befördert werden kann
            soldiers_available = true;
            break;
        }
    }

    if(!soldiers_available)
        return;

    // Alles da --> Beförderungsevent anmelden
    upgrade_event = em->AddEvent(this, UPGRADE_TIME + RANDOM.Rand(__FILE__, __LINE__, GetObjId(), UPGRADE_TIME_RANDOM), 2);
}

void nobMilitary::HitOfCatapultStone()
{
    // Ein Soldat weniger, falls es noch welche gibt
    if(!troops.empty())
    {
        (*troops.begin())->Die();
        helpers::pop_front(troops);
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
 *  Darf das Militärgebäude abgerissen werden (Abriss-Verbot berücksichtigen)?
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
            // Prüfen, ob das Gebäude angegriffen wird
            if(!aggressors.empty())
                return false;
        } break;
        case 2: // near frontiers
        {
            // Prüfen, ob es in Grenznähe steht
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


