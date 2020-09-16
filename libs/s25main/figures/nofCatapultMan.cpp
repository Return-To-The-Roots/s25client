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
#include "world/GameWorldGame.h"
#include "gameData/JobConsts.h"
#include "gameData/MapConsts.h"

const std::array<DrawPoint, 6> STONE_STARTS = {{{-4, -48}, {-3, -47}, {-13, -47}, {-11, -48}, {-13, -47}, {-2, -47}}};

nofCatapultMan::PossibleTarget::PossibleTarget(SerializedGameData& sgd)
    : pos(sgd.PopMapPoint()), distance(sgd.PopUnsignedInt())
{}

void nofCatapultMan::PossibleTarget::Serialize_PossibleTarget(SerializedGameData& sgd) const
{
    sgd.PushMapPoint(pos);
    sgd.PushUnsignedInt(distance);
}

nofCatapultMan::nofCatapultMan(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofBuildingWorker(JOB_HELPER, pos, player, workplace), wheel_steps(0)
{}

nofCatapultMan::nofCatapultMan(SerializedGameData& sgd, const unsigned obj_id)
    : nofBuildingWorker(sgd, obj_id), wheel_steps(sgd.PopSignedInt()), target(sgd)
{}

void nofCatapultMan::Serialize_nofCatapultMan(SerializedGameData& sgd) const
{
    Serialize_nofBuildingWorker(sgd);

    sgd.PushSignedInt(wheel_steps);
    target.Serialize_PossibleTarget(sgd);
}

void nofCatapultMan::WalkedDerived() {}

void nofCatapultMan::DrawWorking(DrawPoint drawPt)
{
    // Offset of the catapult
    drawPt -= DrawPoint(7, 19);
    switch(state)
    {
        default: return;
        case STATE_CATAPULT_TARGETBUILDING:
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
        case STATE_CATAPULT_BACKOFF:
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
        case STATE_WAITING1:
        {
            // Fertig mit warten --> anfangen zu arbeiten

            // Liste von potentiellen Zielen
            std::vector<PossibleTarget> possibleTargets;

            sortedMilitaryBlds buildings = gwg->LookForMilitaryBuildings(pos, 3);
            for(auto& building : buildings)
            {
                // Auch ein richtiges Militärgebäude (kein HQ usw.),
                if(building->GetGOT() == GOT_NOB_MILITARY && gwg->GetPlayer(player).IsAttackable(building->GetPlayer()))
                {
                    // Was nicht im Nebel liegt und auch schon besetzt wurde (nicht neu gebaut)?
                    if(gwg->GetNode(building->GetPos()).fow[player].visibility == VIS_VISIBLE
                       && !static_cast<nobMilitary*>(building)->IsNewBuilt())
                    {
                        // Entfernung ausrechnen
                        unsigned distance = gwg->CalcDistance(pos, building->GetPos());

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
            target = possibleTargets[RANDOM.Rand(__FILE__, __LINE__, GetObjId(), possibleTargets.size())];

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
            if(distX > gwg->GetWidth() / 2)
            {
                distX -= gwg->GetWidth() / 2;
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
            if(distY > gwg->GetHeight() / 2)
            {
                distY -= gwg->GetHeight() / 2;
                targetIsDown = !targetIsDown; // Reverse direction
            }

            // Richtung, in die sich der Katapult drehen soll, bestimmen
            Direction shooting_dir;

            // Y-Abstand nur unwesentlich klein --> Richtung 0 und 3 (direkt gegenüber) nehmen
            if(distY <= distX / 5)
                shooting_dir = (targetIsRight) ? Direction::EAST : Direction::WEST;
            else
            {
                // Ansonsten noch y mit berücksichtigen und je einen der 4 Quadranten nehmen
                if(targetIsDown)
                    shooting_dir = (targetIsRight) ? Direction::SOUTHEAST : Direction::SOUTHWEST;
                else
                    shooting_dir = (targetIsRight) ? Direction::NORTHEAST : Direction::NORTHWEST;
            }

            // "Drehschritte" ausrechnen, da von Richtung 4 aus gedreht wird
            wheel_steps = int(rttr::enum_cast(shooting_dir)) - 4;
            if(wheel_steps < -3)
                wheel_steps = 6 + wheel_steps;

            current_ev = GetEvMgr().AddEvent(this, 15 * (std::abs(wheel_steps) + 1), 1);

            state = STATE_CATAPULT_TARGETBUILDING;

            // wir arbeiten
            workplace->is_working = true;
        }
        break;
        case STATE_CATAPULT_TARGETBUILDING:
        {
            // Stein in Bewegung setzen

            // Soll das Gebäude getroffen werden (70%)
            bool hit = (RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 99) < 70);

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
                unsigned d = RANDOM.Rand(__FILE__, __LINE__, GetObjId(), Direction::COUNT);

                destMap = gwg->GetNeighbour(target.pos, Direction::fromInt(d));
            }

            unsigned shooting_dir = (7 + wheel_steps) % 6;

            // Größe der Welt in Pixeln bestimmen
            int worldWidth = gwg->GetWidth() * TR_W;
            int worldHeight = gwg->GetHeight() * TR_H;

            // Startpunkt bestimmen
            Position start = gwg->GetNodePos(pos) + STONE_STARTS[shooting_dir]; //-V557
            // (Visuellen) Aufschlagpunkt bestimmen
            Position dest = gwg->GetNodePos(destMap);

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
                dest.x += (RANDOM.Rand(__FILE__, __LINE__, GetObjId(), RADIUS_HIT * 2) - RADIUS_HIT);
                // hier nicht nach unten gehen, da die Tür (also Nullpunkt
                // ja schon ziemlich weit unten ist!
                dest.y -= RANDOM.Rand(__FILE__, __LINE__, GetObjId(), RADIUS_HIT);
            }

            // Stein erzeugen
            gwg->AddCatapultStone(new CatapultStone(target.pos, destMap, start, dest, 80));

            // Katapult wieder in Ausgangslage zurückdrehen
            current_ev = GetEvMgr().AddEvent(this, 15 * (std::abs(wheel_steps) + 3), 1);

            state = STATE_CATAPULT_BACKOFF;
        }
        break;
        case STATE_CATAPULT_BACKOFF:
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
