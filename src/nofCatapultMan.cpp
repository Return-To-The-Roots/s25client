// $Id: nofCatapultMan.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "nofCatapultMan.h"
#include "GameWorld.h"
#include "Random.h"
#include "nobMilitary.h"
#include "GameClient.h"
#include "nobUsual.h"
#include "CatapultStone.h"
#include "MapGeometry.h"

const int STONE_STARTS[12] = { -4, -48, -3, -47, -13, -47, -11, -48, -13, -47, -2, -47};


nofCatapultMan::nofCatapultMan(const unsigned short x,
                               const unsigned short y,
                               const unsigned char player,
                               nobUsual* workplace)
    : nofBuildingWorker(JOB_HELPER, x, y, player, workplace),
      wheel_steps(0)
{
}

nofCatapultMan::nofCatapultMan(SerializedGameData* sgd,
                               const unsigned obj_id)
    : nofBuildingWorker(sgd, obj_id),
      wheel_steps( sgd->PopSignedInt() ), target( sgd )
{

}


void nofCatapultMan::Serialize_nofCatapultMan(SerializedGameData* sgd) const
{
    Serialize_nofBuildingWorker(sgd);

    sgd->PushSignedInt(wheel_steps);
    target.Serialize_PossibleTarget(sgd);
}


void nofCatapultMan::WalkedDerived()
{
}


void nofCatapultMan::DrawWorking(int x, int y)
{
    switch(state)
    {
        default: return;
        case STATE_CATAPULT_TARGETBUILDING:
        {
            int step = GameClient::inst().Interpolate(abs(wheel_steps) + 1, current_ev);

            if(step <= abs(wheel_steps))
            {

                if(wheel_steps < 0)
                    step = -step;

                // Katapult auf dem Dach mit Stein drehend zeichnen
                LOADER.GetImageN("rom_bobs", 1781 + (7 + step) % 6)->Draw(x - 7, y - 19, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLOR_WHITE);
            }
            //else
            //  // Katapult schießend zeichnen
            //  LOADER.GetImageN("rom_bobs", 1787+(7+wheel_steps)%6)->Draw(x-7,y-19);

        } break;
        case STATE_CATAPULT_BACKOFF:
        {
            int step = GameClient::inst().Interpolate((abs(wheel_steps) + 3) * 2, current_ev);

            if(step < 2 * 3)
                // Katapult nach Schießen zeichnen (hin und her wippen
                LOADER.GetImageN("rom_bobs", 1787 + (step % 2) * 6 + (7 + wheel_steps) % 6)->Draw(x - 7, y - 19, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLOR_WHITE);
            else
            {
                step = (step - 6) / 2;

                if(wheel_steps < 0)
                    step = -(step);

                // Katapult auf dem Dach mit Stein drehend zeichnen (zurück in Ausgangsposition: Richtung 4)
                LOADER.GetImageN("rom_bobs", 1775 + (7 + wheel_steps - step) % 6)->Draw(x - 7, y - 19, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLOR_WHITE);
            }

        } break;

    }
}


//void nofCatapultMan::HandleStateTargetBuilding()
//{
//}
//
//void nofCatapultMan::HandleStateBackOff()
//{
//}



