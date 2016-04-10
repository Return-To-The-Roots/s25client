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

#include "defines.h" // IWYU pragma: keep
#include "nofShipWright.h"
#include "buildings/nobShipYard.h"
#include "nodeObjs/noShipBuildingSite.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "GameClient.h"
#include "Random.h"
#include "Loader.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "gameData/GameConsts.h"
#include "gameData/JobConsts.h"
#include "libutil/src/colors.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

nofShipWright::nofShipWright(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(JOB_SHIPWRIGHT, pos, player, workplace), dest(MapPoint::Invalid())
{}

const unsigned SHIPWRIGHT_RADIUS = 8;
const unsigned SHIPWRIGHT_WALKING_DISTANCE = 15;
/// Arbeitszeit des Schiffsbauers beim Bauen von großen Schiffen
const unsigned WORKING_TIME_SHIPS = 70;


struct ShipPoint
{
    MapPoint pos;
    unsigned char first_dir;
    ShipPoint(MapPoint pos, unsigned char firstDir): pos(pos), first_dir(firstDir){}
};

namespace{
    struct IsNotReserved
    {
        const World& world;
        IsNotReserved(const World& world): world(world){}

        bool operator()(const MapPoint& pt) const
        {
            return !world.GetNode(pt).reserved;
        }
    };
}

void nofShipWright::HandleDerivedEvent(const unsigned int  /*id*/)
{
    switch(state)
    {
        case STATE_WAITING1:
        {
            // Herausfinden, was der Schiffsbauer als nächstes bauen soll
            if(dynamic_cast<nobShipYard*>(workplace)->GetMode() == nobShipYard::BOATS)
                // in Handwerksmanier Boote herstellen
                nofWorkman::HandleStateWaiting1();
            else
            {
                // Wege müssen immer von der Flagge aus berechnet werden
                MapPoint flagPos = gwg->GetNeighbour(pos, 4);
                std::vector<MapPoint> possiblePts = gwg->GetPointsInRadius<0>(flagPos, SHIPWRIGHT_RADIUS, Identity<MapPoint>(), IsNotReserved(*gwg));

                // Verfügbare Punkte, die geeignete Plätze darstellen würden
                std::vector<ShipPoint> available_points;

                // Besitze ich noch ein Schiff, was gebaut werden muss?
                for(std::vector<MapPoint>::const_iterator it = possiblePts.begin(); it != possiblePts.end(); ++it)
                {
                    noBase* obj = gwg->GetNode(*it).obj;

                    if(!obj)
                        continue;

                    // Schiff?
                    if(obj->GetGOT() == GOT_SHIPBUILDINGSITE)
                    {
                        // Platz noch nicht reserviert und gehört das Schiff auch mir?
                        unsigned char first_dir = INVALID_DIR;
                        if(!gwg->GetNode(pos).reserved &&
                                static_cast<noShipBuildingSite*>(obj)->GetPlayer() == player &&
                                (first_dir = gwg->FindHumanPath(flagPos, *it, SHIPWRIGHT_WALKING_DISTANCE)) != INVALID_DIR)
                        {
                            available_points.push_back(ShipPoint(*it, first_dir));
                        }
                    }
                }

                // Kein Schiff im Bau gefunden? Dann Plätzchen für ein neues Schiff suchen
                if(available_points.empty())
                {
                    for(std::vector<MapPoint>::const_iterator it = possiblePts.begin(); it != possiblePts.end(); ++it)
                    {
                        // Dieser Punkt geeignet?
                        if(IsPointGood(*it))
                        {
                            // Weg dorthin finden
                            unsigned char first_dir = gwg->FindHumanPath(flagPos, *it, SHIPWRIGHT_WALKING_DISTANCE);
                            if(first_dir != 0xFF)
                            {
                                available_points.push_back(ShipPoint(*it, first_dir));
                            }
                        }
                    }
                }

                // Punkte gefunden?
                if(!available_points.empty())
                {
                    // Einen Punkt zufällig auswählen und dorthin laufen
                    ShipPoint p = available_points[RANDOM.Rand(__FILE__, __LINE__, GetObjId(), available_points.size())];
                    dest = p.pos;
                    StartWalkingToShip(p.first_dir);
                }
                else
                {
                    // Nichts zu arbeiten gefunden
                    StartNotWorking();
                    // Weiter warten, vielleicht gibts ja später wieder mal was
                    current_ev = em->AddEvent(this, JOB_CONSTS[job_].wait1_length, 1);
                }
            }
        } break;
        case STATE_WORK:
        {
            // Sind wir an unserem Arbeitsplatz (dem Gebäude), wenn wir die Arbeit beendet haben, bauen wir nur Boote,
            // ansonsten sind wir an unserem Schiff und bauen große Schiffe
            if(workplace->GetPos() == pos)
                // Boote bauen
                nofWorkman::HandleStateWork();
            else
            {
                // fertig mit Arbeiten --> dann müssen die "Folgen des Arbeitens" ausgeführt werden
                WorkFinished();
                // Objekt wieder freigeben
                gwg->SetReserved(pos, false);
                // Wieder nach Hause gehen
                StartWalkingHome();

                // Evtl. Sounds löschen
                if(was_sounding)
                {
                    SOUNDMANAGER.WorkingFinished(this);
                    was_sounding = false;
                }

            }
        } break;
        case STATE_WAITING2:
        {
            // Hier ist die Sache klar, dieser State kann nur bei Handwerkern vorkommen
            nofWorkman::HandleStateWaiting2();
        } break;
        default:
            break;
    }
}

nofShipWright::nofShipWright(SerializedGameData& sgd, const unsigned obj_id)
    : nofWorkman(sgd, obj_id),
      dest(sgd.PopMapPoint())
{
}

void nofShipWright::Serialize(SerializedGameData& sgd) const
{
    nofWorkman::Serialize(sgd);

    sgd.PushMapPoint(dest);
}


/// Startet das Laufen zu der Arbeitsstelle, dem Schiff
void nofShipWright::StartWalkingToShip(const unsigned char  /*first_dir*/)
{
    state = STATE_WALKTOWORKPOINT;
    // Wir arbeiten jetzt
    workplace->is_working = true;
    // Waren verbrauchen
    workplace->ConsumeWares();
    // Punkt für uns reservieren
    gwg->SetReserved(dest, true);
    // Anfangen zu laufen (erstmal aus dem Haus raus!)
    StartWalking(4);

    StopNotWorking();
}

/// Ist ein bestimmter Punkt auf der Karte für den Schiffsbau geeignet
/// poc: differene to original game: points at a sea which cant have a harbor are invalid (original as long as there is 1 harborpoint at any sea on the map any sea is valid)
bool nofShipWright::IsPointGood(const MapPoint pt) const
{
    // Auf Wegen nicht bauen
    for(unsigned i = 0; i < 6; ++i)
    {
        if(gwg->GetPointRoad(pt, i))
            return false;
    }

    return (gwg->IsPlayerTerritory(pt) &&
            gwg->IsCoastalPointToSeaWithHarbor(pt) && (gwg->GetNO(pt)->GetType() == NOP_ENVIRONMENT
                    || gwg->GetNO(pt)->GetType() == NOP_NOTHING));
}



void nofShipWright::WalkToWorkpoint()
{
    // Sind wir am Ziel angekommen?
    if(pos == dest)
    {
        // Anfangen zu arbeiten
        state = STATE_WORK;
        current_ev = em->AddEvent(this, WORKING_TIME_SHIPS, 1);
        return;
    }
    unsigned char dir = gwg->FindHumanPath(pos, dest, 20);
    // Weg suchen und gucken ob der Punkt noch in Ordnung ist
    if(dir == 0xFF || (!IsPointGood(dest) && gwg->GetGOT(dest) != GOT_SHIPBUILDINGSITE))
    {
        // Punkt freigeben
        gwg->SetReserved(dest, false);
        // Kein Weg führt mehr zum Ziel oder Punkt ist nich mehr in Ordnung --> wieder nach Hause gehen
        StartWalkingHome();
    }
    else
    {
        // Alles ok, wir können hinlaufen
        StartWalking(dir);
    }
}

void nofShipWright::StartWalkingHome()
{
    state = STATE_WALKINGHOME;
    // Fahne vor dem Gebäude anpeilen
    dest = gwg->GetNeighbour(workplace->GetPos(), 4);

    // Zu Laufen anfangen
    WalkHome();
}

void nofShipWright::WalkHome()
{
    // Sind wir zu Hause angekommen? (genauer an der Flagge !!)
    if(pos == dest)
    {
        // Weiteres übernimmt nofBuildingWorker
        WorkingReady();
        return;
    }
    unsigned char dir = gwg->FindHumanPath(pos, dest, SHIPWRIGHT_WALKING_DISTANCE);
    // Weg suchen und ob wir überhaupt noch nach Hause kommen
    if(dir == 0xFF)
    {
        // Kein Weg führt mehr nach Hause--> Rumirren
        AbrogateWorkplace();
        StartWandering();
        Wander();
    }
    else
    {
        // Alles ok, wir können hinlaufen
        StartWalking(dir);
    }
}


void nofShipWright::WorkAborted()
{
    // Platz freigeben, falls man gerade arbeitet
    if((state == STATE_WORK && workplace->GetPos() != pos) || state == STATE_WALKTOWORKPOINT) //&& static_cast<nobShipYard*>(workplace)->GetMode() == nobShipYard::SHIPS)
        gwg->SetReserved(dest, true);
}


/// Der Schiffsbauer hat einen Bauschritt bewältigt und geht wieder zurück zum Haus
void nofShipWright::WorkFinished()
{
    // Befindet sich an dieser Stelle schon ein Schiff oder müssen wir es erst noch hinsetzen?
    if(gwg->GetGOT(pos) != GOT_SHIPBUILDINGSITE)
    {
        // Ggf Zierobjekte löschen
        noBase* obj = gwg->GetSpecObj<noBase>(pos);
        if(obj)
        {
            if(obj->GetType() != NOP_ENVIRONMENT)
                // Mittlerweile wurde anderes Objekt hierhin gesetzt --> können kein Schiff mehr bauen
                return;

            gwg->DestroyNO(pos);
        }

        // Baustelle setzen
        gwg->SetNO(pos, new noShipBuildingSite(pos, player));
        // Bauplätze drumrum neu berechnen
        gwg->RecalcBQAroundPointBig(pos);
    }

    // Schiff weiterbauen
    gwg->GetSpecObj<noShipBuildingSite>(pos)->MakeBuildStep();
}


void nofShipWright::WalkedDerived()
{
    switch(state)
    {
        case STATE_WALKTOWORKPOINT: WalkToWorkpoint(); break;
        case STATE_WALKINGHOME: WalkHome(); break;
        default:
            break;
    }
}

const unsigned ANIMATION[42] =
{
    299, 300, 301, 302,
    299, 300, 301, 302,
    299, 300, 301, 302,
    303, 303, 304, 304, 305, 305, 306, 306, 307, 307,
    299, 300, 301, 302,
    299, 300, 301, 302,
    308, 309, 310, 311, 312, 313,
    308, 309, 310, 311, 312, 313
};

void nofShipWright::DrawWorking(int x, int y)
{
    // Nicht mich zeichnen wenn ich im Haus arbeite
    if(this->pos == workplace->GetPos())
        return;

    switch(state)
    {
        default:
            break;
        case STATE_WORK:
        {
            unsigned id = GAMECLIENT.Interpolate(42, current_ev);
            unsigned graphics_id = ANIMATION[id];
            LOADER.GetPlayerImage("rom_bobs", graphics_id)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, gwg->GetPlayer(player).color);

            // Steh-Hammer-Sound
            if(graphics_id == 300)
            {
                SOUNDMANAGER.PlayNOSound(78, this, id, 160 - rand() % 60);
                was_sounding = true;
            }
            else if(graphics_id == 303 || graphics_id == 307)
            {
                SOUNDMANAGER.PlayNOSound(72, this, id - id % 2, 160 - rand() % 60);
                was_sounding = true;
            }

        } break;
    }
}


/// Zeichnen der Figur in sonstigen Arbeitslagen
void nofShipWright::DrawOtherStates(const int x, const int y)
{
    switch(state)
    {
        case STATE_WALKTOWORKPOINT:
        {
            // Schiffsbauer mit Brett zeichnen
            DrawWalking(x, y, LOADER.GetBobN("jobs"), 92, false);
        } break;
        default: return;
    }

}
