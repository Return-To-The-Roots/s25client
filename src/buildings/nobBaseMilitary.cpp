// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU pragma: keep
#include "nobBaseMilitary.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "SerializedGameData.h"
#include "figures/nofAggressiveDefender.h"
#include "figures/nofAttacker.h"
#include "figures/nofDefender.h"
#include "helpers/containerUtils.h"
#include "nobMilitary.h"
#include "random/Random.h"
#include "world/GameWorldGame.h"
#include "gameData/BuildingProperties.h"
#include "gameData/GameConsts.h"
#include <limits>

nobBaseMilitary::nobBaseMilitary(const BuildingType type, const MapPoint pos, const unsigned char player, const Nation nation)
    : noBuilding(type, pos, player, nation), leaving_event(0), go_out(false), defender_(0)
{}

nobBaseMilitary::~nobBaseMilitary()
{
    for(std::list<noFigure*>::iterator it = leave_house.begin(); it != leave_house.end(); ++it)
        delete *it;
}

void nobBaseMilitary::DestroyBuilding()
{
    // Soldaten Bescheid sagen, die evtl auf Mission sind
    // ATTENTION: iterators can be deleted in HomeDestroyed, -> copy first
    std::vector<nofActiveSoldier*> tmpTroopsOnMission(troops_on_mission.begin(), troops_on_mission.end());
    for(std::vector<nofActiveSoldier*>::iterator it = tmpTroopsOnMission.begin(); it != tmpTroopsOnMission.end(); ++it)
        (*it)->HomeDestroyed();
    troops_on_mission.clear();

    // Und die, die das Gebäude evtl gerade angreifen
    // ATTENTION: iterators can be deleted in AttackedGoalDestroyed, -> copy first
    std::vector<nofAttacker*> tmpAggressors(aggressors.begin(), aggressors.end());
    for(std::vector<nofAttacker*>::iterator it = tmpAggressors.begin(); it != tmpAggressors.end(); ++it)
        (*it)->AttackedGoalDestroyed();
    aggressors.clear();

    // Aggressiv-Verteidigenden Soldaten Bescheid sagen, dass sie nach Hause gehen können
    std::vector<nofAggressiveDefender*> tmpDefenders(aggressive_defenders.begin(), aggressive_defenders.end());
    for(std::vector<nofAggressiveDefender*>::iterator it = tmpDefenders.begin(); it != tmpDefenders.end(); ++it)
        (*it)->AttackedGoalDestroyed();
    aggressive_defenders.clear();

    // Verteidiger Bescheid sagen
    if(defender_)
    {
        defender_->HomeDestroyed();
        defender_ = NULL;
    }

    // Warteschlangenevent vernichten
    GetEvMgr().RemoveEvent(leaving_event);

    // Soldaten, die noch in der Warteschlange hängen, rausschicken
    for(std::list<noFigure*>::iterator it = leave_house.begin(); it != leave_house.end(); ++it)
    {
        gwg->AddFigure(pos, (*it));

        if((*it)->DoJobWorks() && dynamic_cast<nofActiveSoldier*>(*it))
            // Wenn er Job-Arbeiten verrichtet, ists ein ActiveSoldier oder TradeDonkey --> dem Soldat muss extra noch Bescheid gesagt
            // werden!
            static_cast<nofActiveSoldier*>(*it)->HomeDestroyedAtBegin();
        else
        {
            (*it)->Abrogate();
            (*it)->StartWandering();
            (*it)->StartWalking(Direction(RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 6)));
        }
    }

    leave_house.clear();

    // Umgebung nach feindlichen Militärgebäuden absuchen und die ihre Grenzflaggen neu berechnen lassen
    // da, wir ja nicht mehr existieren
    sortedMilitaryBlds buildings = gwg->LookForMilitaryBuildings(pos, Direction::SOUTHEAST);
    for(sortedMilitaryBlds::iterator it = buildings.begin(); it != buildings.end(); ++it)
    {
        if((*it)->GetPlayer() != player && BuildingProperties::IsMilitary((*it)->GetBuildingType()))
            static_cast<nobMilitary*>(*it)->LookForEnemyBuildings(this);
    }
}

void nobBaseMilitary::Serialize_nobBaseMilitary(SerializedGameData& sgd) const
{
    Serialize_noBuilding(sgd);

    sgd.PushObjectContainer(leave_house, false);
    sgd.PushObject(leaving_event, true);
    sgd.PushBool(go_out);
    sgd.PushUnsignedInt(0); // former age, compatibility with 0.7, remove it in furher versions
    sgd.PushObjectContainer(troops_on_mission, false);
    sgd.PushObjectContainer(aggressors, true);
    sgd.PushObjectContainer(aggressive_defenders, true);
    sgd.PushObject(defender_, true);
}

