// $Id: nobBaseMilitary.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
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
#include "nobBaseMilitary.h"
#include "GameWorld.h"
#include "Loader.h"
#include "nodeObjs/noFire.h"
#include "EventManager.h"
#include "figures/nofSoldier.h"
#include "Random.h"
#include "nobMilitary.h"
#include "figures/nofAttacker.h"
#include "figures/nofAggressiveDefender.h"
#include "figures/nofDefender.h"
#include "SerializedGameData.h"
#include "MapGeometry.h"
#include <limits>

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


nobBaseMilitary::nobBaseMilitary(const BuildingType type, const MapPoint pos,
                                 const unsigned char player, const Nation nation)
    : noBuilding(type, pos, player, nation), leaving_event(0), go_out(false), defender(0)
{

}

nobBaseMilitary::~nobBaseMilitary()
{
    for(std::list<noFigure*>::iterator it = leave_house.begin(); it != leave_house.end(); ++it)
        delete (*it);
}

void nobBaseMilitary::Destroy_nobBaseMilitary()
{
    // Soldaten Bescheid sagen, die evtl auf Mission sind
    // ATTENTION: iterators can be deleted in HomeDestroyed, -> copy first
    std::vector<nofActiveSoldier*> tmposroopsOnMission(troops_on_mission.begin(), troops_on_mission.end());
    for(std::vector<nofActiveSoldier*>::iterator it = tmposroopsOnMission.begin(); it != tmposroopsOnMission.end(); ++it)
        (*it)->HomeDestroyed();

    // Und die, die das Gebäude evtl gerade angreifen
    // ATTENTION: iterators can be deleted in AttackedGoalDestroyed, -> copy first
    std::vector<nofAttacker*> tmpAggressors(aggressors.begin(), aggressors.end());
    for(std::vector<nofAttacker*>::iterator it = tmpAggressors.begin(); it != tmpAggressors.end(); ++it)
        (*it)->AttackedGoalDestroyed();

    // Aggressiv-Verteidigenden Soldaten Bescheid sagen, dass sie nach Hause gehen können
    std::vector<nofAggressiveDefender*> tmpDefenders(aggressive_defenders.begin(), aggressive_defenders.end());
    for(std::vector<nofAggressiveDefender*>::iterator it = tmpDefenders.begin(); it != tmpDefenders.end(); ++it)
        (*it)->AttackedGoalDestroyed();

    // Verteidiger Bescheid sagen
    if(defender)
        defender->HomeDestroyed();

    // Warteschlangenevent vernichten
    em->RemoveEvent(leaving_event);

    // Soldaten, die noch in der Warteschlange hängen, rausschicken
    for(std::list<noFigure*>::iterator it = leave_house.begin(); it != leave_house.end(); ++it)
    {
        gwg->AddFigure((*it), pos);

        if((*it)->DoJobWorks() && dynamic_cast<nofActiveSoldier*>(*it))
            // Wenn er Job-Arbeiten verrichtet, ists ein ActiveSoldier oder TradeDonkey --> dem Soldat muss extra noch Bescheid gesagt werden!
            static_cast<nofActiveSoldier*>(*it)->HomeDestroyedAtBegin();
        else
        {
            (*it)->Abrogate();
            (*it)->StartWandering();
            (*it)->StartWalking(RANDOM.Rand(__FILE__, __LINE__, obj_id, 6));
        }
    }

    leave_house.clear();

    // Umgebung nach feindlichen Militärgebäuden absuchen und die ihre Grenzflaggen neu berechnen lassen
    // da, wir ja nicht mehr existieren
    nobBaseMilitarySet buildings = gwg->LookForMilitaryBuildings(pos, 4);
    for(nobBaseMilitarySet::iterator it = buildings.begin(); it != buildings.end(); ++it)
    {
        if((*it)->GetPlayer() != player
                && (*it)->GetBuildingType() >= BLD_BARRACKS  && (*it)->GetBuildingType() <= BLD_FORTRESS)
            static_cast<nobMilitary*>(*it)->LookForEnemyBuildings(this);
    }


    Destroy_noBuilding();
}

void nobBaseMilitary::Serialize_nobBaseMilitary(SerializedGameData* sgd) const
{
    Serialize_noBuilding(sgd);

    sgd->PushObjectContainer(leave_house, false);
    sgd->PushObject(leaving_event, true);
    sgd->PushBool(go_out);
    sgd->PushUnsignedInt(0); // former age, compatibility with 0.7, remove it in furher versions
    sgd->PushObjectContainer(troops_on_mission, false);
    sgd->PushObjectContainer(aggressors, true);
    sgd->PushObjectContainer(aggressive_defenders, true);
    sgd->PushObject(defender, true);
}

