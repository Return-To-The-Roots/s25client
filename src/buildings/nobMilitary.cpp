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


#include "defines.h" // IWYU pragma: keep
#include "nobMilitary.h"
#include "GameClient.h"
#include "GameClientPlayer.h"
#include "gameData/MilitaryConsts.h"
#include "Ware.h"
#include "figures/nofPassiveSoldier.h"
#include "figures/nofDefender.h"
#include "figures/nofAggressiveDefender.h"
#include "figures/nofAttacker.h"
#include "Loader.h"
#include "EventManager.h"
#include "Random.h"
#include "buildings/nobBaseWarehouse.h"
#include "FindWhConditions.h"
#include "PostMsg.h"
#include "ai/AIEvents.h"
#include "nodeObjs/noFlag.h"
#include "gameData/GameConsts.h"
#include "SerializedGameData.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "Point.h"
#include "Log.h"
#include "helpers/containerUtils.h"

#include <limits>
#include <stdexcept>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep
class RoadSegment;

nobMilitary::nobMilitary(const BuildingType type, const MapPoint pos, const unsigned char player, const Nation nation)
    : nobBaseMilitary(type, pos, player, nation), new_built(true), coins(0), coinsDisabled(false),
      coinsDisabledVirtual(false), capturing(false), capturing_soldiers(0), goldorder_event(0), upgrade_event(0), is_regulating_troops(false), captured_not_built(false)
{
    // Gebäude entsprechend als Militärgebäude registrieren und in ein Militärquadrat eintragen
    gwg->GetPlayer(player).AddMilitaryBuilding(this);
    gwg->GetMilitarySquares().Add(this);

    // Größe ermitteln
    switch(type)
    {
        case BLD_BARRACKS: size = 0; break;
        case BLD_GUARDHOUSE: size = 1; break;
        case BLD_WATCHTOWER: size = 2; break;
        case BLD_FORTRESS: size = 3; break;
        default: RTTR_Assert(false); size = 0xFF; break;
    }

    LookForEnemyBuildings();

    // Tür aufmachen, bis Gebäude besetzt ist
    OpenDoor();

    // Wenn kein Gold in neu gebaute Militärgebäude eingeliefert werden soll, wird die Goldzufuhr gestoppt
    // Ansonsten neue Goldmünzen anfordern
    if(GAMECLIENT.GetGGS().isEnabled(AddonId::NO_COINS_DEFAULT))
    {
        coinsDisabled = true;
        coinsDisabledVirtual = true;
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
    // Remove from military square and buildings first, to avoid e.g. sending canceled soldiers back to this building
    gwg->GetPlayer(player).RemoveMilitaryBuilding(this);
    gwg->GetMilitarySquares().Remove(this);

    // Bestellungen stornieren
    CancelOrders();

    // Soldaten rausschicken
    for(SortedTroops::iterator it = troops.begin(); it != troops.end(); ++it)
        (*it)->InBuildingDestroyed();
    troops.clear();

    // Inform far-away capturers
    for(std::list<nofAttacker*>::iterator it = far_away_capturers.begin(); it != far_away_capturers.end(); ++it)
        (*it)->AttackedGoalDestroyed();
    far_away_capturers.clear();

    // Events ggf. entfernen
    em->RemoveEvent(goldorder_event);
    em->RemoveEvent(upgrade_event);

    // übriggebliebene Goldmünzen in der Inventur abmelden
    gwg->GetPlayer(player).DecreaseInventoryWare(GD_COINS, coins);

    Destroy_nobBaseMilitary();

    // Land drumherum neu berechnen (nur wenn es schon besetzt wurde!)
    // Nach dem BaseDestroy erst, da in diesem erst das Feuer gesetzt, die Straße gelöscht wird usw.
    if(!new_built)
        gwg->RecalcTerritory(*this, true, false);

    GAMECLIENT.SendAIEvent(new AIEvent::Building(AIEvent::BuildingLost, pos, type_), player);
}

void nobMilitary::Serialize_nobMilitary(SerializedGameData& sgd) const
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

    sgd.PushUnsignedChar(bitfield);

    sgd.PushUnsignedChar(coins);
    sgd.PushBool(coinsDisabled);
    sgd.PushBool(coinsDisabledVirtual);
    sgd.PushUnsignedChar(frontier_distance);
    sgd.PushUnsignedChar(size);
    sgd.PushBool(capturing);
    sgd.PushUnsignedInt(capturing_soldiers);
    sgd.PushObject(goldorder_event, true);
    sgd.PushObject(upgrade_event, true);

    sgd.PushObjectContainer(ordered_troops, true);
    sgd.PushObjectContainer(ordered_coins, true);
    sgd.PushObjectContainer(troops, true);
    sgd.PushObjectContainer(far_away_capturers, true);
}