nobBaseMilitary::nobBaseMilitary(SerializedGameData& sgd, const unsigned obj_id) : noBuilding(sgd, obj_id)
{
    sgd.PopObjectContainer(leave_house, GOT_UNKNOWN);
    leaving_event = sgd.PopEvent();
    go_out = sgd.PopBool();
    sgd.PopUnsignedInt(); // former age, compatibility with 0.7, remove it in furher versions
    sgd.PopObjectContainer(troops_on_mission, GOT_UNKNOWN);
    sgd.PopObjectContainer(aggressors, GOT_NOF_ATTACKER);
    sgd.PopObjectContainer(aggressive_defenders, GOT_NOF_AGGRESSIVEDEFENDER);
    defender_ = sgd.PopObject<nofDefender>(GOT_NOF_DEFENDER);
}

void nobBaseMilitary::AddLeavingEvent()
{
    // Wenn gerade keiner rausgeht, muss neues Event angemeldet werden
    if(!go_out)
    {
        leaving_event = GetEvMgr().AddEvent(this, 20 + RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 10));
        go_out = true;
    }
}

void nobBaseMilitary::AddLeavingFigure(noFigure* fig)
{
    AddLeavingEvent();
    leave_house.push_back(fig);
}

nofAttacker* nobBaseMilitary::FindAggressor(nofAggressiveDefender* defender)
{
    // Look for other attackers on this building that are close and ready to fight
    for(std::list<nofAttacker*>::iterator it = aggressors.begin(); it != aggressors.end(); ++it)
    {
        // The attacker must be ready to fight and must not already have another hunting defender
        if(!(*it)->IsReadyForFight() || (*it)->GetHuntingDefender())
            continue;

        const MapPoint attackerPos = (*it)->GetPos();
        const MapPoint defenderPos = defender->GetPos();
        if(attackerPos == defenderPos)
        {
            // Both are at same pos --> Go!
            (*it)->LetsFight(defender);
            return (*it);
        }
        // Check roughly the distance
        if(gwg->CalcDistance(attackerPos, defenderPos) <= 5)
        {
            // Check it further (e.g. if they have to walk around a river...)
            if(gwg->FindHumanPath(attackerPos, defenderPos, 5) != 0xFF)
            {
                (*it)->LetsFight(defender);
                return (*it);
            }
        }
    }

    return NULL;
}

struct GetMapPointWithRadius
{
    typedef std::pair<MapPoint, unsigned> result_type;

    result_type operator()(const MapPoint pt, unsigned r) { return std::make_pair(pt, r); }
};

MapPoint nobBaseMilitary::FindAnAttackerPlace(unsigned short& ret_radius, nofAttacker* soldier)
{
    const MapPoint flagPos = gwg->GetNeighbour(pos, Direction::SOUTHEAST);

    // Diesen Flaggenplatz nur nehmen, wenn es auch nich gerade eingenommen wird, sonst gibts Deserteure!
    // Eigenommen werden können natürlich nur richtige Militärgebäude
    bool capturing = (BuildingProperties::IsMilitary(bldType_)) ? (static_cast<nobMilitary*>(this)->IsBeingCaptured()) : false;

    if(!capturing && gwg->ValidPointForFighting(flagPos, false))
    {
        ret_radius = 0;
        return flagPos;
    }

    const MapPoint soldierPos = soldier->GetPos();
    // Get points AROUND the flag. Never AT the flag
    std::vector<GetMapPointWithRadius::result_type> nodes = gwg->GetPointsInRadius(flagPos, 3, GetMapPointWithRadius());

    // Weg zu allen möglichen Punkten berechnen und den mit den kürzesten Weg nehmen
    // Die bisher kürzeste gefundene Länge
    unsigned min_length = std::numeric_limits<unsigned>::max();
    MapPoint minPt = MapPoint::Invalid();
    ret_radius = 100;
    for(std::vector<GetMapPointWithRadius::result_type>::iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
        // We found a point with a better radius
        if(it->second > ret_radius)
            break;

        if(!gwg->ValidWaitingAroundBuildingPoint(it->first, soldier, pos))
            continue;

        // Derselbe Punkt? Dann können wir gleich abbrechen, finden ja sowieso keinen kürzeren Weg mehr
        if(soldierPos == it->first)
        {
            ret_radius = it->second;
            return it->first;
        }

        unsigned length = 0;
        // Gültiger Weg gefunden
        if(gwg->FindHumanPath(soldierPos, it->first, 100, false, &length) != INVALID_DIR)
        {
            // Kürzer als bisher kürzester Weg? --> Dann nehmen wir diesen Punkt (vorerst)
            if(length < min_length)
            {
                minPt = it->first;
                ret_radius = it->second;
                min_length = length;
            }
        }
    }
    return minPt;
}

bool nobBaseMilitary::CallDefender(nofAttacker* attacker)
{
    // Ist noch ein Verteidiger draußen (der z.B. grad wieder reingeht?
    if(defender_)
    {
        // Dann nehmen wir den, müssen ihm nur den neuen Angreifer mitteilen
        defender_->NewAttacker(attacker);
        // Leute, die aus diesem Gebäude zum Angriff/aggressiver Verteidigung rauskommen wollen,
        // blocken
        CancelJobs();

        return true;
    }
    // ansonsten einen neuen aus dem Gebäude holen
    else if((defender_ = ProvideDefender(attacker)))
    {
        // Leute, die aus diesem Gebäude zum Angriff/aggressiver Verteidigung rauskommen wollen,
        // blocken
        CancelJobs();
        // Soldat muss noch rauskommen
        AddLeavingFigure(defender_);

        return true;
    } else
    {
        // Gebäude ist leer, dann kann es erobert werden
        return false;
    }
}

