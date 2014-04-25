// $Id: nobBaseMilitary.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "nobBaseMilitary.h"
#include "GameWorld.h"
#include "Loader.h"
#include "noFire.h"
#include "EventManager.h"
#include "nofSoldier.h"
#include "Random.h"
#include "nobMilitary.h"
#include "nofAttacker.h"
#include "nofAggressiveDefender.h"
#include "nofDefender.h"
#include "SerializedGameData.h"
#include "MapGeometry.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


nobBaseMilitary::nobBaseMilitary(const BuildingType type, const unsigned short x, const unsigned short y,
                                 const unsigned char player, const Nation nation)
    : noBuilding(type, x, y, player, nation), leaving_event(0), go_out(false), defender(0)
{

}

nobBaseMilitary::~nobBaseMilitary()
{
    for(std::list<noFigure*>::iterator it = leave_house.begin(); it != leave_house.end(); ++it)
        delete (*it);
}

void nobBaseMilitary::Destroy_nobBaseMilitary()
{
    list<nofActiveSoldier*>::iterator next_it;

    // Soldaten Bescheid sagen, die evtl auf Mission sind
    // Achtung: Hier können Iteratoren gelöscht werden in HomeDestroyed, daher Sicherheitsschleife!
    for(list<nofActiveSoldier*>::iterator it = troops_on_mission.begin(); it.valid(); it = next_it)
    {
        next_it = it.GetNext();
        (*it)->HomeDestroyed();
    }

    list<nofAttacker*>::iterator next_it2;

    // Und die, die das Gebäude evtl gerade angreifen
    // Achtung: Hier können Iteratoren gelöscht werden in AttackedGoalDestroyed, daher Sicherheitsschleife!
    for(list<nofAttacker*>::iterator it = aggressors.begin(); it.valid(); it = next_it2)
    {
        next_it2 = it.GetNext();
        (*it)->AttackedGoalDestroyed();
    }

    // Aggressiv-Verteidigenden Soldaten Bescheid sagen, dass sie nach Hause gehen können
    for(list<nofAggressiveDefender*>::iterator it = aggressive_defenders.begin(); it.valid(); ++it)
        (*it)->AttackedGoalDestroyed();

    // Verteidiger Bescheid sagen
    if(defender)
        defender->HomeDestroyed();

    // Warteschlangenevent vernichten
    em->RemoveEvent(leaving_event);

    // Soldaten, die noch in der Warteschlange hängen, rausschicken
    for(std::list<noFigure*>::iterator it = leave_house.begin(); it != leave_house.end(); ++it)
    {
        gwg->AddFigure((*it), x, y);

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
    std::list<nobBaseMilitary*> buildings;
    gwg->LookForMilitaryBuildings(buildings, x, y, 4);

    for(std::list<nobBaseMilitary*>::iterator it = buildings.begin(); it != buildings.end(); ++it)
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

    sgd->PushObjectList(leave_house, false);
    sgd->PushObject(leaving_event, true);
    sgd->PushBool(go_out);
    sgd->PushUnsignedInt(0); // former age, compatibility with 0.7, remove it in furher versions
    sgd->PushObjectList(troops_on_mission, false);
    sgd->PushObjectList(aggressors, true);
    sgd->PushObjectList(aggressive_defenders, true);
    sgd->PushObject(defender, true);
}

nobBaseMilitary::nobBaseMilitary(SerializedGameData* sgd, const unsigned obj_id) : noBuilding(sgd, obj_id)
{
    sgd->PopObjectList(leave_house, GOT_UNKNOWN);
    leaving_event = sgd->PopObject<EventManager::Event>(GOT_EVENT);
    go_out = sgd->PopBool();
    sgd->PopUnsignedInt(); // former age, compatibility with 0.7, remove it in furher versions
    sgd->PopObjectList(troops_on_mission, GOT_UNKNOWN);
    sgd->PopObjectList(aggressors, GOT_NOF_ATTACKER);
    sgd->PopObjectList(aggressive_defenders, GOT_NOF_AGGRESSIVEDEFENDER);
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
    for(list<nofAttacker*>::iterator it = aggressors.begin(); it.valid(); ++it)
    {
        // Überhaupt Lust zum Kämpfen?
        if((*it)->IsReadyForFight())
        {
            // Mal sehen, ob er auch nicht so weit entfernt ist (erstmal grob)
            if(gwg->CalcDistance((*it)->GetX(), (*it)->GetY(), defender->GetX(), defender->GetY()) < 5)
            {
                // Er darf auch per Fuß nicht zu weit entfernt sein (nicht dass er an der anderen Seite
                // von nem Fluss steht (genau)
                if(gwg->FindHumanPath((*it)->GetX(), (*it)->GetY(), defender->GetX(), defender->GetY(), 5) != 0xFF)
                {
                    // Ja, mit dem kann sich der Soldat duellieren
                    (*it)->LetsFight(defender);
                    // zurückgeben
                    return (*it);
                }
            }
        }
    }

    return 0;
}


struct Node
{
    unsigned short x, y;
};

void nobBaseMilitary::FindAnAttackerPlace(unsigned short& ret_x, unsigned short& ret_y, unsigned short& ret_radius, nofAttacker* soldier)
{


    const unsigned short flag_x = gwg->GetXA(x, y, 4);
    const unsigned short flag_y = gwg->GetYA(x, y, 4);

    unsigned short d;

    // Diesen Flaggenplatz nur nehmen, wenn es auch nich gerade eingenommen wird, sonst gibts Deserteure!
    // Eigenommen werden können natürlich nur richtige Militärgebäude
    bool capturing = (type >= BLD_BARRACKS && type <= BLD_FORTRESS) ? (static_cast<nobMilitary*>(this)->IsCaptured()) : false;

    if(gwg->ValidPointForFighting(flag_x, flag_y, false) && !capturing)
    {
        ret_x = flag_x;
        ret_y = flag_y;
        ret_radius = 0;
        return;
    }

    // Wenn Platz an der Flagge noch frei ist, soll er da hin gehen
    list<Node> nodes;

    // Ansonsten immer die Runde rum gehen und ein freies Plätzchen suchen (max. 3 Runden rum)
    for(d = 1; d <= 3 && !nodes.size(); ++d)
    {
        // links anfangen und im Uhrzeigersinn vorgehen
        ret_x = flag_x - d;
        ret_y = this->y + 1;

        for(unsigned short i = 0; i < d; ++i, ret_x += (ret_y & 1), --ret_y)
        {
            if(gwg->ValidWaitingAroundBuildingPoint(ret_x, ret_y, soldier, x, y))
            {
                Node n = {ret_x, ret_y};
                nodes.push_back(n);
            }
        }
        for(unsigned short i = 0; i < d; ++i, ++ret_x)
        {
            if(gwg->ValidWaitingAroundBuildingPoint(ret_x, ret_y, soldier, x, y))
            {
                Node n = {ret_x, ret_y};
                nodes.push_back(n);
            }
        }
        for(unsigned short i = 0; i < d; ++i, ret_x += (ret_y & 1), ++ret_y)
        {
            if(gwg->ValidWaitingAroundBuildingPoint(ret_x, ret_y, soldier, x, y))
            {
                Node n = {ret_x, ret_y};
                nodes.push_back(n);
            }
        }
        for(unsigned short i = 0; i < d; ++i, ret_x -= !(ret_y & 1), ++ret_y)
        {
            if(gwg->ValidWaitingAroundBuildingPoint(ret_x, ret_y, soldier, x, y))
            {
                Node n = {ret_x, ret_y};
                nodes.push_back(n);
            }
        }
        for(unsigned short i = 0; i < d; ++i, --ret_x)
        {
            if(gwg->ValidWaitingAroundBuildingPoint(ret_x, ret_y, soldier, x, y))
            {
                Node n = {ret_x, ret_y};
                nodes.push_back(n);
            }
        }
        for(unsigned short i = 0; i < d; ++i, ret_x -= !(ret_y & 1), --ret_y)
        {
            if(gwg->ValidWaitingAroundBuildingPoint(ret_x, ret_y, soldier, x, y))
            {
                Node n = {ret_x, ret_y};
                nodes.push_back(n);
            }
        }
    }

    // Bisher noch nichts gefunden, x = Nirvana
    ret_x = 0xFFFF;

    // Nichts gefunden, dann raus
    if(!nodes.size())
        return;


    // Weg zu allen gefundenen Punkten berechnen und den mit den kürzesten Weg nehmen
    // Die bisher kürzeste gefundene Länge
    unsigned min_length = 0xFFFFFFFF;
    for(list<Node>::iterator it = nodes.begin(); it.valid(); ++it)
    {
        // Derselbe Punkt? Dann können wir gleich abbrechen, finden ja sowieso keinen kürzeren Weg mehr
        if(soldier->GetX() == it->x && soldier->GetY() == it->y)
        {
            ret_x = it->x;
            ret_y = it->y;
            ret_radius = d;
            return;
        }

        unsigned length = 0;
        // Gültiger Weg gefunden
        if(gwg->FindHumanPath(soldier->GetX(), soldier->GetY(), it->x, it->y, 100, false, &length) != 0xFF)
        {
            // Kürzer als bisher kürzester Weg? --> Dann nehmen wir diesen Punkt (vorerst)
            if(length < min_length)
            {
                ret_x = it->x;
                ret_y = it->y;
                ret_radius = d;
                min_length = length;
            }
        }
    }
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


    for(list<nofAttacker*>::iterator it = aggressors.begin(); it.valid(); ++it)
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
        best_attacker->AttackFlag(defender);

    // und ihn zurückgeben, wenns keine gibt, natürlich 0
    return best_attacker;
}

void nobBaseMilitary::CheckArrestedAttackers()
{
    for(list<nofAttacker*>::iterator it = aggressors.begin(); it.valid(); ++it)
    {
        // Ist der Soldat überhaupt bereit zum Kämpfen (also wartet er um die Flagge herum)?
        if((*it)->IsAttackerReady())
        {
            // Und kommt er überhaupt zur Flagge (könnte ja in der 2. Reihe stehen, sodass die
            // vor ihm ihn den Weg versperren)?
            if(gwg->FindHumanPath((*it)->GetX(), (*it)->GetY(), gwg->GetXA(x, y, 4), gwg->GetYA(x, y, 4), 5, false) != 0xFF)
            {
                // dann kann der zur Flagge gehen
                (*it)->AttackFlag();
                return;
            }
        }
    }
}

bool nobBaseMilitary::SendSuccessor(const unsigned short x, const unsigned short y, const unsigned short radius, const unsigned char dir)
{
    for(list<nofAttacker*>::iterator it = aggressors.begin(); it.valid(); ++it)
    {
        // Wartet der Soldat überhaupt um die Flagge?
        if((*it)->IsAttackerReady())
        {
            // Und steht er auch weiter außen?, sonst machts natürlich keinen Sinn..
            if((*it)->GetRadius() > radius)
            {
                // Und findet er einen zu diesem Punkt?
                if(gwg->FindHumanPath((*it)->GetX(), (*it)->GetY(), x, y, 50, false) != 0xFF)
                {
                    // dann soll er dorthin gehen
                    (*it)->StartSucceeding(x, y, radius, dir);

                    return true;
                }
            }
        }
    }

    return false;
}


bool nobBaseMilitary::Test(nofAttacker* attacker)
{
    if(aggressors.search(attacker).valid())
        return true;
    else
        return false;
}

bool nobBaseMilitary::TestOnMission(nofActiveSoldier* soldier)
{
    if(troops_on_mission.search(soldier).valid())
        return true;
    else
        return false;
}

/// Bricht einen aktuell von diesem Haus gestarteten Angriff/aggressive Verteidigung ab, d.h. setzt die Soldaten
/// aus der Warteschleife wieder in das Haus --> wenn Angreifer an der Fahne ist und Verteidiger rauskommen soll
void nobBaseMilitary::CancelJobs()
{
    // Soldaten, die noch in der Warteschlange hängen, rausschicken
    for(std::list<noFigure*>::iterator it = leave_house.begin(); it != leave_house.end(); ++it)
    {
        // Nur Soldaten nehmen (Job-Arbeiten) und keine (normalen) Verteidiger, da diese ja rauskommen
        // sollen zum Kampf
        if((*it)->DoJobWorks() && (*it)->GetGOT() != GOT_NOF_DEFENDER)
        {
            nofActiveSoldier* as = dynamic_cast<nofActiveSoldier*>(*it);
            assert(as);

            // Nicht mehr auf Mission
            troops_on_mission.erase(as);
            // Wenn er Job-Arbeiten verrichtet, ists ein ActiveSoldier --> dem muss extra noch Bescheid gesagt werden!
            as->InformTargetsAboutCancelling();
            // Wieder in das Haus verfrachten
            this->AddActiveSoldier(as);
            it = leave_house.erase(it);
            if(it == leave_house.end())
                break;

            //leave_house.erase(&it);
        }
    }

    //leave_house.clear();
}