void nofCatapultMan::HandleDerivedEvent(const unsigned int id)
{
    switch(state)
    {
        default:
            break;
        case STATE_WAITING1:
        {
            // Fertig mit warten --> anfangen zu arbeiten
            std::list<nobBaseMilitary*> buildings;
            gwg->LookForMilitaryBuildings(buildings, x, y, 3);

            // Liste von potentiellen Zielen
            list<PossibleTarget> pts;


            for(std::list<nobBaseMilitary*>::iterator it = buildings.begin(); it != buildings.end(); ++it)
            {
                // Auch ein richtiges Militärgebäude (kein HQ usw.),
                if((*it)->GetGOT() == GOT_NOB_MILITARY && GameClient::inst().GetPlayer(player)->IsPlayerAttackable((*it)->GetPlayer()))
                {
                    // Was nicht im Nebel liegt und auch schon besetzt wurde (nicht neu gebaut)?
                    if(gwg->GetNode((*it)->GetX(), (*it)->GetY()).fow[player].visibility == VIS_VISIBLE
                            && !static_cast<nobMilitary*>((*it))->IsNewBuilt())
                    {
                        // Entfernung ausrechnen
                        unsigned distance = gwg->CalcDistance(x, y, (*it)->GetX(), (*it)->GetY());

                        // Entfernung nicht zu hoch?
                        if(distance < 14)
                        {
                            // Mit in die Liste aufnehmen
                            PossibleTarget pt((*it)->GetX(), (*it)->GetY(), distance);
                            pts.push_back(pt);
                        }
                    }
                }
            }

            // Gibts evtl keine Ziele?
            if(!pts.size())
            {
                // Weiter warten, vielleicht gibts ja später wieder mal was
                current_ev = em->AddEvent(this, CATAPULT_WAIT1_LENGTH, 1);
                StartNotWorking();
                return;
            }

            // Waren verbrauchen
            workplace->ConsumeWares();

            // Eins zufällig auswählen
            target = *pts[Random::inst().Rand(__FILE__, __LINE__, obj_id, pts.size())];

            // Richtung, in die sich der Katapult drehen soll, bestimmen
            unsigned char shooting_dir;

            // Normale X-Distanz (ohne Beachtung der Kartenränderüberquerung)
            unsigned x_dist = abs(int(target.x) - int(x));
            // Distanzen jeweils bei Überquerung des linken und rechten Randes
            unsigned x_dist1 = abs(int(target.x) - int(x) + gwg->GetWidth());
            unsigned x_dist2 = abs(int(target.x) - int(x) - gwg->GetWidth());
            // Minimale, d.h. im Endeffekt reale Distanz
            unsigned min_dist_x = min(min(x_dist, x_dist1), x_dist2);

            // Normale Y-Distanz (ohne Beachtung der Kartenränderüberquerung)
            unsigned y_dist = abs(int(target.y) - int(y));
            // Distanzen jeweils bei Überquerung des linken und rechten Randes
            unsigned y_dist1 = abs(int(target.y) - int(y) + gwg->GetHeight());
            unsigned y_dist2 = abs(int(target.y) - int(y) - gwg->GetHeight());
            // Minimale, d.h. im Endeffekt reale Distanz
            unsigned min_dist_y = min(min(y_dist, y_dist1), y_dist2);

            bool side_x = (x < target.x);
            if(x_dist > x_dist1 || x_dist > x_dist2) side_x = !side_x; // Wenn er über Kartengrenze schießt, Richtung umkehren
            bool side_y = (y < target.y);
            if(y_dist > y_dist1 || y_dist > y_dist2) side_y = !side_y;

            // Y-Abstand nur unwesentlich klein --> Richtung 0 und 3 (direkt gegenüber) nehmen
            if(min_dist_y <= min_dist_x / 5)
                shooting_dir = (side_x) ? 3 : 0;
            else
            {
                // Ansonsten noch y mit berücksichtigen und je einen der 4 Quadranten nehmen
                if(side_y)
                    shooting_dir = (side_x) ? 4 : 5;
                else
                    shooting_dir = (side_x) ? 2 : 1;
            }

            // "Drehschritte" ausrechnen, da von Richtung 4 aus gedreht wird
            wheel_steps = int(shooting_dir) - 4;
            if(wheel_steps < -3)
                wheel_steps = 6 + wheel_steps;

            current_ev = em->AddEvent(this, 15 * (abs(wheel_steps) + 1), 1);

            state = STATE_CATAPULT_TARGETBUILDING;

            // wir arbeiten
            workplace->is_working = true;

        } break;
        case STATE_CATAPULT_TARGETBUILDING:
        {
            // Stein in Bewegung setzen

            // Soll das Gebäude getroffen werden (70%)
            bool hit = (Random::inst().Rand(__FILE__, __LINE__, obj_id, 99) < 70);

            // Radius fürs Treffen und Nicht-Treffen,  (in Pixeln), nur visuell
            const int RADIUS_HIT = 15; // nicht nach unten hin!

            // Zielkoordinaten als (Map-Koordinaten!)
            unsigned short dest_map_x, dest_map_y;

            if(hit)
            {
                // Soll getroffen werden --> Aufschlagskoordinaten gleich dem eigentlichem Ziel
                dest_map_x = target.x;
                dest_map_y = target.y;
            }
            else
            {
                // Ansonsten zufälligen Punkt rundrum heraussuchen
                unsigned d = Random::inst().Rand(__FILE__, __LINE__, obj_id, 6);

                dest_map_x = gwg->GetXA(target.x, target.y, d);
                dest_map_y = gwg->GetYA(target.x, target.y, d);
            }



            unsigned char shooting_dir = (7 + wheel_steps) % 6;

            // Größe der Welt in Pixeln bestimmen
            int world_width = gwg->GetWidth() * TR_W;
            int world_height = gwg->GetHeight() * TR_H;

            // Startpunkt bestimmen
            int start_x = int(gwg->GetTerrainX(x, y)) + STONE_STARTS[(7 + wheel_steps) % 6 * 2];
            int start_y = int(gwg->GetTerrainY(x, y)) + STONE_STARTS[shooting_dir * 2 + 1];
            // (Visuellen) Aufschlagpunkt bestimmen
            int dest_x = int(gwg->GetTerrainX(dest_map_x, dest_map_y));
            int dest_y = int(gwg->GetTerrainY(dest_map_x, dest_map_y));

            // Kartenränder beachten
            // Wenn Abstand kleiner is, den kürzeren Abstand über den Kartenrand wählen
            if(abs(start_x + world_width - dest_x) < abs(start_x - dest_x))
                start_x += world_width;
            else if(abs(start_x - world_width - dest_x) < abs(start_x - dest_x))
                start_x -= world_width;
            if(abs(start_y + world_height - dest_y) < abs(start_y - dest_y))
                start_y += world_height;
            else if(abs(start_y - world_height - dest_y) < abs(start_y - dest_y))
                start_y -= world_height;

            // Bei getroffenen den Aufschlagspunkt am Gebäude ein bisschen variieren
            if(hit)
            {
                dest_x += (Random::inst().Rand(__FILE__, __LINE__, obj_id, RADIUS_HIT * 2) - RADIUS_HIT);
                // hier nicht nach unten gehen, da die Tür (also Nullpunkt
                // ja schon ziemlich weit unten ist!
                dest_y -= Random::inst().Rand(__FILE__, __LINE__, obj_id, RADIUS_HIT);
            }

            // Stein erzeugen
            gwg->AddCatapultStone(new CatapultStone(target.x, target.y, dest_map_x, dest_map_y,
                                                    start_x, start_y,
                                                    dest_x, dest_y,
                                                    80));

            // Katapult wieder in Ausgangslage zurückdrehen
            current_ev = em->AddEvent(this, 15 * (abs(wheel_steps) + 3), 1);

            state = STATE_CATAPULT_BACKOFF;
        } break;
        case STATE_CATAPULT_BACKOFF:
        {
            current_ev = 0;
            // wir arbeiten nicht mehr
            workplace->is_working = false;
            // Wieder versuchen, zu arbeiten
            TryToWork();

        } break;
    }
}

void nofCatapultMan::WorkArborted()
{
}

