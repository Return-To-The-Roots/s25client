// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofShipWright.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "buildings/nobShipYard.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "nodeObjs/noShipBuildingSite.h"
#include "gameTypes/Direction.h"
#include "gameData/JobConsts.h"
#include "s25util/colors.h"

nofShipWright::nofShipWright(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(Job::Shipwright, pos, player, workplace), curShipBuildPos(MapPoint::Invalid())
{
    RTTR_Assert(!workplace || dynamic_cast<nobShipYard*>(workplace));
}

const unsigned SHIPWRIGHT_RADIUS = 8;
const unsigned SHIPWRIGHT_WALKING_DISTANCE = 15;
/// Arbeitszeit des Schiffsbauers beim Bauen von großen Schiffen
const unsigned WORKING_TIME_SHIPS = 70;

namespace {
struct IsNotReserved
{
    const World& world;
    IsNotReserved(const World& world) : world(world) {}

    bool operator()(const MapPoint& pt) const { return !world.getNode(pt).reserved; }
};
} // namespace

void nofShipWright::HandleDerivedEvent(const unsigned /*id*/)
{
    switch(state)
    {
        case State::Waiting1:
        {
            // Herausfinden, was der Schiffsbauer als nächstes bauen soll
            if(static_cast<nobShipYard*>(workplace)->getMode() == nobShipYard::Mode::Boats)
                // in Handwerksmanier Boote herstellen
                nofWorkman::HandleStateWaiting1();
            else
            {
                // Wege müssen immer von der Flagge aus berechnet werden
                MapPoint flagPos = world->getNeighbour(pos, Direction::SouthEast);
                const std::vector<MapPoint> possiblePts =
                  world->GetMatchingPointsInRadius(flagPos, SHIPWRIGHT_RADIUS, IsNotReserved(*world));

                // Verfügbare Punkte, die geeignete Plätze darstellen würden
                std::vector<MapPoint> available_points;

                // Besitze ich noch ein Schiff, was gebaut werden muss?
                for(const auto& pt : possiblePts)
                {
                    noBase* obj = world->getNode(pt).obj;

                    if(!obj)
                        continue;

                    // Our ship
                    if(obj->getGOT() == GO_Type::Shipbuildingsite
                       && static_cast<noShipBuildingSite*>(obj)->getPlayer() == player)
                    {
                        if(world->FindHumanPath(flagPos, pt, SHIPWRIGHT_WALKING_DISTANCE))
                            available_points.push_back(pt);
                    }
                }

                // Kein Schiff im Bau gefunden? Dann Plätzchen für ein neues Schiff suchen
                if(available_points.empty())
                {
                    for(const auto& pt : possiblePts)
                    {
                        // Dieser Punkt geeignet?
                        if(isPointGood(pt) && world->FindHumanPath(flagPos, pt, SHIPWRIGHT_WALKING_DISTANCE))
                            available_points.push_back(pt);
                    }
                }

                // Punkte gefunden?
                if(!available_points.empty())
                {
                    // Einen Punkt zufällig auswählen und dorthin laufen
                    curShipBuildPos = RANDOM_ELEMENT(available_points);
                    StartWalkingToShip();
                } else
                {
                    // Nichts zu arbeiten gefunden
                    workplace->StartNotWorking();
                    // Weiter warten, vielleicht gibts ja später wieder mal was
                    current_ev = getEvMgr().AddEvent(this, JOB_CONSTS[job_].wait1_length, 1);
                }
            }
        }
        break;
        case State::Work:
        {
            // Sind wir an unserem Arbeitsplatz (dem Gebäude), wenn wir die Arbeit beendet haben, bauen wir nur Boote,
            // ansonsten sind wir an unserem Schiff und bauen große Schiffe
            if(workplace->getPos() == pos)
                // Boote bauen
                nofWorkman::HandleStateWork();
            else
            {
                // fertig mit Arbeiten --> dann müssen die "Folgen des Arbeitens" ausgeführt werden
                WorkFinished();
                // Objekt wieder freigeben
                world->setReserved(pos, false);
                // Wieder nach Hause gehen
                StartWalkingHome();

                // Evtl. Sounds löschen
                if(was_sounding)
                {
                    world->getSoundMgr().stopSounds(*this);
                    was_sounding = false;
                }
            }
        }
        break;
        case State::Waiting2:
        {
            // Hier ist die Sache klar, dieser State kann nur bei Handwerkern vorkommen
            nofWorkman::HandleStateWaiting2();
        }
        break;
        default: break;
    }
}

nofShipWright::nofShipWright(SerializedGameData& sgd, const unsigned obj_id)
    : nofWorkman(sgd, obj_id), curShipBuildPos(sgd.PopMapPoint())
{}

void nofShipWright::Serialize(SerializedGameData& sgd) const
{
    nofWorkman::Serialize(sgd);

    helpers::pushPoint(sgd, curShipBuildPos);
}

/// Startet das Laufen zu der Arbeitsstelle, dem Schiff
void nofShipWright::StartWalkingToShip()
{
    state = State::WalkToWorkpoint;
    // Wir arbeiten jetzt
    workplace->is_working = true;
    // Waren verbrauchen
    workplace->ConsumeWares();
    // Punkt für uns reservieren
    world->setReserved(curShipBuildPos, true);
    // Anfangen zu laufen (erstmal aus dem Haus raus!)
    StartWalking(Direction::SouthEast);

    workplace->StopNotWorking();
}

/// Ist ein bestimmter Punkt auf der Karte für den Schiffsbau geeignet
/// poc: differene to original game: points at a sea which cant have a harbor are invalid (original as long as there is
/// 1 harborpoint at any sea on the map any sea is valid)
bool nofShipWright::isPointGood(const MapPoint pt) const
{
    // Auf Wegen nicht bauen
    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        if(world->getPointRoad(pt, dir) != PointRoad::None)
            return false;
    }

    return (world->isPlayerTerritory(pt) && world->isCoastalPointToSeaWithHarbor(pt)
            && (world->getNO(pt)->getType() == NodalObjectType::Environment
                || world->getNO(pt)->getType() == NodalObjectType::Nothing));
}

