﻿// $Id: nofFarmhand.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "defines.h"
#include "nofFarmhand.h"

#include "buildings/nobUsual.h"
#include "GameWorld.h"
#include "EventManager.h"
#include "Random.h"
#include "gameData/JobConsts.h"
#include "SoundManager.h"
#include "SerializedGameData.h"
#include "GameClient.h"
#include "ai/AIEventManager.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

nofFarmhand::nofFarmhand(const Job job, const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofBuildingWorker(job, pos, player, workplace), dest(0, 0)
{
}

void nofFarmhand::Serialize_nofFarmhand(SerializedGameData* sgd) const
{
    Serialize_nofBuildingWorker(sgd);

    sgd->PushMapPoint(dest);
}

nofFarmhand::nofFarmhand(SerializedGameData* sgd, const unsigned obj_id) : nofBuildingWorker(sgd, obj_id), dest(sgd->PopMapPoint())
{}


void nofFarmhand::WalkedDerived()
{
    switch(state)
    {
        case STATE_WALKTOWORKPOINT: WalkToWorkpoint(); break;
        case STATE_WALKINGHOME: WalkHome(); break;
        default:
            break;
    }
}


void nofFarmhand::HandleDerivedEvent(const unsigned int id)
{
    switch(state)
    {
        case STATE_WORK:
        {
            // fertig mit Arbeiten --> dann mÃ¼ssen die "Folgen des Arbeitens" ausgefÃ¼hrt werden
            WorkFinished();
            // Objekt wieder freigeben
            gwg->GetNode(pos).reserved = false;
            // Wieder nach Hause gehen
            StartWalkingHome();

            // Evtl. Sounds lÃ¶schen
            if(was_sounding)
            {
                SOUNDMANAGER.WorkingFinished(this);
                was_sounding = false;
            }

        } break;
        case STATE_WAITING1:
        {
            // Fertig mit warten --> anfangen zu arbeiten
            // Die Arbeitsradien der Berufe wie in JobConst.h (ab JOB_WOODCUTTER!)
            const unsigned char RADIUS[7] =
            { 6, 7, 6, 0, 8, 2, 2 };

            // Additional radius delta r which is used when a point in radius r was found
            // I.e. looks till radius r + delta r
            const unsigned ADD_RADIUS_WHEN_FOUND[7] =
            { 1, 1, 1, 1, 0, 1, 1};


            // Anzahl der Radien, wo wir gÃ¼ltige Punkte gefunden haben
            unsigned radius_count = 0;

            // Available points: 1st class and 2st class
            std::vector< MapPoint > available_points[3];

            unsigned max_radius = (job == JOB_CHARBURNER) ? 3 : RADIUS[job - JOB_WOODCUTTER];
            unsigned add_radius_when_found = (job == JOB_CHARBURNER) ? 1 : ADD_RADIUS_WHEN_FOUND[job - JOB_WOODCUTTER];

            bool points_found = false;
            bool wait = false;

            for(MapCoord tx = gwg->GetXA(pos, 0), r = 1; r <= max_radius; tx = gwg->GetXA(tx, pos.y, 0), ++r)
            {
                // Wurde ein Punkt in diesem Radius gefunden?
                bool found_in_radius = false;

                MapPoint t2(tx, pos.y);
                for(unsigned i = 2; i < 8; ++i)
                {
                    for(MapCoord r2 = 0; r2 < r; t2 = gwg->GetNeighbour(t2,  i % 6), ++r2)
                    {
                        if(IsPointAvailable(t2))
                        {
                            if (!gwg->GetNode(t2).reserved)
                            {
                                available_points[GetPointQuality(t2) - PQ_CLASS1].push_back(MapPoint(t2));
                                found_in_radius = true;
                                points_found = true;
                            }
                            else if (job == JOB_STONEMASON)
                            {
                                // just wait a little bit longer
                                wait = true;
                            }
                        }
                    }
                }


                // Nur die zwei ADD_RADIUS_WHEN_FOUND Radien erst einmal nehmen
                if(found_in_radius)
                {
                    if( radius_count++ == add_radius_when_found)
                        break;
                }
            }

            // Are there any objects at all?
            if(points_found)
            {
                // Prefer 1st class objects and use only 2nd class objects if there are no more other objects anymore
                MapPoint p(0, 0);
                for(unsigned i = 0; i < 3; ++i)
                {
                    if(!available_points[i].empty())
                    {
                        p = available_points[i][RANDOM.Rand(__FILE__, __LINE__, obj_id, available_points[i].size())];
                        break;
                    }
                }

                // Als neues Ziel nehmen
                dest = p;

                state = STATE_WALKTOWORKPOINT;

                // Wir arbeiten jetzt
                workplace->is_working = true;

                // Punkt fÃ¼r uns reservieren
                gwg->GetNode(dest).reserved = true;;

                // Anfangen zu laufen (erstmal aus dem Haus raus!)
                StartWalking(4);

                StopNotWorking();

                WalkingStarted();
            }
            else if (wait)
            {
                // We have to wait, since we do not know whether there are any unreachable or reserved points where there's more to get
                current_ev = em->AddEvent(this, JOB_CONSTS[job].wait1_length, 1);

                StartNotWorking();
            }
            else
            {

                if(GAMECLIENT.GetPlayerID() == this->player)
                {
                    if (!OutOfRessourcesMsgSent)
                    {
                        switch(job)
                        {
                            case JOB_STONEMASON:
                                GAMECLIENT.SendPostMessage(
                                    new ImagePostMsgWithLocation(_("No more stones in range"), PMC_GENERAL, pos, workplace->GetBuildingType(), workplace->GetNation()));
                                OutOfRessourcesMsgSent = true;
                                // ProduktivitÃ¤tsanzeige auf 0 setzen
                                workplace->SetProductivityToZero();
                                break;
                            case JOB_FISHER:
                                GAMECLIENT.SendPostMessage(
                                    new ImagePostMsgWithLocation(_("No more fishes in range"), PMC_GENERAL, pos, workplace->GetBuildingType(), workplace->GetNation()));
                                OutOfRessourcesMsgSent = true;
                                // ProduktivitÃ¤tsanzeige auf 0 setzen
                                workplace->SetProductivityToZero();
                                break;
                            default:
                                break;
                        }
                    }
                }

                // KI-Event erzeugen
                switch(workplace->GetBuildingType())
                {
                    case BLD_WOODCUTTER:
                    case BLD_QUARRY:
                    case BLD_FISHERY:
                        GAMECLIENT.SendAIEvent(new AIEvent::Building(AIEvent::NoMoreResourcesReachable, workplace->GetPos(), workplace->GetBuildingType()), player);
                        break;
                    default:
                        break;
                }

                // Weiter warten, vielleicht gibts ja spÃ¤ter wieder mal was
                current_ev = em->AddEvent(this, JOB_CONSTS[job].wait1_length, 1);

                StartNotWorking();
            }

        } break;
        default:
            break;
    }
}

bool nofFarmhand::IsPointAvailable(const MapPoint pt)
{
    // Gibts an diesen Punkt Ã¼berhaupt die nÃ¶tigen Vorraussetzungen fÃ¼r den Beruf?
    if(GetPointQuality(pt) != PQ_NOTPOSSIBLE)
    {
        // Gucken, ob ein Weg hinfÃ¼hrt
        if(gwg->FindHumanPath(this->pos, pt, 20) != 0xFF)
            return 1;
        else
            return 0;
    }
    else
        return 0;
}

void nofFarmhand::WalkToWorkpoint()
{
    // Sind wir am Ziel angekommen?
    if(pos == dest)
    {
        // Anfangen zu arbeiten
        state = STATE_WORK;
        current_ev = em->AddEvent(this, JOB_CONSTS[job].work_length, 1);
        WorkStarted();
    }
    // Weg suchen und gucken ob der Punkt noch in Ordnung ist
    else if((dir = gwg->FindHumanPath(pos, dest, 20)) == 0xFF || GetPointQuality(dest) == PQ_NOTPOSSIBLE)
    {
        // Punkt freigeben
        gwg->GetNode(dest).reserved = false;
        // Kein Weg fÃ¼hrt mehr zum Ziel oder Punkt ist nich mehr in Ordnung --> wieder nach Hause gehen
        StartWalkingHome();
    }
    else
    {
        // Alles ok, wir kÃ¶nnen hinlaufen
        StartWalking(dir);
    }
}

void nofFarmhand::StartWalkingHome()
{
    state = STATE_WALKINGHOME;
    // Fahne vor dem GebÃ¤ude anpeilen
    dest = gwg->GetNeighbour(workplace->GetPos(), 4);

    // Zu Laufen anfangen
    WalkHome();
}

void nofFarmhand::WalkHome()
{
    // Sind wir zu Hause angekommen? (genauer an der Flagge !!)
    if(pos == dest)
    {
        // Weiteres Ã¼bernimmt nofBuildingWorker
        WorkingReady();
    }
    // Weg suchen und ob wir Ã¼berhaupt noch nach Hause kommen
    else if((dir = gwg->FindHumanPath(pos, dest, 40)) == 0xFF)
    {
        // Kein Weg fÃ¼hrt mehr nach Hause--> Rumirren
        StartWandering();
        Wander();
        // Haus Bescheid sagen
        workplace->WorkerLost();
        workplace = 0;
    }
    else
    {
        // Alles ok, wir kÃ¶nnen hinlaufen
        StartWalking(dir);
    }
}


void nofFarmhand::WorkAborted()
{
    // Platz freigeben, falls man gerade arbeitet
    if(state == STATE_WORK || state == STATE_WALKTOWORKPOINT)
        gwg->GetNode(dest).reserved = false;

    WorkAborted_Farmhand();
}


void nofFarmhand::WorkAborted_Farmhand()
{
}


/// Zeichnen der Figur in sonstigen Arbeitslagen
void nofFarmhand::DrawOtherStates(const int x, const int y)
{
    switch(state)
    {
        case STATE_WALKTOWORKPOINT:
        {
            // Normales Laufen zeichnen
            DrawWalking(x, y);
        } break;
        default: return;
    }
}


/// Inform derived class about the start of the whole working process (at the beginning when walking out of the house)
void nofFarmhand::WalkingStarted()
{
}

