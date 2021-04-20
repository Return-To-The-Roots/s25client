// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofFarmhand.h"
#include "EventManager.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "buildings/nobUsual.h"
#include "notifications/BuildingNote.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "gameData/JobConsts.h"

nofFarmhand::nofFarmhand(const Job job, const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofBuildingWorker(job, pos, player, workplace), dest(0, 0)
{}

void nofFarmhand::Serialize(SerializedGameData& sgd) const
{
    nofBuildingWorker::Serialize(sgd);

    helpers::pushPoint(sgd, dest);
}

nofFarmhand::nofFarmhand(SerializedGameData& sgd, const unsigned obj_id)
    : nofBuildingWorker(sgd, obj_id), dest(sgd.PopMapPoint())
{}

void nofFarmhand::WalkedDerived()
{
    switch(state)
    {
        case State::WalkToWorkpoint: WalkToWorkpoint(); break;
        case State::WalkingHome: WalkHome(); break;
        default: break;
    }
}

void nofFarmhand::HandleDerivedEvent(const unsigned /*id*/)
{
    switch(state)
    {
        case State::Work:
        {
            // fertig mit Arbeiten --> dann müssen die "Folgen des Arbeitens" ausgeführt werden
            WorkFinished();
            // Objekt wieder freigeben
            world->SetReserved(pos, false);
            // Wieder nach Hause gehen
            StartWalkingHome();

            // Evtl. Sounds löschen
            if(was_sounding)
            {
                world->GetSoundMgr().stopSounds(*this);
                was_sounding = false;
            }
        }
        break;
        case State::Waiting1:
        {
            // Fertig mit warten --> anfangen zu arbeiten
            // Work radius
            const unsigned max_radius = [](Job job) {
                switch(job)
                {
                    case Job::Carpenter: return 0;
                    case Job::Hunter:
                    case Job::Farmer: return 2;
                    case Job::CharBurner: return 3;
                    case Job::Woodcutter:
                    case Job::Forester: return 6;
                    case Job::Fisher: return 7;
                    case Job::Stonemason: return 8;
                    default: throw std::logic_error("Invalid job");
                }
            }(job_);
            // Additional radius delta r which is used when a point in radius r was found
            // I.e. looks till radius r + delta r
            const unsigned add_radius_when_found = [](Job job) {
                switch(job)
                {
                    case Job::Woodcutter:
                    case Job::Fisher:
                    case Job::Forester:
                    case Job::Carpenter:
                    case Job::Hunter:
                    case Job::Farmer:
                    case Job::CharBurner: return 1;
                    case Job::Stonemason: return 0;
                    default: throw std::logic_error("Invalid job");
                }
            }(job_);

            bool points_found = false;
            bool wait = false;
            // Anzahl der Radien, wo wir gültige Punkte gefunden haben
            unsigned radius_count = 0;

            helpers::EnumArray<std::vector<MapPoint>, PointQuality> available_points;

            for(MapCoord tx = world->GetXA(pos, Direction::West), r = 1; r <= max_radius;
                tx = world->GetXA(MapPoint(tx, pos.y), Direction::West), ++r)
            {
                // Wurde ein Punkt in diesem Radius gefunden?
                bool found_in_radius = false;

                MapPoint t2(tx, pos.y);
                for(const auto dir : helpers::enumRange(Direction::NorthEast))
                {
                    for(MapCoord r2 = 0; r2 < r; t2 = world->GetNeighbour(t2, dir), ++r2)
                    {
                        if(IsPointAvailable(t2))
                        {
                            if(!world->GetNode(t2).reserved)
                            {
                                available_points[GetPointQuality(t2)].push_back(MapPoint(t2));
                                found_in_radius = true;
                                points_found = true;
                            } else if(job_ == Job::Stonemason)
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
                    if(radius_count++ == add_radius_when_found)
                        break;
                }
            }

            // Are there any objects at all?
            if(points_found)
            {
                // Prefer points with lower class (better)
                for(auto& available_point : available_points)
                {
                    if(!available_point.empty())
                    {
                        dest = RANDOM_ELEMENT(available_point);
                        break;
                    }
                }

                state = State::WalkToWorkpoint;

                // Wir arbeiten jetzt
                workplace->is_working = true;
                workplace->StopNotWorking();

                // Punkt für uns reservieren
                world->SetReserved(dest, true);

                // Anfangen zu laufen (erstmal aus dem Haus raus!)
                StartWalking(Direction::SouthEast);
                WalkingStarted();
            } else if(wait)
            {
                // We have to wait, since we do not know whether there are any unreachable or reserved points where
                // there's more to get
                current_ev = GetEvMgr().AddEvent(this, JOB_CONSTS[job_].wait1_length, 1);

                workplace->StartNotWorking();
            } else
            {
                switch(job_)
                {
                    case Job::Stonemason:
                    case Job::Fisher: workplace->OnOutOfResources(); break;
                    case Job::Woodcutter:
                        world->GetNotifications().publish(BuildingNote(
                          BuildingNote::NoRessources, player, workplace->GetPos(), workplace->GetBuildingType()));
                        break;
                    default: break;
                }

                // Weiter warten, vielleicht gibts ja später wieder mal was
                current_ev = GetEvMgr().AddEvent(this, JOB_CONSTS[job_].wait1_length, 1);

                workplace->StartNotWorking();
            }
        }
        break;
        default: break;
    }
}

bool nofFarmhand::IsPointAvailable(const MapPoint pt) const
{
    // Gibts an diesen Punkt überhaupt die nötigen Vorraussetzungen für den Beruf?
    if(GetPointQuality(pt) != PointQuality::NotPossible)
    {
        // Gucken, ob ein Weg hinführt
        return world->FindHumanPath(this->pos, pt, 20) != boost::none;
    } else
        return false;
}

void nofFarmhand::WalkToWorkpoint()
{
    // Sind wir am Ziel angekommen?
    if(pos == dest)
    {
        // Anfangen zu arbeiten
        state = State::Work;
        current_ev = GetEvMgr().AddEvent(this, JOB_CONSTS[job_].work_length, 1);
        WorkStarted();
        return;
    }

    // Weg suchen und gucken ob der Punkt noch in Ordnung ist
    const auto dir = world->FindHumanPath(pos, dest, 20);
    if(!dir || GetPointQuality(dest) == PointQuality::NotPossible)
    {
        // Punkt freigeben
        world->SetReserved(dest, false);
        // Kein Weg führt mehr zum Ziel oder Punkt ist nich mehr in Ordnung --> wieder nach Hause gehen
        StartWalkingHome();
    } else
    {
        // All good, let's start walking there
        StartWalking(*dir);
    }
}

void nofFarmhand::StartWalkingHome()
{
    state = State::WalkingHome;
    // Fahne vor dem Gebäude anpeilen
    dest = world->GetNeighbour(workplace->GetPos(), Direction::SouthEast);

    // Zu Laufen anfangen
    WalkHome();
}

void nofFarmhand::WalkHome()
{
    // Sind wir zu Hause angekommen? (genauer an der Flagge !!)
    if(pos == dest)
    {
        // Weiteres übernimmt nofBuildingWorker
        WorkingReady();
        return;
    }

    const auto dir = world->FindHumanPath(pos, dest, 40);
    // Weg suchen und ob wir überhaupt noch nach Hause kommen
    if(!dir)
    {
        // Kein Weg führt mehr nach Hause--> Rumirren
        AbrogateWorkplace();
        StartWandering();
        Wander();
    } else
    {
        // All good, let's start walking there
        StartWalking(*dir);
    }
}

void nofFarmhand::WorkAborted()
{
    // Platz freigeben, falls man gerade arbeitet
    if(state == State::Work || state == State::WalkToWorkpoint)
        world->SetReserved(dest, false);
}

/// Zeichnen der Figur in sonstigen Arbeitslagen
void nofFarmhand::DrawOtherStates(DrawPoint drawPt)
{
    switch(state)
    {
        case State::WalkToWorkpoint:
        {
            // Normales Laufen zeichnen
            DrawWalking(drawPt);
        }
        break;
        default: return;
    }
}

/// Inform derived class about the start of the whole working process (at the beginning when walking out of the house)
void nofFarmhand::WalkingStarted() {}