nobMilitary::nobMilitary(SerializedGameData& sgd, const unsigned obj_id) : nobBaseMilitary(sgd, obj_id),
    is_regulating_troops(false)
{
    // use a bitfield instead of 1 unsigned char per boolean
    // mainly for compatibility :-)

    unsigned char bitfield = sgd.PopUnsignedChar();

    new_built = bitfield & (1 << 0);
    captured_not_built = !(bitfield & (1 << 1));

    coins = sgd.PopUnsignedChar();
    coinsDisabled = sgd.PopBool();
    coinsDisabledVirtual = sgd.PopBool();
    frontier_distance = sgd.PopUnsignedChar();
    size = sgd.PopUnsignedChar();
    capturing = sgd.PopBool();
    capturing_soldiers = sgd.PopUnsignedInt();
    goldorder_event = sgd.PopObject<EventManager::Event>(GOT_EVENT);
    upgrade_event = sgd.PopObject<EventManager::Event>(GOT_EVENT);


    sgd.PopObjectContainer(ordered_troops, GOT_NOF_PASSIVESOLDIER);
    sgd.PopObjectContainer(ordered_coins, GOT_WARE);
    sgd.PopObjectContainer(troops, GOT_NOF_PASSIVESOLDIER);
    sgd.PopObjectContainer(far_away_capturers, GOT_NOF_ATTACKER);

    // ins Militärquadrat einfügen
    gwg->GetMilitarySquares().Add(this);

    if(capturing && capturing_soldiers == 0 && aggressors.empty())
    {
        LOG.lprintf("Bug in savegame detected: Building at (%d,%d) beeing captured has no capturers. Trying to fix this...\n", pos.x, pos.y);
        capturing = false;
    }
}


void nobMilitary::Draw(int x, int y)
{
    // Gebäude an sich zeichnen
    DrawBaseBuilding(x, y);

    // (max 4) Besatzungs-Fähnchen zeichnen
    unsigned flags = min<unsigned>(troops.size() + this->leave_house.size(), 4);

    for(unsigned i = 0; i < flags; ++i)
        LOADER.GetMapPlayerImage(3162 + GAMECLIENT.GetGlobalAnimation(8, 2, 1, pos.x * pos.y * i))->Draw(x + TROOPS_FLAGS[nation][size][0], y + TROOPS_FLAGS[nation][size][1] + (i) * 3, 0, 0, 0, 0, 0, 0, COLOR_WHITE, gwg->GetPlayer(player).color);

    // Die Fahne, die anzeigt wie weit das Gebäude von der Grenze entfernt ist, zeichnen
    unsigned frontier_distance_tmp = frontier_distance;
    glArchivItem_Bitmap_Player* bitmap = NULL;
    unsigned int animationFrame = GAMECLIENT.GetGlobalAnimation(4, 1, 1, pos.x * pos.y * GetObjId());
    if(frontier_distance_tmp == 2)
    {
        // todo Hafenflagge
        bitmap = LOADER.GetPlayerImage("map_new", 3150 + animationFrame);
    }
    else
    {
        if(frontier_distance_tmp == 3) frontier_distance_tmp = 2;
        bitmap = LOADER.GetMapPlayerImage(3150 + frontier_distance_tmp * 4 + animationFrame);
    }
    if(bitmap)
        bitmap->Draw(x + BORDER_FLAGS[nation][size][0], y + BORDER_FLAGS[nation][size][1], 0, 0, 0, 0, 0, 0);

    // Wenn Goldzufuhr gestoppt ist, Schild außen am Gebäude zeichnen zeichnen
    if(coinsDisabledVirtual)
        LOADER.GetMapImageN(46)->Draw(x + BUILDING_SIGN_CONSTS[nation][type_].x, y + BUILDING_SIGN_CONSTS[nation][type_].y, 0, 0, 0, 0, 0, 0);
}