nobBaseMilitary::nobBaseMilitary(SerializedGameData* sgd, const unsigned obj_id) : noBuilding(sgd, obj_id)
{
    sgd->PopObjectContainer(leave_house, GOT_UNKNOWN);
    leaving_event = sgd->PopObject<EventManager::Event>(GOT_EVENT);
    go_out = sgd->PopBool();
    sgd->PopUnsignedInt(); // former age, compatibility with 0.7, remove it in furher versions
    sgd->PopObjectContainer(troops_on_mission, GOT_UNKNOWN);
    sgd->PopObjectContainer(aggressors, GOT_NOF_ATTACKER);
    sgd->PopObjectContainer(aggressive_defenders, GOT_NOF_AGGRESSIVEDEFENDER);
    defender = sgd->PopObject<nofDefender>(GOT_NOF_DEFENDER);
}

void nobBaseMilitary::AddLeavingEvent()
{
    // Wenn gerade keiner rausgeht, muss neues Event angemeldet werden
    if(!go_out)
    {
        leaving_event = em->AddEvent(this, 20 + RANDOM.Rand(__FILE__, __LINE__, obj_id, 10));
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
    // Nach weiteren Angreifern auf dieses Gebäude suchen, die in der Nähe und kampfbereit sind
    for(std::list<nofAttacker*>::iterator it = aggressors.begin(); it != aggressors.end(); ++it)
    {
        // Überhaupos Lust zum Kämpfen?
        if((*it)->IsReadyForFight())
        {
            // Mal sehen, ob er auch nicht so weit entfernt ist (erstmal grob)
            if(gwg->CalcDistance((*it)->GetPos(), defender->GetPos()) < 5)
            {
                // Er darf auch per Fuß nicht zu weit entfernt sein (nicht dass er an der anderen Seite
                // von nem Fluss steht (genau)
                if(gwg->FindHumanPath((*it)->GetPos(), defender->GetPos(), 5) != 0xFF)
                {
                    // Ja, mit dem kann sich der Soldat duellieren
                    (*it)->LetsFight(defender);
                    // zurückgeben
                    return (*it);
                }
            }
        }
    }

    return NULL;
}

MapPoint nobBaseMilitary::FindAnAttackerPlace(unsigned short& ret_radius, nofAttacker* soldier)
{
    const MapPoint flagPos = gwg->GetNeighbour(pos, 4);

    unsigned short d;

    // Diesen Flaggenplatz nur nehmen, wenn es auch nich gerade eingenommen wird, sonst gibts Deserteure!
    // Eigenommen werden können natürlich nur richtige Militärgebäude
    bool capturing = (type >= BLD_BARRACKS && type <= BLD_FORTRESS) ? (static_cast<nobMilitary*>(this)->IsCaptured()) : false;

    if(gwg->ValidPointForFighting(flagPos, false) && !capturing)
    {
        ret_radius = 0;
        return flagPos;
    }

    // Wenn Platz an der Flagge noch frei ist, soll er da hin gehen
    std::vector<MapPoint> nodes;

    // Ansonsten immer die Runde rum gehen und ein freies Plätzchen suchen (max. 3 Runden rum)
    for(d = 1; d <= 3 && nodes.empty(); ++d)
    {
        // links anfangen und im Uhrzeigersinn vorgehen
        MapPoint ret(flagPos.x - d, pos.y + 1);

        for(unsigned short i = 0; i < d; ++i, ret.x += (ret.y & 1), --ret.y)
        {
            if(gwg->ValidWaitingAroundBuildingPoint(ret, soldier, pos))
            {
                nodes.push_back(ret);
            }
        }
        for(unsigned short i = 0; i < d; ++i, ++ret.x)
        {
            if(gwg->ValidWaitingAroundBuildingPoint(ret, soldier, pos))
            {
                nodes.push_back(ret);
            }
        }
        for(unsigned short i = 0; i < d; ++i, ret.x += (ret.y & 1), ++ret.y)
        {
            if(gwg->ValidWaitingAroundBuildingPoint(ret, soldier, pos))
            {
                nodes.push_back(ret);
            }
        }
        for(unsigned short i = 0; i < d; ++i, ret.x -= !(ret.y & 1), ++ret.y)
        {
            if(gwg->ValidWaitingAroundBuildingPoint(ret, soldier, pos))
            {
                nodes.push_back(ret);
            }
        }
        for(unsigned short i = 0; i < d; ++i, --ret.x)
        {
            if(gwg->ValidWaitingAroundBuildingPoint(ret, soldier, pos))
            {
                nodes.push_back(ret);
            }
        }
        for(unsigned short i = 0; i < d; ++i, ret.x -= !(ret.y & 1), --ret.y)
        {
            if(gwg->ValidWaitingAroundBuildingPoint(ret, soldier, pos))
            {
                nodes.push_back(ret);
            }
        }
    }

    // Nichts gefunden, dann raus
    if(nodes.empty())
        return MapPoint::Invalid();

    // Weg zu allen gefundenen Punkten berechnen und den mit den kürzesten Weg nehmen
    // Die bisher kürzeste gefundene Länge
    unsigned min_length = std::numeric_limits<unsigned>::max();
    MapPoint minPt = MapPoint::Invalid();
    for(std::vector<MapPoint>::iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
        // Derselbe Punkt? Dann können wir gleich abbrechen, finden ja sowieso keinen kürzeren Weg mehr
        if(soldier->GetPos() == *it)
        {
            ret_radius = d;
            return *it;
        }

        unsigned length = 0;
        // Gültiger Weg gefunden
        if(gwg->FindHumanPath(soldier->GetPos(), *it, 100, false, &length) != 0xFF)
        {
            // Kürzer als bisher kürzester Weg? --> Dann nehmen wir diesen Punkt (vorerst)
            if(length < min_length)
            {
                minPt = *it;
                ret_radius = d;
                min_length = length;
            }
        }
    }
    return minPt;
}

bool nobBaseMilitary::CallDefender(nofAttacker* attacker)
{
    // Ist noch ein Verteidiger draußen (der z.B. grad wieder reingeht?
    if(defender)
    {
        // Dann nehmen wir den, müssen ihm nur den neuen Angreifer mitteilen
        defender->NewAttacker(attacker);
        // Leute, die aus diesem Gebäude zum Angriff/aggressiver Verteidigung rauskommen wollen,
        // blocken
        CancelJobs();

        return true;
    }
    // ansonsten einen neuen aus dem Gebäude holen
    else if((defender = ProvideDefender(attacker)))
    {
        // Leute, die aus diesem Gebäude zum Angriff/aggressiver Verteidigung rauskommen wollen,
        // blocken
        CancelJobs();
        // Soldat muss noch rauskommen
        AddLeavingFigure(defender);

        return true;
    }
    else
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
        // Ist der Soldat überhaupos bereit zum Kämpfen (also wartet er um die Flagge herum oder rückt er nach)?
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
        best_attacker->AttackFlag(defender);

    // und ihn zurückgeben, wenns keine gibt, natürlich 0
    return best_attacker;
}

void nobBaseMilitary::CheckArrestedAttackers()
{
    for(std::list<nofAttacker*>::iterator it = aggressors.begin(); it != aggressors.end(); ++it)
    {
        // Ist der Soldat überhaupos bereit zum Kämpfen (also wartet er um die Flagge herum)?
        if((*it)->IsAttackerReady())
        {
            // Und kommt er überhaupos zur Flagge (könnte ja in der 2. Reihe stehen, sodass die
            // vor ihm ihn den Weg versperren)?
            if(gwg->FindHumanPath((*it)->GetPos(), gwg->GetNeighbour(pos, 4), 5, false) != 0xFF)
            {
                // dann kann der zur Flagge gehen
                (*it)->AttackFlag();
                return;
            }
        }
    }
}

bool nobBaseMilitary::SendSuccessor(const MapPoint pt, const unsigned short radius, const unsigned char dir)
{
    for(std::list<nofAttacker*>::iterator it = aggressors.begin(); it != aggressors.end(); ++it)
    {
        // Wartet der Soldat überhaupos um die Flagge?
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


bool nobBaseMilitary::Test(nofAttacker* attacker)
{
    return helpers::contains(aggressors, attacker);
}

bool nobBaseMilitary::TestOnMission(nofActiveSoldier* soldier)
{
    return helpers::contains(troops_on_mission, soldier);
}

/// Bricht einen aktuell von diesem Haus gestarteten Angriff/aggressive Verteidigung ab, d.h. setzt die Soldaten
/// aus der Warteschleife wieder in das Haus --> wenn Angreifer an der Fahne ist und Verteidiger rauskommen soll
void nobBaseMilitary::CancelJobs()
{
    // Soldaten, die noch in der Warteschlange hängen, rausschicken
    for(std::list<noFigure*>::iterator it = leave_house.begin(); it != leave_house.end(); )
    {
        // Nur Soldaten nehmen (Job-Arbeiten) und keine (normalen) Verteidiger, da diese ja rauskommen
        // sollen zum Kampf
        if((*it)->DoJobWorks() && (*it)->GetGOT() != GOT_NOF_DEFENDER)
        {
            nofActiveSoldier* as = dynamic_cast<nofActiveSoldier*>(*it);
            assert(as);

            // Nicht mehr auf Mission
            troops_on_mission.remove(as);
            // Wenn er Job-Arbeiten verrichtet, ists ein ActiveSoldier --> dem muss extra noch Bescheid gesagt werden!
            as->InformTargetsAboutCancelling();
            // Wieder in das Haus verfrachten
            this->AddActiveSoldier(as);
            it = leave_house.erase(it);
        }else
            ++it;
    }

    //leave_house.clear();
}