void nofShipWright::WalkToWorkpoint()
{
    // Sind wir am Ziel angekommen?
    if(pos == curShipBuildPos)
    {
        // Anfangen zu arbeiten
        state = State::Work;
        current_ev = getEvMgr().AddEvent(this, WORKING_TIME_SHIPS, 1);
        return;
    }
    const auto dir = world->FindHumanPath(pos, curShipBuildPos, 20);
    // Weg suchen und gucken ob der Punkt noch in Ordnung ist
    if(!dir || (!isPointGood(curShipBuildPos) && world->getGOT(curShipBuildPos) != GO_Type::Shipbuildingsite))
    {
        // Punkt freigeben
        world->setReserved(curShipBuildPos, false);
        // Kein Weg führt mehr zum Ziel oder Punkt ist nich mehr in Ordnung --> wieder nach Hause gehen
        StartWalkingHome();
    } else
    {
        // All good, let's start walking there
        StartWalking(*dir);
    }
}

void nofShipWright::StartWalkingHome()
{
    state = State::WalkingHome;
    // Fahne vor dem Gebäude anpeilen
    curShipBuildPos = world->getNeighbour(workplace->getPos(), Direction::SouthEast);

    // Zu Laufen anfangen
    WalkHome();
}

void nofShipWright::WalkHome()
{
    // Sind wir zu Hause angekommen? (genauer an der Flagge !!)
    if(pos == curShipBuildPos)
    {
        // Weiteres übernimmt nofBuildingWorker
        WorkingReady();
        return;
    }
    const auto dir = world->FindHumanPath(pos, curShipBuildPos, SHIPWRIGHT_WALKING_DISTANCE);
    // Weg suchen und ob wir überhaupt noch nach Hause kommen
    if(dir)
    {
        // All good, let's start walking there
        StartWalking(*dir);
    } else
    {
        // Kein Weg führt mehr nach Hause--> Rumirren
        AbrogateWorkplace();
        StartWandering();
        Wander();
    }
}

void nofShipWright::WorkAborted()
{
    // Platz freigeben, falls man gerade arbeitet
    if((state == State::Work && workplace->getPos() != pos)
       || state == State::WalkToWorkpoint) //&& static_cast<nobShipYard*>(workplace)->GetMode() == nobShipYard::SHIPS)
        world->setReserved(curShipBuildPos, false);
}

/// Der Schiffsbauer hat einen Bauschritt bewältigt und geht wieder zurück zum Haus
void nofShipWright::WorkFinished()
{
    // Befindet sich an dieser Stelle schon ein Schiff oder müssen wir es erst noch hinsetzen?
    if(world->getGOT(pos) != GO_Type::Shipbuildingsite)
    {
        // Ggf Zierobjekte löschen
        auto* obj = world->getSpecObj<noBase>(pos);
        if(obj)
        {
            if(obj->getType() != NodalObjectType::Environment)
                // Mittlerweile wurde anderes Objekt hierhin gesetzt --> können kein Schiff mehr bauen
                return;

            world->DestroyNO(pos);
        }

        // Baustelle setzen
        world->setNO(pos, new noShipBuildingSite(pos, player));
        // Bauplätze drumrum neu berechnen
        world->RecalcBQAroundPointBig(pos);
    }

    // Schiff weiterbauen
    world->getSpecObj<noShipBuildingSite>(pos)->MakeBuildStep();
}

void nofShipWright::WalkedDerived()
{
    switch(state)
    {
        case State::WalkToWorkpoint: WalkToWorkpoint(); break;
        case State::WalkingHome: WalkHome(); break;
        default: break;
    }
}

const std::array<unsigned, 42> ANIMATION = {299, 300, 301, 302, 299, 300, 301, 302, 299, 300, 301, 302, 303, 303,
                                            304, 304, 305, 305, 306, 306, 307, 307, 299, 300, 301, 302, 299, 300,
                                            301, 302, 308, 309, 310, 311, 312, 313, 308, 309, 310, 311, 312, 313};

void nofShipWright::DrawWorking(DrawPoint drawPt)
{
    // Nicht mich zeichnen wenn ich im Haus arbeite
    if(this->pos == workplace->getPos())
        return;

    switch(state)
    {
        default: break;
        case State::Work:
        {
            unsigned id = GAMECLIENT.Interpolate(42, current_ev);
            unsigned graphics_id = ANIMATION[id];
            LOADER.getPlayerImage("rom_bobs", graphics_id)
              ->DrawFull(drawPt, COLOR_WHITE, world->getPlayer(player).color);

            // Steh-Hammer-Sound
            if(graphics_id == 300)
            {
                world->getSoundMgr().playNOSound(78, *this, id, 160 - rand() % 60);
                was_sounding = true;
            } else if(graphics_id == 303 || graphics_id == 307)
            {
                world->getSoundMgr().playNOSound(72, *this, id - id % 2, 160 - rand() % 60);
                was_sounding = true;
            }
        }
        break;
    }
}

/// Zeichnen der Figur in sonstigen Arbeitslagen
void nofShipWright::DrawOtherStates(DrawPoint drawPt)
{
    switch(state)
    {
        case State::WalkToWorkpoint:
        {
            // Schiffsbauer mit Brett zeichnen
            DrawWalking(drawPt, LOADER.getBob("jobs"), 92, false);
        }
        break;
        default: return;
    }
}