void nobMilitary::HandleEvent(const unsigned int id)
{
    switch(id)
    {
            // "Rausgeh-Event"
        case 0:
        {
            leaving_event = NULL;

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

            RegulateTroops();

        } break;
        // Goldbestell-Event
        case 1:
        {
            goldorder_event = NULL;

            // ggf. nach neuen Goldmünzen suchen
            SearchCoins();
        } break;
        // Beförderungs-Event
        case 2:
        {
            upgrade_event = NULL;

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
                gwg->GetPlayer(player).DecreaseInventoryWare(GD_COINS, 1);

                // Evtl neues Beförderungsevent anmelden
                PrepareUpgrading();

                // Ggf. neue Goldmünzen bestellen
                SearchCoins();
            }

        } break;
    }
}

unsigned int nobMilitary::GetMilitaryRadius() const
{
    return MILITARY_RADIUS[size];
}

void nobMilitary::LookForEnemyBuildings(const nobBaseMilitary* const exception)
{
    // Umgebung nach Militärgebäuden absuchen
    sortedMilitaryBlds buildings = gwg->LookForMilitaryBuildings(pos, 3);
    frontier_distance = 0;

    for(sortedMilitaryBlds::iterator it = buildings.begin(); it != buildings.end(); ++it)
    {
        // feindliches Militärgebäude?
        if(*it != exception && (*it)->GetPlayer() != player && gwg->GetPlayer((*it)->GetPlayer()).IsPlayerAttackable(player))
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
    RTTR_Assert(helpers::contains(gwg->GetPlayer(player).GetMilitaryBuildings(), this)); // If this fails, the building is beeing destroyed!

    // Wenn das Gebäude eingenommen wird, erstmal keine neuen Truppen und warten, wieviele noch reinkommen
    if(IsCaptured())
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
        GameClientPlayer& owner = gwg->GetPlayer(player);
        if (owner.militarySettings_[1] > MILITARY_SETTINGS_SCALE[1] / 2)
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
        if(owner.FindWarehouse(*this, FW::NoCondition(), true, false))
        {
            // Dann den Rest (einer muss immer noch drinbleiben!)
            // erst die schwachen Soldaten raus
            if (owner.militarySettings_[1] > MILITARY_SETTINGS_SCALE[1] / 2)
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
        if (IsUnderAttack() && GAMECLIENT.GetGGS().getSelection(AddonId::DEFENDER_BEHAVIOR) == 2)
        {
            diff = (gwg->GetPlayer(player).militarySettings_[2] * diff) / MILITARY_SETTINGS_SCALE[2];
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
			gwg->GetPlayer(player).OrderTroops(this, diff);
    }

    is_regulating_troops = false;
}

int nobMilitary::CalcTroopsCount()
{
    return (TROOPS_COUNT[nation][size] - 1) * gwg->GetPlayer(player).militarySettings_[4 + frontier_distance] / MILITARY_SETTINGS_SCALE[4 + frontier_distance] + 1;
}

void nobMilitary::SendSoldiersHome()
{
	int diff = 1 - static_cast<int>(GetTotalSoldiers());
    if(diff < 0) //poc: this should only be >0 if we are being captured. capturing should be true until its the last soldier and this last one would count twice here and result in a returning soldier that shouldnt return.
	{
		// Nur rausschicken, wenn es einen Weg zu einem Lagerhaus gibt!
        if(!gwg->GetPlayer(player).FindWarehouse(*this, FW::NoCondition(), true, false))
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
        if (IsUnderAttack() && GAMECLIENT.GetGGS().getSelection(AddonId::DEFENDER_BEHAVIOR) == 2)
        {
            diff = (gwg->GetPlayer(player).militarySettings_[2] * diff) / MILITARY_SETTINGS_SCALE[2];
        }
        gwg->GetPlayer(player).OrderTroops(this, diff,true);
	} 
	//now notify the max ranks we no longer wanted (they will pick a new target which may be the same building that is why we cancel them after ordering new ones in the hope to get low ranks instead)
	for (std::vector<nofPassiveSoldier*>::const_iterator it=noNeed.begin(); it!=noNeed.end(); ++it)
		(*it)->NotNeeded();
}

bool nobMilitary::IsUseless() const
{
    if(frontier_distance || new_built)
        return false;
    return !gwg->DoesTerritoryChange(*this, true, false);
}

void nobMilitary::TakeWare(Ware* ware)
{
    // Goldmünze in Bestellliste aufnehmen
    RTTR_Assert(!helpers::contains(ordered_coins, ware));
    ordered_coins.push_back(ware);
}


void nobMilitary::AddWare(Ware*& ware)
{
    // Ein Golstück mehr
    ++coins;
    // aus der Bestellliste raushaun
    RTTR_Assert(helpers::contains(ordered_coins, ware));
    ordered_coins.remove(ware);

    // Ware vernichten
    gwg->GetPlayer(player).RemoveWare(ware);
    deletePtr(ware);

    // Evtl. Soldaten befördern
    PrepareUpgrading();
}

void nobMilitary::WareLost(Ware* ware)
{
    // Ein Goldstück konnte nicht kommen --> aus der Bestellliste entfernen
    RTTR_Assert(helpers::contains(ordered_coins, ware));
    ordered_coins.remove(ware);
}

bool nobMilitary::FreePlaceAtFlag()
{
    return false;
}
void nobMilitary::GotWorker(Job  /*job*/, noFigure* worker)
{
    // Insert soldiers sorted. Weak ones first
    RTTR_Assert(dynamic_cast<nofPassiveSoldier*>(worker));
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
    soldier->ResetHome();
    em->AddToKillList(soldier);

    RTTR_Assert(soldier->GetPlayer() == player);

    // Returned home
    if(soldier == defender_)
        NoDefender();
    else if(helpers::contains(troops_on_mission, soldier))
    {
        troops_on_mission.remove(soldier);
    }else if(IsCaptured() || IsFarAwayCapturer(dynamic_cast<nofAttacker*>(soldier)))
    {
        RTTR_Assert(dynamic_cast<nofAttacker*>(soldier));
        return;
    }
    // Do only if not capturing
    RegulateTroops();
}

void nobMilitary::AddPassiveSoldier(nofPassiveSoldier* soldier)
{
    RTTR_Assert(soldier->GetPlayer() == player);
    RTTR_Assert(troops.size() < unsigned(TROOPS_COUNT[nation][size]));

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
        gwg->RecalcTerritory(*this, false, true);
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
    if(soldier->GetGOT() == GOT_NOF_PASSIVESOLDIER)
    {
        RTTR_Assert(helpers::contains(ordered_troops, static_cast<nofPassiveSoldier*>(soldier)));
        ordered_troops.erase(static_cast<nofPassiveSoldier*>(soldier));
    }else
    {
        nofActiveSoldier* actSoldier = dynamic_cast<nofActiveSoldier*>(soldier);
        RTTR_Assert(actSoldier);
        RTTR_Assert(helpers::contains(troops_on_mission, actSoldier));
        troops_on_mission.remove(actSoldier);
    }
    RegulateTroops();
}

void nobMilitary::SoldierOnMission(nofPassiveSoldier* passive_soldier, nofActiveSoldier* active_soldier)
{
    // Aus der Besatzungsliste raushauen, aber noch mit merken
    troops.erase(passive_soldier);
    passive_soldier->LeftBuilding();
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
    unsigned rank = ((rank_count - 1) * gwg->GetPlayer(player).militarySettings_[1]) / MILITARY_SETTINGS_SCALE[1];

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
        nofAggressiveDefender* defender = new nofAggressiveDefender(soldier, attacker);
        // soll rausgehen
        AddLeavingFigure(defender);
        SoldierOnMission(soldier, defender);
        // alten passiven Soldaten vernichten
        soldier->Destroy();
        delete soldier;

        return defender;
    }
    else
        return NULL;
}

/// Gibt die Anzahl der Soldaten zurück, die für einen Angriff auf ein bestimmtes Ziel zur Verfügung stehen
unsigned nobMilitary::GetNumSoldiersForAttack(const MapPoint dest, const unsigned char player_attacker) const
{
    // Soldaten ausrechnen, wie viel man davon nehmen könnte, je nachdem wie viele in den
    // Militäreinstellungen zum Angriff eingestellt wurden
    unsigned short soldiers_count =
        (GetTroopsCount() > 1) ?
        ((GetTroopsCount() - 1) * gwg->GetPlayer(player_attacker).militarySettings_[3] / 5) : 0;

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
    soldier->LeftBuilding();

    // und vernichten
    soldier->Destroy();
    delete soldier;

    return defender;
}

void nobMilitary::Capture(const unsigned char new_owner)
{
    RTTR_Assert(IsCaptured());

    captured_not_built = true;

    // Goldmünzen in der Inventur vom alten Spieler abziehen und dem neuen hinzufügen
    gwg->GetPlayer(player).DecreaseInventoryWare(GD_COINS, coins);
    gwg->GetPlayer(new_owner).IncreaseInventoryWare(GD_COINS, coins);

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
    gwg->GetPlayer(player).RemoveMilitaryBuilding(this);
    gwg->GetPlayer(new_owner).AddMilitaryBuilding(this);

    // Alten Besitzer merken
    unsigned char old_player = player;

    // neuer Spieler
    player = new_owner;

    // Flagge davor auch übernehmen
    GetFlag()->Capture(new_owner);

    // Territorium neu berechnen
    gwg->RecalcTerritory(*this, false, false);

    // Sichtbarkeiten berechnen für alten Spieler
    gwg->RecalcVisibilitiesAroundPoint(pos, GetMilitaryRadius() + VISUALRANGE_MILITARY + 1, old_player, NULL);

    // Grenzflagge entsprechend neu setzen von den Feinden
    LookForEnemyBuildings();
    // und von den Verbündeten (da ja ein Feindgebäude weg ist)!
    sortedMilitaryBlds buildings = gwg->LookForMilitaryBuildings(pos, 4);
    for(sortedMilitaryBlds::iterator it = buildings.begin(); it != buildings.end(); ++it)
    {
        // verbündetes Gebäude?
        if(gwg->GetPlayer((*it)->GetPlayer()).IsPlayerAttackable(old_player)
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

    // Send all allied aggressors home (we own the building now!)
    for(std::list<nofAttacker*>::iterator it = aggressors.begin(); it != aggressors.end();)
    {
        nofAttacker* attacker = *it;
        // dont remove attackers owned by players not allied with the new owner!
        unsigned char attPlayer = attacker->GetPlayer();
        if(attPlayer != player && !gwg->GetPlayer(attPlayer).IsPlayerAttackable(player))
        {
            it = aggressors.erase(it);
            attacker->CapturedBuildingFull();
        }else
            ++it;
    }

    // Fanfarensound abspieln, falls das Militärgebäude im Sichtbereich ist und unseres ist
    gwg->MilitaryBuildingCaptured(pos, player);

    // Post verschicken, an den alten Besitzer und an den neuen Besitzer
    if(GAMECLIENT.GetPlayerID() == old_player)
        GAMECLIENT.SendPostMessage(new ImagePostMsgWithLocation(_("Military building lost"), PMC_MILITARY, pos, GetBuildingType(), GetNation()));
    if(GAMECLIENT.GetPlayerID() == this->player)
        GAMECLIENT.SendPostMessage(new ImagePostMsgWithLocation(_("Military building captured"), PMC_MILITARY, pos, GetBuildingType(), GetNation()));

    // ggf. Fenster schließen vom alten Spieler
    gwg->ImportantObjectDestroyed(pos);

    // AIEvent senden an gewinner&verlierer
    GAMECLIENT.SendAIEvent(new AIEvent::Building(AIEvent::BuildingConquered, pos, type_), player);
    GAMECLIENT.SendAIEvent(new AIEvent::Building(AIEvent::BuildingLost, pos, type_), old_player);
}

void nobMilitary::NeedOccupyingTroops()
{
    RTTR_Assert(IsCaptured()); // Only valid during capturing
    // Check if we need more soldiers from the attacking soldiers
    // Choose the closest ones first to avoid having them walk a long way

    nofAttacker* best_attacker = NULL;
    unsigned best_radius = std::numeric_limits<unsigned>::max();

    unsigned needed_soldiers = unsigned(CalcTroopsCount());
    unsigned currentSoldiers = troops.size() + capturing_soldiers + troops_on_mission.size();

    if(needed_soldiers > currentSoldiers)
    {
        // Soldaten absuchen
        for(std::list<nofAttacker*>::iterator it = aggressors.begin(); it != aggressors.end(); ++it)
        {
            // Is the soldier standing around and owned by the player?
            if(!(*it)->IsAttackerReady() || (*it)->GetPlayer() != player)
                continue;
            // Näher als der bisher beste?
            if((*it)->GetRadius() >= best_radius)
                continue;
            // Und kommt er überhaupt zur Flagge (könnte ja in der 2. Reihe stehen, sodass die vor ihm ihn den Weg versperren)?
            if(gwg->FindHumanPath((*it)->GetPos(), gwg->GetNeighbour(pos, 4), 10, false) != 0xFF)
            {
                // Dann is das der bisher beste
                best_attacker = *it;
                best_radius = best_attacker->GetRadius();
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

        // If necessary look for further soldiers who are not standing around the building
        for(std::list<nofAttacker*>::iterator it = aggressors.begin(); it != aggressors.end() && needed_soldiers > currentSoldiers + far_away_capturers.size(); )
        {
            nofAttacker* attacker = *it;

            if(attacker->GetPlayer() == player)
            {
                // Ask attacker if this is possible
                if(attacker->TryToStartFarAwayCapturing(this))
                {
                    it = aggressors.erase(it);
                    far_away_capturers.push_back(attacker);
                    continue;
                }
            }
            ++it;
        }
    }

    // At this point agressors contains only soldiers, that cannot capture the building (from other player or without a path to flag),
    // the one(s) that is currently walking to capture the building and possibly some more from other (non-allied) players
    // So send those home, who cannot capture the building
    for(std::list<nofAttacker*>::iterator it = aggressors.begin(); it != aggressors.end();)
    {
        nofAttacker* attacker = *it;
        // Nicht gerade Soldaten löschen, die das Gebäude noch einnehmen!
		//also: dont remove attackers owned by players not allied with the new owner!
		if(attacker->GetState() != nofActiveSoldier::STATE_ATTACKING_CAPTURINGNEXT && !gwg->GetPlayer(attacker->GetPlayer()).IsPlayerAttackable(player))
        {
			it = aggressors.erase(it);
            attacker->CapturedBuildingFull();
        }else
            ++it;
    }
}

void nobMilitary::SetCoinsAllowed(const bool enabled)
{
    if(coinsDisabled == !enabled)
        return;

    // Umstellen
    coinsDisabled = !enabled;
    // Wenn das von einem fremden Spieler umgestellt wurde (oder vom Replay), muss auch das visuelle umgestellt werden
    if(GAMECLIENT.GetPlayerID() != player || GAMECLIENT.IsReplayModeOn())
        coinsDisabledVirtual = coinsDisabled;

    if(!coinsDisabled)
        SearchCoins(); // Order coins if we just enabled it
    else
    {
        // send coins back if just deactivated
        for(std::list<Ware*>::iterator it = ordered_coins.begin(); it != ordered_coins.end();)
        {
            // But only those, that are not just beeing carried in
            if((*it)->GetLocation() != this)
            {
                WareNotNeeded(*it);
                it = ordered_coins.erase(it);
            }else
                ++it;
        }
    }
}


unsigned nobMilitary::CalcCoinsPoints()
{
    // Will ich überhaupt Goldmünzen, wenn nich, sofort raus
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
    // Wenn die Goldzufuhr gestoppt wurde oder Münzvorrat voll ist, will ich gar keine Goldmünzen
    return (!coinsDisabled && coins + ordered_coins.size() != GOLD_COUNT[nation][size] && !new_built);
}

void nobMilitary::SearchCoins()
{
    // Brauche ich überhaupt Goldmünzen bzw. hab ich vielleicht schon ein Event angemeldet?
    if(WantCoins() && !goldorder_event)
    {
        // Lagerhaus mit Goldmünzen suchen
        nobBaseWarehouse* wh = gwg->GetPlayer(player).FindWarehouse(*this, FW::HasMinWares(GD_COINS), false, false);
        if(wh)
        {
            // Wenns eins gibt, dort eine Goldmünze bestellen
            Ware* ware = wh->OrderWare(GD_COINS, this);

            if(!ware)
            {
                RTTR_Assert(false);
                // Ware dürfte nicht 0 werden, da ja ein Lagerhaus MIT GOLDMÜNZEN bereits gesucht wird
                LOG.lprintf("nobMilitary::SearchCoins: WARNING: ware = NULL. Bug alarm!\n");
                return;
            }

            RTTR_Assert(helpers::contains(ordered_coins, ware));

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
        nofPassiveSoldier* soldier = *troops.begin();
        helpers::pop_front(troops);
        // Shortcut for Die(): No need to remove from world as it is inside and we can delete it right away
        soldier->RemoveFromInventory();
        soldier->LeftBuilding();
        soldier->Destroy();
        deletePtr(soldier);
    }

    // If there are troops left, order some more, else this will be destroyed
    if(!troops.empty())
        RegulateTroops();

    // Post verschicken
    if(GAMECLIENT.GetPlayerID() == this->player)
        GAMECLIENT.SendPostMessage(new ImagePostMsgWithLocation(_("A catapult is firing upon us!"), PMC_MILITARY, pos, GetBuildingType(), GetNation()));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Darf das Militärgebäude abgerissen werden (Abriss-Verbot berücksichtigen)?
 *
 *  @author OLiver
 */
bool nobMilitary::IsDemolitionAllowed() const
{
    switch(GAMECLIENT.GetGGS().getSelection(AddonId::DEMOLITION_PROHIBITION))
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
    RTTR_Assert(IsAggressor(soldier) || IsFarAwayCapturer(soldier));
    aggressors.remove(soldier);
    far_away_capturers.remove(soldier);

    if (aggressors.empty())
        RegulateTroops();
}

void nobMilitary::CapturingSoldierArrived()
{
    RTTR_Assert(IsCaptured());
    RTTR_Assert(capturing_soldiers > 0);
    --capturing_soldiers;
    if(capturing_soldiers == 0)
    {
        // Search again
        NeedOccupyingTroops();
        if(capturing_soldiers > 0)
            return; // Found more
        // Einnahme beendet
        capturing = false;
        // Nun die Besetzung prüfen
        RegulateTroops();
    }
}

/// A far-away capturer arrived at the building/flag and starts the capturing
void nobMilitary::FarAwayCapturerReachedGoal(nofAttacker* attacker)
{
    RTTR_Assert(IsFarAwayCapturer(attacker));
    if(IsCaptured())
    {
        // If we are still capturing just re-add this soldier to the aggressors
        // one of the currently capturing soldiers will notify him
        far_away_capturers.remove(attacker);
        aggressors.push_back(attacker);
    }else
    {
        // Otherwise we are in a kind of "normal" working state of the building and will just add him when he gets in
        // Call the next one
        CallNextFarAwayCapturer(attacker);
    }
}

void nobMilitary::CallNextFarAwayCapturer(nofAttacker* attacker)
{
    const MapPoint flagPos = GetFlag()->GetPos();
    unsigned minLength = std::numeric_limits<unsigned>::max();
    nofAttacker* bestAttacker = NULL;
    for(std::list<nofAttacker*>::iterator it = far_away_capturers.begin(); it != far_away_capturers.end(); ++it)
    {
        // Skip us and possible capturers at the building
        if(*it == attacker || (*it)->GetPos() == pos)
            continue;
        if(!(*it)->IsAttackerReady())
            continue;
        RTTR_Assert((*it)->GetPos() != flagPos); // Impossible. This should be the current attacker
        unsigned length;
        if(gwg->FindHumanPath((*it)->GetPos(), flagPos, MAX_FAR_AWAY_CAPTURING_DISTANCE, false, &length) == INVALID_DIR)
            continue;
        if(length < minLength)
        {
            minLength = length;
            bestAttacker = *it;
        }
    }
    if(bestAttacker)
        bestAttacker->AttackFlag();
}

