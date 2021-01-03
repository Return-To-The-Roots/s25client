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
#include "world/GameWorldGame.h"
#include "nodeObjs/noShipBuildingSite.h"
#include "gameTypes/Direction.h"
#include "gameData/GameConsts.h"
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

    bool operator()(const MapPoint& pt) const { return !world.GetNode(pt).reserved; }
};
} // namespace

void nofShipWright::HandleDerivedEvent(const unsigned /*id*/)
{
    switch(state)
    {
        case State::Waiting1:
        {
            // Herausfinden, was der Schiffsbauer als nächstes bauen soll
            if(static_cast<nobShipYard*>(workplace)->GetMode() == nobShipYard::Mode::Boats)
                // in Handwerksmanier Boote herstellen
                nofWorkman::HandleStateWaiting1();
            else
            {
                // Wege müssen immer von der Flagge aus berechnet werden
                MapPoint flagPos = gwg->GetNeighbour(pos, Direction::SouthEast);
                std::vector<MapPoint> possiblePts =
                  gwg->GetPointsInRadius<-1>(flagPos, SHIPWRIGHT_RADIUS, Identity<MapPoint>(), IsNotReserved(*gwg));

                // Verfügbare Punkte, die geeignete Plätze darstellen würden
                std::vector<MapPoint> available_points;

                // Besitze ich noch ein Schiff, was gebaut werden muss?
                for(const auto& pt : possiblePts)
                {
                    noBase* obj = gwg->GetNode(pt).obj;

                    if(!obj)
                        continue;

                    // Schiff?
                    if(obj->GetGOT() == GO_Type::Shipbuildingsite)
                    {
                        // Platz noch nicht reserviert und gehört das Schiff auch mir?
                        if(!gwg->GetNode(pos).reserved && static_cast<noShipBuildingSite*>(obj)->GetPlayer() == player)
                        {
                            if(gwg->FindHumanPath(flagPos, pt, SHIPWRIGHT_WALKING_DISTANCE))
                                available_points.push_back(pt);
                        }
                    }
                }

                // Kein Schiff im Bau gefunden? Dann Plätzchen für ein neues Schiff suchen
                if(available_points.empty())
                {
                    for(const auto& pt : possiblePts)
                    {
                        // Dieser Punkt geeignet?
                        if(IsPointGood(pt) && gwg->FindHumanPath(flagPos, pt, SHIPWRIGHT_WALKING_DISTANCE))
                            available_points.push_back(pt);
                    }
                }

                // Punkte gefunden?
                if(!available_points.empty())
                {
                    // Einen Punkt zufällig auswählen und dorthin laufen
                    curShipBuildPos =
                      available_points[RANDOM.Rand(__FILE__, __LINE__, GetObjId(), available_points.size())];
                    StartWalkingToShip();
                } else
                {
                    // Nichts zu arbeiten gefunden
                    workplace->StartNotWorking();
                    // Weiter warten, vielleicht gibts ja später wieder mal was
                    current_ev = GetEvMgr().AddEvent(this, JOB_CONSTS[job_].wait1_length, 1);
                }
            }
        }
        break;
        case State::Work:
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

    sgd.PushMapPoint(curShipBuildPos);
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
    gwg->SetReserved(curShipBuildPos, true);
    // Anfangen zu laufen (erstmal aus dem Haus raus!)
    StartWalking(Direction::SouthEast);

    workplace->StopNotWorking();
}

/// Ist ein bestimmter Punkt auf der Karte für den Schiffsbau geeignet
/// poc: differene to original game: points at a sea which cant have a harbor are invalid (original as long as there is
/// 1 harborpoint at any sea on the map any sea is valid)
bool nofShipWright::IsPointGood(const MapPoint pt) const
{
    // Auf Wegen nicht bauen
    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        if(gwg->GetPointRoad(pt, dir) != PointRoad::None)
            return false;
    }

    return (gwg->IsPlayerTerritory(pt) && gwg->IsCoastalPointToSeaWithHarbor(pt)
            && (gwg->GetNO(pt)->GetType() == NodalObjectType::Environment
                || gwg->GetNO(pt)->GetType() == NodalObjectType::Nothing));
}

void nofShipWright::WalkToWorkpoint()
{
    // Sind wir am Ziel angekommen?
    if(pos == curShipBuildPos)
    {
        // Anfangen zu arbeiten
        state = State::Work;
        current_ev = GetEvMgr().AddEvent(this, WORKING_TIME_SHIPS, 1);
        return;
    }
    const auto dir = gwg->FindHumanPath(pos, curShipBuildPos, 20);
    // Weg suchen und gucken ob der Punkt noch in Ordnung ist
    if(!dir || (!IsPointGood(curShipBuildPos) && gwg->GetGOT(curShipBuildPos) != GO_Type::Shipbuildingsite))
    {
        // Punkt freigeben
        gwg->SetReserved(curShipBuildPos, false);
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
    curShipBuildPos = gwg->GetNeighbour(workplace->GetPos(), Direction::SouthEast);

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
    const auto dir = gwg->FindHumanPath(pos, curShipBuildPos, SHIPWRIGHT_WALKING_DISTANCE);
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
    if((state == State::Work && workplace->GetPos() != pos)
       || state == State::WalkToWorkpoint) //&& static_cast<nobShipYard*>(workplace)->GetMode() == nobShipYard::SHIPS)
        gwg->SetReserved(curShipBuildPos, false);
}

/// Der Schiffsbauer hat einen Bauschritt bewältigt und geht wieder zurück zum Haus
void nofShipWright::WorkFinished()
{
    // Befindet sich an dieser Stelle schon ein Schiff oder müssen wir es erst noch hinsetzen?
    if(gwg->GetGOT(pos) != GO_Type::Shipbuildingsite)
    {
        // Ggf Zierobjekte löschen
        auto* obj = gwg->GetSpecObj<noBase>(pos);
        if(obj)
        {
            if(obj->GetType() != NodalObjectType::Environment)
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
    if(this->pos == workplace->GetPos())
        return;

    switch(state)
    {
        default: break;
        case State::Work:
        {
            unsigned id = GAMECLIENT.Interpolate(42, current_ev);
            unsigned graphics_id = ANIMATION[id];
            LOADER.GetPlayerImage("rom_bobs", graphics_id)->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);

            // Steh-Hammer-Sound
            if(graphics_id == 300)
            {
                SOUNDMANAGER.PlayNOSound(78, this, id, 160 - rand() % 60);
                was_sounding = true;
            } else if(graphics_id == 303 || graphics_id == 307)
            {
                SOUNDMANAGER.PlayNOSound(72, this, id - id % 2, 160 - rand() % 60);
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
            DrawWalking(drawPt, LOADER.GetBob("jobs"), 92, false);
        }
        break;
        default: return;
    }
}
