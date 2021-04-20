// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofCatapultMan.h"
#include "CatapultStone.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobUsual.h"
#include "enum_cast.hpp"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "gameData/JobConsts.h"
#include "gameData/MapConsts.h"

const std::array<DrawPoint, 6> STONE_STARTS = {{{-4, -48}, {-3, -47}, {-13, -47}, {-11, -48}, {-13, -47}, {-2, -47}}};

nofCatapultMan::PossibleTarget::PossibleTarget(SerializedGameData& sgd)
    : pos(sgd.PopMapPoint()), distance(sgd.PopUnsignedInt())
{}

void nofCatapultMan::PossibleTarget::Serialize(SerializedGameData& sgd) const
{
    helpers::pushPoint(sgd, pos);
    sgd.PushUnsignedInt(distance);
}

nofCatapultMan::nofCatapultMan(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofBuildingWorker(Job::Helper, pos, player, workplace), wheel_steps(0)
{}

nofCatapultMan::nofCatapultMan(SerializedGameData& sgd, const unsigned obj_id)
    : nofBuildingWorker(sgd, obj_id), wheel_steps(sgd.PopSignedInt()), target(sgd)
{}

void nofCatapultMan::Serialize(SerializedGameData& sgd) const
{
    nofBuildingWorker::Serialize(sgd);

    sgd.PushSignedInt(wheel_steps);
    target.PossibleTarget::Serialize(sgd);
}

void nofCatapultMan::WalkedDerived() {}

void nofCatapultMan::DrawWorking(DrawPoint drawPt)
{
    // Offset of the catapult
    drawPt -= DrawPoint(7, 19);
    switch(state)
    {
        default: return;
        case State::CatapultTargetBuilding:
        {
            int step = GAMECLIENT.Interpolate(std::abs(wheel_steps) + 1, current_ev);

            if(step <= std::abs(wheel_steps))
            {
                if(wheel_steps < 0)
                    step = -step;

                // Katapult auf dem Dach mit Stein drehend zeichnen
                LOADER.GetPlayerImage("rom_bobs", 1781 + (7 + step) % 6)->DrawFull(drawPt);
            }
            // else
            //  // Katapult schießend zeichnen
            //  LOADER.GetPlayerImage("rom_bobs", 1787+(7+wheel_steps)%6)->Draw(x-7,y-19);
        }
        break;
        case State::CatapultBackoff:
        {
            int step = GAMECLIENT.Interpolate((std::abs(wheel_steps) + 3) * 2, current_ev);

            if(step < 2 * 3)
                // Katapult nach Schießen zeichnen (hin und her wippen
                LOADER.GetPlayerImage("rom_bobs", 1787 + (step % 2) * 6 + (7 + wheel_steps) % 6)->DrawFull(drawPt);
            else
            {
                step = (step - 6) / 2;

                if(wheel_steps < 0)
                    step = -(step);

                // Katapult auf dem Dach mit Stein drehend zeichnen (zurück in Ausgangsposition: Richtung 4)
                LOADER.GetPlayerImage("rom_bobs", 1775 + (7 + wheel_steps - step) % 6)->DrawFull(drawPt);
            }
        }
        break;
    }
}

void nofCatapultMan::HandleDerivedEvent(const unsigned /*id*/)
{
    switch(state)
    {
        default: break;
        case State::Waiting1:
        {
            // Fertig mit warten --> anfangen zu arbeiten

            // Liste von potentiellen Zielen
            std::vector<PossibleTarget> possibleTargets;

            sortedMilitaryBlds buildings = world->LookForMilitaryBuildings(pos, 3);
            for(auto* building : buildings)
            {
                // Auch ein richtiges Militärgebäude (kein HQ usw.),
                if(building->GetGOT() == GO_Type::NobMilitary
                   && world->GetPlayer(player).IsAttackable(building->GetPlayer()))
                {
                    // Was nicht im Nebel liegt und auch schon besetzt wurde (nicht neu gebaut)?
                    if(world->GetNode(building->GetPos()).fow[player].visibility == Visibility::Visible
                       && !static_cast<nobMilitary*>(building)->IsNewBuilt())
                    {
                        // Entfernung ausrechnen
                        unsigned distance = world->CalcDistance(pos, building->GetPos());

                        // Entfernung nicht zu hoch?
                        if(distance < 14)
                        {
                            // Mit in die Liste aufnehmen
                            possibleTargets.push_back(PossibleTarget(building->GetPos(), distance));
                        }
                    }
                }
            }

            // Gibts evtl keine Ziele?
            if(possibleTargets.empty())
            {
                // Weiter warten, vielleicht gibts ja später wieder mal was
                current_ev = GetEvMgr().AddEvent(this, CATAPULT_WAIT1_LENGTH, 1);
                workplace->StartNotWorking();
                return;
            }

            // Waren verbrauchen
            workplace->ConsumeWares();

            // Eins zufällig auswählen
            target = RANDOM_ELEMENT(possibleTargets);

            // Get distance and direction
            int distX;
            bool targetIsRight; // We should face right (to higher x)
            if(target.pos.x > pos.x)
            {
                distX = target.pos.x - pos.x;
                targetIsRight = true;
            } else
            {
                distX = pos.x - target.pos.x;
                targetIsRight = false;
            }
            // Distance over map border is closer (max distance is size/2 due to wrap around)
            if(distX > world->GetWidth() / 2)
            {
                distX -= world->GetWidth() / 2;
                targetIsRight = !targetIsRight; // Reverse direction
            }

            // Same for Y
            int distY;
            bool targetIsDown; // We should face down (to higher y)
            if(target.pos.y > pos.y)
            {
                distY = target.pos.y - pos.y;
                targetIsDown = true;
            } else
            {
                distY = pos.y - target.pos.y;
                targetIsDown = false;
            }
            // Distance over map border is closer (max distance is size/2 due to wrap around)
            if(distY > world->GetHeight() / 2)
            {
                distY -= world->GetHeight() / 2;
                targetIsDown = !targetIsDown; // Reverse direction
            }

            // Richtung, in die sich der Katapult drehen soll, bestimmen
            Direction shooting_dir;

            // Y-Abstand nur unwesentlich klein --> Richtung 0 und 3 (direkt gegenüber) nehmen
            if(distY <= distX / 5)
                shooting_dir = (targetIsRight) ? Direction::East : Direction::West;
            else
            {
                // Ansonsten noch y mit berücksichtigen und je einen der 4 Quadranten nehmen
                if(targetIsDown)
                    shooting_dir = (targetIsRight) ? Direction::SouthEast : Direction::SouthWest;
                else
                    shooting_dir = (targetIsRight) ? Direction::NorthEast : Direction::NorthWest;
            }

            // "Drehschritte" ausrechnen, da von Richtung 4 aus gedreht wird
            wheel_steps = int(rttr::enum_cast(shooting_dir)) - 4;
            if(wheel_steps < -3)
                wheel_steps = 6 + wheel_steps;

            current_ev = GetEvMgr().AddEvent(this, 15 * (std::abs(wheel_steps) + 1), 1);

            state = State::CatapultTargetBuilding;

            // wir arbeiten
            workplace->is_working = true;
        }
        break;
        case State::CatapultTargetBuilding:
        {
            // Stein in Bewegung setzen

            // Soll das Gebäude getroffen werden (70%)
            bool hit = (RANDOM_RAND(99) < 70);

            // Radius fürs Treffen und Nicht-Treffen,  (in Pixeln), nur visuell
            const int RADIUS_HIT = 15; // nicht nach unten hin!

            // Zielkoordinaten als (Map-Koordinaten!)
            MapPoint destMap;

            if(hit)
            {
                // Soll getroffen werden --> Aufschlagskoordinaten gleich dem eigentlichem Ziel
                destMap = target.pos;
            } else
            {
                // Ansonsten zufälligen Punkt rundrum heraussuchen
                destMap = world->GetNeighbour(target.pos, RANDOM_ENUM(Direction));
            }

            unsigned shooting_dir = (7 + wheel_steps) % 6;

            // Größe der Welt in Pixeln bestimmen
            int worldWidth = world->GetWidth() * TR_W;
            int worldHeight = world->GetHeight() * TR_H;

            // Startpunkt bestimmen
            Position start = world->GetNodePos(pos) + STONE_STARTS[shooting_dir]; //-V557
            // (Visuellen) Aufschlagpunkt bestimmen
            Position dest = world->GetNodePos(destMap);

            // Kartenränder beachten
            // Wenn Abstand kleiner is, den kürzeren Abstand über den Kartenrand wählen
            if(std::abs(start.x + worldWidth - dest.x) < std::abs(start.x - dest.x))
                start.x += worldWidth;
            else if(std::abs(start.x - worldWidth - dest.x) < std::abs(start.x - dest.x))
                start.x -= worldWidth;
            if(std::abs(start.y + worldHeight - dest.y) < std::abs(start.y - dest.y))
                start.y += worldHeight;
            else if(std::abs(start.y - worldHeight - dest.y) < std::abs(start.y - dest.y))
                start.y -= worldHeight;

            // Bei getroffenen den Aufschlagspunkt am Gebäude ein bisschen variieren
            if(hit)
            {
                dest.x += (RANDOM_RAND(RADIUS_HIT * 2) - RADIUS_HIT);
                // hier nicht nach unten gehen, da die Tür (also Nullpunkt
                // ja schon ziemlich weit unten ist!
                dest.y -= RANDOM_RAND(RADIUS_HIT);
            }

            // Stein erzeugen
            world->AddCatapultStone(new CatapultStone(target.pos, destMap, start, dest, 80));

            // Katapult wieder in Ausgangslage zurückdrehen
            current_ev = GetEvMgr().AddEvent(this, 15 * (std::abs(wheel_steps) + 3), 1);

            state = State::CatapultBackoff;
        }
        break;
        case State::CatapultBackoff:
        {
            current_ev = nullptr;
            // wir arbeiten nicht mehr
            workplace->is_working = false;
            // Wieder versuchen, zu arbeiten
            TryToWork();
        }
        break;
    }
}

void nofCatapultMan::WorkAborted() {}