nofAttacker* nobBaseMilitary::FindAttackerNearBuilding()
{
    // Alle angreifenden Soldaten durchgehen
    // Den Soldaten, der am nächsten dran steht, nehmen
    nofAttacker* best_attacker = 0;
    unsigned best_radius = 0xFFFFFFFF;

    for(std::list<nofAttacker*>::iterator it = aggressors.begin(); it != aggressors.end(); ++it)
    {
        // Ist der Soldat überhaupt bereit zum Kämpfen (also wartet er um die Flagge herum oder rückt er nach)?
        if((*it)->IsAttackerReady())
        {
            // Besser als bisher bester?
            if((*it)->GetRadius() < best_radius || !best_attacker)
            {
                best_attacker = *it;
                best_radius = best_attacker->GetRadius();
            }
        }
    }

    if(best_attacker)
        // Den schließlich zur Flagge schicken
        best_attacker->AttackFlag(defender_);

    // und ihn zurückgeben, wenns keine gibt, natürlich 0
    return best_attacker;
}

void nobBaseMilitary::CheckArrestedAttackers()
{
    for(std::list<nofAttacker*>::iterator it = aggressors.begin(); it != aggressors.end(); ++it)
    {
        // Ist der Soldat überhaupt bereit zum Kämpfen (also wartet er um die Flagge herum)?
        if((*it)->IsAttackerReady())
        {
            // Und kommt er überhaupt zur Flagge (könnte ja in der 2. Reihe stehen, sodass die
            // vor ihm ihn den Weg versperren)?
            if(gwg->FindHumanPath((*it)->GetPos(), gwg->GetNeighbour(pos, Direction::SOUTHEAST), 5, false) != 0xFF)
            {
                // dann kann der zur Flagge gehen
                (*it)->AttackFlag();
                return;
            }
        }
    }
}

bool nobBaseMilitary::SendSuccessor(const MapPoint pt, const unsigned short radius, const Direction dir)
{
    for(std::list<nofAttacker*>::iterator it = aggressors.begin(); it != aggressors.end(); ++it)
    {
        // Wartet der Soldat überhaupt um die Flagge?
        if((*it)->IsAttackerReady())
        {
            // Und steht er auch weiter außen?, sonst machts natürlich keinen Sinn..
            if((*it)->GetRadius() > radius)
            {
                // Und findet er einen zu diesem Punkt?
                if(gwg->FindHumanPath((*it)->GetPos(), pt, 50, false) != 0xFF)
                {
                    // dann soll er dorthin gehen
                    (*it)->StartSucceeding(pt, radius, dir);

                    return true;
                }
            }
        }
    }

    return false;
}

bool nobBaseMilitary::IsAttackable(unsigned playerIdx) const
{
    // If we cannot be seen by the player -> not attackable
    if(gwg->CalcVisiblityWithAllies(pos, playerIdx) != VIS_VISIBLE)
        return false;
    // Else it depends on the team settings
    return gwg->GetPlayer(player).IsAttackable(playerIdx);
}

bool nobBaseMilitary::IsAggressor(nofAttacker* attacker) const
{
    return helpers::contains(aggressors, attacker);
}

bool nobBaseMilitary::IsAggressiveDefender(nofAggressiveDefender* soldier) const
{
    return helpers::contains(aggressive_defenders, soldier);
}

bool nobBaseMilitary::IsOnMission(nofActiveSoldier* soldier) const
{
    return helpers::contains(troops_on_mission, soldier);
}

/// Bricht einen aktuell von diesem Haus gestarteten Angriff/aggressive Verteidigung ab, d.h. setzt die Soldaten
/// aus der Warteschleife wieder in das Haus --> wenn Angreifer an der Fahne ist und Verteidiger rauskommen soll
void nobBaseMilitary::CancelJobs()
{
    // Soldaten, die noch in der Warteschlange hängen, rausschicken
    for(std::list<noFigure*>::iterator it = leave_house.begin(); it != leave_house.end();)
    {
        // Nur Soldaten nehmen (Job-Arbeiten) und keine (normalen) Verteidiger, da diese ja rauskommen
        // sollen zum Kampf
        if((*it)->DoJobWorks() && (*it)->GetGOT() != GOT_NOF_DEFENDER)
        {
            nofActiveSoldier* soldier = dynamic_cast<nofActiveSoldier*>(*it);
            RTTR_Assert(soldier);

            // Wenn er Job-Arbeiten verrichtet, ists ein ActiveSoldier --> dem muss extra noch Bescheid gesagt werden!
            soldier->InformTargetsAboutCancelling();
            // Wieder in das Haus verfrachten
            this->AddActiveSoldier(soldier);
            it = leave_house.erase(it);
        } else
            ++it;
    }

    // leave_house.clear();
}
