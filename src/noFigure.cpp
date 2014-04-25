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
#include "noFigure.h"

#include "GameWorld.h"
#include "Loader.h"
#include "noRoadNode.h"
#include "EventManager.h"
#include "MapGeometry.h"

#include "nobBaseWarehouse.h"
#include "DoorConsts.h"
#include "macros.h"
#include "GameClient.h"
#include "GameClientPlayer.h"
#include "noBuildingSite.h"

#include "nobUsual.h"
#include "nofBuilder.h"
#include "nofCarpenter.h"
#include "nofArmorer.h"
#include "nofStonemason.h"
#include "nofBrewer.h"
#include "nofMinter.h"
#include "nofButcher.h"
#include "nofIronfounder.h"
#include "nofMiller.h"
#include "nofMetalworker.h"
#include "nofBaker.h"
#include "nofWellguy.h"
#include "nofGeologist.h"
#include "nofMiner.h"
#include "nofFarmer.h"
#include "nofForester.h"
#include "nofWoodcutter.h"
#include "nofPigbreeder.h"
#include "nofDonkeybreeder.h"
#include "nofHunter.h"
#include "nofFisher.h"
#include "noSkeleton.h"
#include "nofPassiveSoldier.h"
#include "nofCarrier.h"
#include "nofShipWright.h"
#include "nofCatapultMan.h"
#include "nofPlaner.h"
#include "nofScout_LookoutTower.h"
#include "nofScout_Free.h"
#include "nofPassiveWorker.h"
#include "nofCharburner.h"

#include "nobHarborBuilding.h"

#include "glSmartBitmap.h"

#include "Swap.h"
#include "Random.h"

#include "SerializedGameData.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


const RoadSegment noFigure::emulated_wanderroad(RoadSegment::RT_NORMAL, 0, 0, std::vector<unsigned char>(0, 0));
/// Welche Strecke soll minimal und maximal zurückgelegt werden beim Rumirren, bevor eine Flagge gesucht wird
const unsigned short WANDER_WAY_MIN = 20;
const unsigned short WANDER_WAY_MAX = 40;
/// Versuche, eine Flagge zu finden, bis er stirbt beim Rumirren
const unsigned short WANDER_TRYINGS = 3;
// GröÃƒÂŸe des Rechtecks um den Punkt, wo er die Flaggen sucht beim Rumirren
const unsigned short WANDER_RADIUS = 10;
/// Dasselbe nochmal für Soldaten
const unsigned short WANDER_TRYINGS_SOLDIERS = 6;
const unsigned short WANDER_RADIUS_SOLDIERS = 15;




noFigure::noFigure(const Job job, const unsigned short x, const unsigned short y, const unsigned char player, noRoadNode* const goal)
    :   noMovable(NOP_FIGURE, x, y), fs(FS_GOTOGOAL), job(job), player(player), cur_rs(0),
        rs_pos(0), rs_dir(0), on_ship(false), goal(goal), waiting_for_free_node(false),
        flag_x(0xFFFF), flag_y(0xFFFF), last_id(0xFFFFFFFF)

{
    //if(GetVisualRange())
    //  gwg->SetVisibilitiesAroundPoint(x,y,GetVisualRange(),player);

    // Haben wir ein Ziel?
    // Gehen wir in ein Lagerhaus? Dann dürfen wir da nicht unsere Arbeit ausführen, sondern
    // gehen quasi nach Hause von Anfang an aus
    if(goal && (goal->GetGOT() == GOT_NOB_HARBORBUILDING || goal->GetGOT() == GOT_NOB_STOREHOUSE
                || goal->GetGOT() == GOT_NOB_HQ))
        fs = FS_GOHOME;

}

noFigure::noFigure(const Job job, const unsigned short x, const unsigned short y, const unsigned char player)
    :   noMovable(NOP_FIGURE, x, y), fs(FS_JOB), job(job), player(player), cur_rs(0),
        rs_pos(0), rs_dir(0), on_ship(false), goal(0), waiting_for_free_node(false), last_id(0xFFFFFFFF)
{
    //f(GetVisualRange())
    //  gwg->SetVisibilitiesAroundPoint(x,y,GetVisualRange(),player);
}

void noFigure::Destroy_noFigure()
{
    Destroy_noMovable();

    assert(players->getElement(player)->CheckDependentFigure(this) == false);
}

void noFigure::Serialize_noFigure(SerializedGameData* sgd) const
{
    Serialize_noMovable(sgd);

    sgd->PushUnsignedChar(static_cast<unsigned char>(fs));
    sgd->PushUnsignedChar(static_cast<unsigned char>(job));
    sgd->PushUnsignedChar(player);
    sgd->PushObject(cur_rs, true);
    sgd->PushUnsignedShort(rs_pos);
    sgd->PushBool(rs_dir);
    sgd->PushBool(on_ship);

    if(fs == FS_GOTOGOAL || fs == FS_GOHOME)
        sgd->PushObject(goal, false);

    sgd->PushBool(waiting_for_free_node);

    if(fs == FS_WANDER)
    {
        sgd->PushUnsignedShort(wander_way);
        sgd->PushUnsignedShort(wander_tryings);
        sgd->PushUnsignedShort(flag_x);
        sgd->PushUnsignedShort(flag_y);
        sgd->PushUnsignedInt(flag_obj_id);
        sgd->PushUnsignedInt(burned_wh_id);
    }
}

noFigure::noFigure(SerializedGameData* sgd, const unsigned obj_id) : noMovable(sgd, obj_id),
    fs(FigureState(sgd->PopUnsignedChar())),
    job(Job(sgd->PopUnsignedChar())),
    player(sgd->PopUnsignedChar()),
    cur_rs(sgd->PopObject<RoadSegment>(GOT_ROADSEGMENT)),
    rs_pos(sgd->PopUnsignedShort()),
    rs_dir(sgd->PopBool()),
    on_ship(sgd->PopBool()),
    last_id(0xFFFFFFFF)
{
    if(fs == FS_GOTOGOAL || fs == FS_GOHOME)
        goal = sgd->PopObject<noRoadNode>(GOT_UNKNOWN);
    else
        goal = 0;

    waiting_for_free_node = sgd->PopBool();

    if(fs == FS_WANDER)
    {
        wander_way = sgd->PopUnsignedShort();
        wander_tryings = sgd->PopUnsignedShort();
        flag_x = sgd->PopUnsignedShort();
        flag_y = sgd->PopUnsignedShort();
        flag_obj_id = sgd->PopUnsignedInt();
        burned_wh_id = sgd->PopUnsignedInt();
    }
}


void noFigure::ActAtFirst()
{
    // Je nach unserem Status bestimmte Dinge tun
    switch(fs)
    {
        default: break;
        case FS_GOTOGOAL: WalkToGoal(); break;
        case FS_JOB: StartWalking(4); break; // erstmal rauslaufen, darum kümmern sich dann die abgeleiteten Klassen
        case FS_GOHOME:
        {
            // Wenn ich gleich wieder nach Hause geschickt wurde und aus einem Lagerhaus rauskomme, gar nicht erst rausgehen!
            if(goal->GetX() == x && goal->GetY() == y)
            {
                gwg->RemoveFigure(this, x, y);
                static_cast<nobBaseWarehouse*>(goal)->AddFigure(this);
            }
            else
                // ansonsten ganz normal rausgehen
                WalkToGoal();
        } break;
        case FS_WANDER: StartWalking(4); break; // erstmal rauslaufen, darum kümmern sich dann die Wander-Funktionen
    }
}


/// Gibt den Sichtradius dieser Figur zurück (0, falls nicht-spähend)
unsigned noFigure::GetVisualRange() const
{
    return 0;
}

/// Legt die Anfangsdaten für das Laufen auf Wegen fest
void noFigure::InitializeRoadWalking(const RoadSegment* const road, const unsigned short rs_pos, const bool rs_dir)
{
    this->cur_rs = road;
    this->rs_pos = rs_pos;
    this->rs_dir = rs_dir;
}

bool noFigure::CalcFigurRelative(int& x, int& y)
{
    int x1 = static_cast<int>(gwg->GetTerrainX(this->x, this->y));
    int y1 = static_cast<int>(gwg->GetTerrainY(this->x, this->y));
    int x2 = static_cast<int>(gwg->GetTerrainX(gwg->GetXA(this->x, this->y, dir), gwg->GetYA(this->x, this->y, dir)));
    int y2 = static_cast<int>(gwg->GetTerrainY(gwg->GetXA(this->x, this->y, dir), gwg->GetYA(this->x, this->y, dir)));


    // Gehen wir über einen Kartenrand (horizontale Richung?)
    if(abs(x1 - x2) >= gwg->GetWidth() * TR_W / 2)
    {
        if(abs(x1 - int(gwg->GetWidth())*TR_W - x2) < abs(x1 - x2))
            x1 -= gwg->GetWidth() * TR_W;
        else
            x1 += gwg->GetWidth() * TR_W;
    }
    // Und dasselbe für vertikale Richtung
    if(abs(y1 - y2) >= gwg->GetHeight() * TR_H / 2)
    {
        if(abs(y1 - int(gwg->GetHeight())*TR_H - y2) < abs(y1 - y2))
            y1 -= gwg->GetHeight() * TR_H;
        else
            y1 += gwg->GetHeight() * TR_H;
    }

    MapCoord xa = gwg->GetXA(this->x, this->y, 1), ya = gwg->GetYA(this->x, this->y, 1);

    if(dir == 1 && (gwg->GetNO(xa, ya)->GetType() == NOP_BUILDINGSITE || gwg->GetNO(xa, ya)->GetType() == NOP_BUILDING))
    {
        x2 += gwg->GetSpecObj<noBaseBuilding>(xa, ya)->GetDoorPointX();
        y2 += gwg->GetSpecObj<noBaseBuilding>(xa, ya)->GetDoorPointY();
        x += gwg->GetSpecObj<noBaseBuilding>(xa, ya)->GetDoorPointX();
        y += gwg->GetSpecObj<noBaseBuilding>(xa, ya)->GetDoorPointY();
    }
    else if(gwg->GetNO(this->x, this->y)->GetType() == NOP_BUILDINGSITE || gwg->GetNO(this->x, this->y)->GetType() == NOP_BUILDING)
    {
        x1 += gwg->GetSpecObj<noBaseBuilding>(this->x, this->y)->GetDoorPointX();
        y1 += gwg->GetSpecObj<noBaseBuilding>(this->x, this->y)->GetDoorPointY();
        x += gwg->GetSpecObj<noBaseBuilding>(this->x, this->y)->GetDoorPointX();
        y += gwg->GetSpecObj<noBaseBuilding>(this->x, this->y)->GetDoorPointY();
    }

    // Wenn die Träger runterlaufne, muss es andersrum sein, da die Träger dann immer vom OBEREN Punkt aus gezeichnet werden
    if(dir == 1 || dir == 2)
    {
        Swap(x1, x2);
        Swap(y1, y2);
    }

    CalcRelative(x, y, x1, y1, x2, y2);

    return 1;
}

void noFigure::StartWalking(const unsigned char dir)
{
    assert(!(GetGOT() == GOT_NOF_PASSIVESOLDIER && fs == FS_JOB));

    assert(dir <= 5);
    if(dir > 5)
    {
        LOG.lprintf("Achtung: Bug im Spiel entdeckt! noFigure::StartWalking: dir = %d\n", unsigned(dir));
        return;
    }

    // Gehen wir in ein Gebäude?
    if(dir == 1 && gwg->GetNO(gwg->GetXA(x, y, 1), gwg->GetYA(x, y, 1))->GetType() == NOP_BUILDING)
        gwg->GetSpecObj<noBuilding>(gwg->GetXA(x, y, 1), gwg->GetYA(x, y, 1))->OpenDoor(); // Dann die Tür aufmachen
    // oder aus einem raus?
    if(dir == 4 && gwg->GetNO(x, y)->GetType() == NOP_BUILDING)
        gwg->GetSpecObj<noBuilding>(x, y)->OpenDoor(); // Dann die Tür aufmachen

    // Ist der Platz schon besetzt, wo wir hinlaufen wollen und laufen wir auf StraÃƒÂŸen?
    if(!gwg->IsRoadNodeForFigures(gwg->GetXA(x, y, dir), gwg->GetYA(x, y, dir), dir) &&
            cur_rs)
    {
        // Dann stehen bleiben!
        this->dir = dir;
        waiting_for_free_node = true;
        // Andere Figuren stoppen
        gwg->StopOnRoads(x, y, dir);
    }
    else
    {
        // Normal hinlaufen
        StartMoving(dir, 20);
    }
}

/*void noFigure::StartWalkingFailedTrade(const unsigned char dir)
{
    assert(!(GetGOT() == GOT_NOF_PASSIVESOLDIER && fs == FS_JOB));

    assert(dir <= 5);
    if(dir > 5)
    {
        LOG.lprintf("Achtung: Bug im Spiel entdeckt! noFigure::StartWalking: dir = %d\n", unsigned(dir));
        return;
    }

    // Gehen wir in ein Gebäude?
    if(dir == 1 && gwg->GetNO(gwg->GetXA(x,y,1),gwg->GetYA(x,y,1))->GetType() == NOP_BUILDING)
        gwg->GetSpecObj<noBuilding>(gwg->GetXA(x,y,1),gwg->GetYA(x,y,1))->OpenDoor(); // Dann die Tür aufmachen
    // oder aus einem raus?
    if(dir == 4 && gwg->GetNO(x,y)->GetType() == NOP_BUILDING)
        gwg->GetSpecObj<noBuilding>(x,y)->OpenDoor(); // Dann die Tür aufmachen

    // Ist der Platz schon besetzt, wo wir hinlaufen wollen und laufen wir auf StraÃƒÂŸen?
    if(!gwg->IsRoadNodeForFigures(gwg->GetXA(x,y,dir),gwg->GetYA(x,y,dir),dir) &&
        cur_rs)
    {
        // Dann stehen bleiben!
        this->dir = dir;
        waiting_for_free_node = true;
        // Andere Figuren stoppen
        gwg->StopOnRoads(x,y,dir);
    }
    else
    {
        // Normal hinlaufen
        StartMoving(dir,20);
    }
}*/



void noFigure::DrawShadow(const int x, const int y, const unsigned char anistep, unsigned char dir)
{
    glArchivItem_Bitmap* bitmap = LOADER.GetMapImageN(900 + ( (dir + 3) % 6 ) * 8 + anistep);
    if(bitmap)
        bitmap->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_SHADOW);
}

void noFigure::WalkFigure()
{
    // Tür hinter sich zumachen, wenn wir aus einem Gebäude kommen
    if(dir == 4 && gwg->GetNO(x, y)->GetType() == NOP_BUILDING)
        gwg->GetSpecObj<noBuilding>(x, y)->CloseDoor();

    Walk();

    if(cur_rs)
        ++rs_pos;


    // oder in eins reingegangen sind
    if(dir == 1 && gwg->GetNO(x, y)->GetType() == NOP_BUILDING)
        gwg->GetSpecObj<noBuilding>(x, y)->CloseDoor();

}


void noFigure::WalkToGoal()
{
    // Kein Ziel mehr --> Rumirren
    if(!goal)
    {
        StartWandering();
        Wander();
        return;
    }

    // StraÃƒÂŸe abgelaufen oder noch gar keine StraÃƒÂŸe vorhanden?
    if(((cur_rs) ? (rs_pos == cur_rs->GetLength()) : true))
    {
        // Ziel erreicht?
        // Bei dem Träger können das beide Flaggen sein!
        unsigned short goal_x1, goal_y1, goal_x2 = 0xFFFF, goal_y2 = 0xFFFF;
        if(GetGOT() == GOT_NOF_CARRIER && fs == FS_GOTOGOAL)
        {
            goal_x1 = static_cast<nofCarrier*>(this)->GetFirstFlag() ?
                      static_cast<nofCarrier*>(this)->GetFirstFlag()->GetX() : 0xFFFF;
            goal_y1 = static_cast<nofCarrier*>(this)->GetFirstFlag() ?
                      static_cast<nofCarrier*>(this)->GetFirstFlag()->GetY() : 0xFFFF;
            goal_x2 = static_cast<nofCarrier*>(this)->GetSecondFlag() ?
                      static_cast<nofCarrier*>(this)->GetSecondFlag()->GetX() : 0xFFFF;
            goal_y2 = static_cast<nofCarrier*>(this)->GetSecondFlag() ?
                      static_cast<nofCarrier*>(this)->GetSecondFlag()->GetY() : 0xFFFF;
        }
        else
        {
            goal_x1 = goal->GetX();
            goal_y1 = goal->GetY();
        }

        if((goal_x1 == x && goal_y1 == y) || (goal_x2 == x && goal_y2 == y))
        {
            if(fs == FS_GOHOME)
            {
                // Mann im Lagerhaus angekommen
                gwg->RemoveFigure(this, x, y);
                static_cast<nobBaseWarehouse*>(goal)->AddFigure(this);
            }
            else
            {
                // Zeug nullen
                cur_rs = NULL;
                rs_dir = 0;
                rs_pos = 0;
                goal = NULL;


                // abgeleiteter Klasse sagen, dass das Ziel erreicht wurde
                fs = FS_JOB;
                GoalReached();
            }

        }
        else
        {
            Point<MapCoord> next_harbor;
            // Neuen Weg berechnen
            unsigned char route = gwg->FindHumanPathOnRoads(gwg->GetSpecObj<noRoadNode>(x, y), goal, NULL, &next_harbor);
            // Kein Weg zum Ziel... nächstes Lagerhaus suchen
            if(route == 0xFF)
            {
                // Arbeisplatz oder Laghaus Bescheid sagen
                Abrogate();
                // Wir gehen jetzt nach Hause
                GoHome();
                // Evtl wurde kein Lagerhaus gefunden und wir sollen rumirren, dann tun wir das gleich
                if(fs == FS_WANDER)
                {
                    Wander();
                    return;
                }

                // Nach Hause laufen...
                WalkToGoal();
                return;
            }
            // Oder müssen wir das Schiff nehmen?
            else if(route == SHIP_DIR)
            {
                // Uns in den Hafen einquartieren
                noBase* nob;
                if((nob = gwg->GetNO(x, y))->GetGOT() != GOT_NOB_HARBORBUILDING)
                {
                    // Es gibt keinen Hafen mehr -> nach Hause gehen

                    // Arbeisplatz oder Laghaus Bescheid sagen
                    Abrogate();
                    // Wir gehen jetzt nach Hause
                    GoHome();
                    // Evtl wurde kein Lagerhaus gefunden und wir sollen rumirren, dann tun wir das gleich
                    if(fs == FS_WANDER)
                    {
                        Wander();
                        return;
                    }

                    // Nach Hause laufen...
                    WalkToGoal();
                    return;
                }

                // Uns in den Hafen einquartieren
                cur_rs = NULL; // wir laufen nicht mehr auf einer StraÃƒÂŸe
                gwg->RemoveFigure(this, x, y);
                static_cast<nobHarborBuilding*>(nob)->AddFigureForShip(this, next_harbor);

                return;
            }


            // Nächste StraÃƒÂŸe wollen, auf der man geht
            cur_rs = gwg->GetSpecObj<noRoadNode>(x, y)->routes[route];
            StartWalking(route);
            rs_pos = 0;
            rs_dir = (gwg->GetSpecObj<noRoadNode>(x, y) == cur_rs->GetF1()) ? false : true;
        }

    }
    else
    {
        StartWalking(cur_rs->GetDir(rs_dir, rs_pos));
    }
}

/*void noFigure::WalkToGoalFailedTrade()
{
    // Kein Ziel mehr --> Rumirren
    if(!goal)
    {
        StartWanderingFailedTrade();
        WanderFailedTrade();
        return;
    }

    // StraÃƒÂŸe abgelaufen oder noch gar keine StraÃƒÂŸe vorhanden?
    if(((cur_rs)?(rs_pos == cur_rs->GetLength()):true))
    {
        // Ziel erreicht?
        // Bei dem Träger können das beide Flaggen sein!
        unsigned short goal_x1, goal_y1, goal_x2=0xFFFF, goal_y2=0xFFFF;
        if(GetGOT() == GOT_NOF_CARRIER && fs == FS_GOTOGOAL)
        {
            goal_x1 = static_cast<nofCarrier*>(this)->GetFirstFlag() ?
                static_cast<nofCarrier*>(this)->GetFirstFlag()->GetX() : 0xFFFF;
            goal_y1 = static_cast<nofCarrier*>(this)->GetFirstFlag() ?
                static_cast<nofCarrier*>(this)->GetFirstFlag()->GetY() : 0xFFFF;
            goal_x2 = static_cast<nofCarrier*>(this)->GetSecondFlag() ?
                static_cast<nofCarrier*>(this)->GetSecondFlag()->GetX() : 0xFFFF;
            goal_y2 = static_cast<nofCarrier*>(this)->GetSecondFlag() ?
                static_cast<nofCarrier*>(this)->GetSecondFlag()->GetY() : 0xFFFF;
        }
        else
        {
            goal_x1 = goal->GetX();
            goal_y1 = goal->GetY();
        }

        if((goal_x1 == x && goal_y1 == y) || (goal_x2 == x && goal_y2 == y))
        {
            if(fs == FS_GOHOME)
            {
                // Mann im Lagerhaus angekommen     - this is what we hope happens ...
                gwg->GetPlayer(static_cast<nobBaseWarehouse*>(goal)->GetPlayer())->IncreaseInventoryJob(this->GetJobType(),1);
                static_cast<nobBaseWarehouse*>(goal)->AddFigure(this);
                gwg->RemoveFigure(this,x,y);
                // the ware we carried is lost because poc forgot to carry the information and is too lazy to fix it now ...

            }
            else
            {
                // Zeug nullen
                cur_rs = NULL;
                rs_dir = 0;
                rs_pos = 0;
                goal = NULL;


                // abgeleiteter Klasse sagen, dass das Ziel erreicht wurde
                fs = FS_JOB;
                GoalReached();
            }

        }
        else
        {
            Point<MapCoord> next_harbor;
            // Neuen Weg berechnen
            unsigned char route = gwg->FindHumanPathOnRoads(gwg->GetSpecObj<noRoadNode>(x,y),goal,NULL,&next_harbor);
            // Kein Weg zum Ziel... nächstes Lagerhaus suchen
            if(route == 0xFF)
            {
                // Arbeisplatz oder Laghaus Bescheid sagen
                Abrogate();
                // Wir gehen jetzt nach Hause
                GoHome();
                // Evtl wurde kein Lagerhaus gefunden und wir sollen rumirren, dann tun wir das gleich
                if(fs == FS_WANDER)
                {
                    WanderFailedTrade();
                    return;
                }

                // Nach Hause laufen...
                WalkToGoalFailedTrade();
                return;
            }
            // Oder müssen wir das Schiff nehmen? shouldnt happen for lost wanderers
            else if(route == SHIP_DIR)
            {
                // Uns in den Hafen einquartieren
                noBase * nob;
                if((nob=gwg->GetNO(x,y))->GetGOT() != GOT_NOB_HARBORBUILDING)
                {
                    // Es gibt keinen Hafen mehr -> nach Hause gehen

                    // Arbeisplatz oder Laghaus Bescheid sagen
                    Abrogate();
                    // Wir gehen jetzt nach Hause
                    GoHome();
                    // Evtl wurde kein Lagerhaus gefunden und wir sollen rumirren, dann tun wir das gleich
                    if(fs == FS_WANDER)
                    {
                        WanderFailedTrade();
                        return;
                    }

                    // Nach Hause laufen...
                    WalkToGoalFailedTrade();
                    return;
                }

                // Uns in den Hafen einquartieren
                cur_rs = NULL; // wir laufen nicht mehr auf einer StraÃƒÂŸe
                gwg->RemoveFigure(this,x,y);
                static_cast<nobHarborBuilding*>(nob)->AddFigureForShip(this,next_harbor);

                return;
            }


            // Nächste StraÃƒÂŸe wollen, auf der man geht
            cur_rs = gwg->GetSpecObj<noRoadNode>(x,y)->routes[route];
            StartWalking(route);
            rs_pos = 0;
            rs_dir = (gwg->GetSpecObj<noRoadNode>(x,y) == cur_rs->GetF1()) ? false : true;
        }

    }
    else
    {
        StartWalkingFailedTrade(cur_rs->GetDir(rs_dir,rs_pos));
    }
}*/



void noFigure::HandleEvent(const unsigned int id)
{
    // Bei ID = 0 ists ein Laufevent, bei allen anderen an abgeleitete Klassen weiterleiten
    if(id)
    {
        HandleDerivedEvent(id);
    }
    else
    {
        current_ev = 0;
        WalkFigure();

        // Alte Richtung und Position für die Berechnung der Sichtbarkeiten merken
        unsigned char old_dir = dir;

        Point<MapCoord> old_pos(x, y);

        switch(fs)
        {
            case FS_GOHOME:
            case FS_GOTOGOAL:
            {
                WalkToGoal();
            } break;

            case FS_JOB:
            {
                Walked();
                break;
            }
            case FS_WANDER:
            {
                Wander();
                break;
            }
        }

        // Ggf. Sichtbereich testen
        if(GetVisualRange())
        {

            // Use old position (don't use this->x/y because it might be different now
            // Figure could be in a ship etc.)
            gwg->RecalcMovingVisibilities(old_pos.x, old_pos.y, player, GetVisualRange(), old_dir, NULL);


            list<noBase*> figures;
            gwg->GetDynamicObjectsFrom(old_pos.x, old_pos.y, figures);

            // Wenn Figur verschwunden ist, muss ihr ehemaliger gesamter Sichtbereich noch einmal
            // neue berechnet werden
            if(!figures.search(this).valid())
                CalcVisibilities(old_pos.x, old_pos.y);
        }

    }
}

void noFigure::GoHome(noRoadNode* goal)
{
    if(on_ship)
    {
        // Wir befinden uns gerade an Deck, also einfach goal auf Null setzen und dann sehen wir, was so passiert
        this->goal = NULL;
        return;
    }
    // Nächstes Lagerhaus suchen
    else if(goal == NULL)
    {
        // Wenn wir cur_rs == 0, dann hängen wir wahrscheinlich noch im Lagerhaus in der Warteschlange
        if(cur_rs == 0)
        {
            assert(gwg->GetNO(x, y)->GetGOT() == GOT_NOB_HQ ||
                   gwg->GetNO(x, y)->GetGOT() == GOT_NOB_STOREHOUSE
                   || gwg->GetNO(x, y)->GetGOT() == GOT_NOB_HARBORBUILDING);

            gwg->GetSpecObj<nobBaseWarehouse>(x, y)->CancelFigure(this);
            return;
        }
        else
            this->goal = gwg->GetPlayer(player)->FindWarehouse((rs_dir) ? cur_rs->GetF1() : cur_rs->GetF2(), FW::Condition_StoreFigure, 0, true, &job, false);
    }
    else
        this->goal = goal;

    if(this->goal)
    {
        fs = FS_GOHOME;
        // Lagerhaus Bescheid sagen
        static_cast<nobBaseWarehouse*>(this->goal)->AddDependentFigure(this);

        // Wenn wir stehen, zusätzlich noch loslaufen!
        if(waiting_for_free_node)
        {
            waiting_for_free_node = false;
            WalkToGoal();
            // anderen Leuten noch ggf Bescheid sagen
            gwg->RoadNodeAvailable(this->x, this->y);
        }
    }
    else
    {
        // Kein Lagerhaus gefunden --> Rumirren
        StartWandering();
        cur_rs = 0;
    }
}



void noFigure::StartWandering(const unsigned burned_wh_id)
{
    fs = FS_WANDER;
    cur_rs = 0;
    goal = 0;
    rs_pos = 0;
    this->burned_wh_id = burned_wh_id;
    // eine bestimmte Strecke rumirren und dann eine Flagge suchen
    // 3x rumirren und eine Flagge suchen, wenn dann keine gefunden wurde, stirbt die Figur
    wander_way = WANDER_WAY_MIN + RANDOM.Rand(__FILE__, __LINE__, obj_id, WANDER_WAY_MAX - WANDER_WAY_MIN);
    // Soldaten sind härter im Nehmen
    bool is_soldier = (job >= JOB_PRIVATE && job <= JOB_GENERAL);
    wander_tryings = is_soldier ? WANDER_TRYINGS_SOLDIERS : WANDER_TRYINGS;

    // Wenn wir stehen, zusätzlich noch loslaufen!
    if(waiting_for_free_node)
    {
        waiting_for_free_node = false;
        Wander();
    }
}

/*void noFigure::StartWanderingFailedTrade(const unsigned burned_wh_id)
{
    fs = FS_WANDER;
    cur_rs = 0;
    goal = 0;
    rs_pos = 0;
    this->burned_wh_id = burned_wh_id;
    // eine bestimmte Strecke rumirren und dann eine Flagge suchen
    // 3x rumirren und eine Flagge suchen, wenn dann keine gefunden wurde, stirbt die Figur
    wander_way = WANDER_WAY_MIN + RANDOM.Rand(__FILE__,__LINE__,obj_id,WANDER_WAY_MAX-WANDER_WAY_MIN);
    // Soldaten sind härter im Nehmen
    bool is_soldier = (job >= JOB_PRIVATE && job <= JOB_GENERAL);
    wander_tryings = is_soldier ? WANDER_TRYINGS_SOLDIERS : WANDER_TRYINGS;

    // Wenn wir stehen, zusätzlich noch loslaufen!
    if(waiting_for_free_node)
    {
        waiting_for_free_node = false;
        WanderFailedTrade();
    }
}*/

void noFigure::Wander()
{
    // Sind wir noch auf der Suche nach einer Flagge?
    if(wander_way != 0xFFFF)
    {
        // Soldaten sind härter im Nehmen
        bool is_soldier = (job >= JOB_PRIVATE && job <= JOB_GENERAL);
        unsigned short wander_radius = is_soldier ? WANDER_RADIUS_SOLDIERS : WANDER_RADIUS;
        // Ist es mal wieder an der Zeit, eine Flagge zu suchen?
        if(!wander_way)
        {
            // Umgebung abscannen, nicht über den Rand gehen
            unsigned short x1 = (x > wander_radius) ? (x - wander_radius) : 0;
            unsigned short y1 = (y > wander_radius) ? (y - wander_radius) : 0;
            unsigned short x2 = (x + wander_radius < gwg->GetWidth()) ? (x + wander_radius) : (gwg->GetWidth() - 1);
            unsigned short y2 = (y + wander_radius < gwg->GetHeight()) ? (y + wander_radius) : (gwg->GetHeight() - 1);

            // Flaggen sammeln und dann zufällig eine auswählen
            list<noFlag*> flags;

            for(unsigned short py = y1; py <= y2; ++py)
            {
                for(unsigned short px = x1; px <= x2; ++px)
                {
                    if(gwg->GetNO(px, py)->GetType() == NOP_FLAG)
                    {
                        if(gwg->GetSpecObj<noFlag>(px, py)->GetPlayer() == player)
                            flags.push_back(gwg->GetSpecObj<noFlag>(px, py));
                    }
                }
            }


            unsigned best_way = 0xFFFFFFFF;
            noFlag* best_flag = 0;

            for(list<noFlag*>::iterator it = flags.begin(); it.valid(); ++it)
            {
                // Ist das ein Flüchtling aus einem abgebrannten Lagerhaus?
                if(burned_wh_id != 0xFFFFFFFF)
                {
                    // Dann evtl gucken, ob anderen Mitglieder schon gesagt haben, dass die Flagge nicht zugänglich ist
                    if((*it)->IsImpossibleForBWU(burned_wh_id))
                    {
                        //printf("flagge gesiebt\n");
                        // Dann können wir die Flagge überspringen
                        continue;
                    }
                }

                // würde die die bisher beste an Weg unterbieten?
                unsigned way = gwg->CalcDistance(x, y, (*it)->GetX(), (*it)->GetY());
                if(way < best_way)
                {
                    // Gibts nen Weg zu dieser Flagge?
                    if((dir = gwg->FindHumanPath(x, y, (*it)->GetX(), (*it)->GetY(), 10, false)) != 0xFF)
                    {
                        // gucken, ob ein Weg zu einem Warenhaus führt
                        if(gwg->GetPlayer(player)->FindWarehouse(*it, FW::Condition_StoreFigure, 0, true, &job, false))
                        {
                            // dann nehmen wir die doch glatt
                            best_way = way;
                            best_flag = *it;
                        }
                    }
                    else if(burned_wh_id != 0xFFFFFFFF)
                    {
                        // Flagge nicht möglich zugänglich bei einem Flüchting aus einem abgebrannten Lagerhaus?
                        // --> der ganzen Gruppe Bescheid sagen, damit die nicht auch alle sinnlos einen Weg zu
                        // dieser Flagge suchen
                        (*it)->ImpossibleForBWU(burned_wh_id);
                    }
                }
            }

            if(best_flag)
            {
                // bestmögliche schlieÃƒÂŸlich nehmen
                wander_way = 0xFFFF;
                flag_x = best_flag->GetX();
                flag_y = best_flag->GetY();
                flag_obj_id = best_flag->GetObjId();
                WanderToFlag();
                return;
            }



            // Wurde keine Flagge gefunden?

            // Haben wir noch Versuche?
            if(--wander_tryings > 0)
            {
                // von vorne beginnen wieder mit Rumirren
                wander_way = WANDER_WAY_MIN + RANDOM.Rand(__FILE__, __LINE__, obj_id, WANDER_WAY_MAX - WANDER_WAY_MIN);
            }
            else
            {
                // Genug rumgeirrt, wir finden halt einfach nichts --> Sterben
                Die();
                return;
            }
        }

        // weiter umherirren, einfach in eine zufällige Richtung
        // Müssen dabei natürlich aufpassen, dass wir nur dorthin gehen wo es auch für Figuren möglich ist
        unsigned char doffset = RANDOM.Rand(__FILE__, __LINE__, obj_id, 6);
        for(unsigned char d = 0; d < 6; ++d)
        {
            unsigned char dir = (d + doffset) % 6;

            // Nicht über den Rand gehen!
            if(x == 0 && (dir == 0 || dir == 1 || dir == 5)) continue;
            if(y == 0 && (dir == 1 || dir == 2)) continue;
            if(x == gwg->GetWidth() - 1 && (dir == 2 || dir == 3 || dir == 4)) continue;
            if(y == gwg->GetHeight() - 1 && (dir == 4 || dir == 5)) continue;

            if(gwg->IsNodeForFigures(gwg->GetXA(x, y, dir), gwg->GetYA(x, y, dir))
                    && gwg->IsNodeToNodeForFigure(x, y, dir))
            {
                StartWalking(dir);
                --wander_way;
                return;
            }
        }

        // Wir sind eingesperrt! Kein Weg mehr gefunden --> Sterben
        Die();
    }
    // Wir laufen schon zur Flagge
    else
    {
        WanderToFlag();
    }
}

void noFigure::WanderFailedTrade()
{
    // Sind wir noch auf der Suche nach einer Flagge?
    DieFailedTrade();
    return;
    /*
    if(wander_way != 0xFFFF)
    {
        // Soldaten sind härter im Nehmen
        bool is_soldier = (job >= JOB_PRIVATE && job <= JOB_GENERAL);
        unsigned short wander_radius = is_soldier ? WANDER_RADIUS_SOLDIERS : WANDER_RADIUS;
        // Ist es mal wieder an der Zeit, eine Flagge zu suchen?
        if(!wander_way)
        {
            // Umgebung abscannen, nicht über den Rand gehen
            unsigned short x1 = (x > wander_radius) ? (x-wander_radius) : 0;
            unsigned short y1 = (y > wander_radius) ? (y-wander_radius) : 0;
            unsigned short x2 = (x+wander_radius < gwg->GetWidth()) ? (x+wander_radius) : (gwg->GetWidth()-1);
            unsigned short y2 = (y+wander_radius < gwg->GetHeight()) ? (y+wander_radius) : (gwg->GetHeight()-1);

            // Flaggen sammeln und dann zufällig eine auswählen
            list<noFlag*> flags;

            for(unsigned short py = y1;py<=y2;++py)
            {
                for(unsigned short px = x1;px<=x2;++px)
                {
                    if(gwg->GetNO(px,py)->GetType() == NOP_FLAG)
                    {
                        if(gwg->GetSpecObj<noFlag>(px,py)->GetPlayer() == player)
                            flags.push_back(gwg->GetSpecObj<noFlag>(px,py));
                    }
                }
            }


            unsigned best_way = 0xFFFFFFFF;
            noFlag * best_flag = 0;

            for(list<noFlag*>::iterator it = flags.begin();it.valid();++it)
            {
                // Ist das ein Flüchtling aus einem abgebrannten Lagerhaus?
                if(burned_wh_id != 0xFFFFFFFF)
                {
                    // Dann evtl gucken, ob anderen Mitglieder schon gesagt haben, dass die Flagge nicht zugänglich ist
                    if((*it)->IsImpossibleForBWU(burned_wh_id))
                    {
                        //printf("flagge gesiebt\n");
                        // Dann können wir die Flagge überspringen
                        continue;
                    }
                }

                // würde die die bisher beste an Weg unterbieten?
                unsigned way = gwg->CalcDistance(x,y,(*it)->GetX(),(*it)->GetY());
                if(way < best_way)
                {
                    // Gibts nen Weg zu dieser Flagge?
                    if((dir = gwg->FindHumanPath(x,y,(*it)->GetX(),(*it)->GetY(),10,false)) != 0xFF)
                    {
                        // gucken, ob ein Weg zu einem Warenhaus führt
                        if(gwg->GetPlayer(player)->FindWarehouse(*it,FW::Condition_StoreFigure,0,true,&job,false))
                        {
                            // dann nehmen wir die doch glatt
                            best_way = way;
                            best_flag = *it;
                        }
                    }
                    else if(burned_wh_id != 0xFFFFFFFF)
                    {
                        // Flagge nicht möglich zugänglich bei einem Flüchting aus einem abgebrannten Lagerhaus?
                        // --> der ganzen Gruppe Bescheid sagen, damit die nicht auch alle sinnlos einen Weg zu
                        // dieser Flagge suchen
                        (*it)->ImpossibleForBWU(burned_wh_id);
                    }
                }
            }

            if(best_flag)
            {
                // bestmögliche schlieÃƒÂŸlich nehmen
                wander_way = 0xFFFF;
                flag_x = best_flag->GetX();
                flag_y = best_flag->GetY();
                flag_obj_id = best_flag->GetObjId();
                WanderToFlagFailedTrade();
                return;
            }



            // Wurde keine Flagge gefunden?

            // Haben wir noch Versuche?
            if(--wander_tryings > 0)
            {
                // von vorne beginnen wieder mit Rumirren
                wander_way = WANDER_WAY_MIN + RANDOM.Rand(__FILE__,__LINE__,obj_id,WANDER_WAY_MAX-WANDER_WAY_MIN);
            }
            else
            {
                // Genug rumgeirrt, wir finden halt einfach nichts --> Sterben
                Die();
                return;
            }
        }

        // weiter umherirren, einfach in eine zufällige Richtung
        // Müssen dabei natürlich aufpassen, dass wir nur dorthin gehen wo es auch für Figuren möglich ist
        unsigned char doffset = RANDOM.Rand(__FILE__,__LINE__,obj_id,6);
        for(unsigned char d = 0;d<6;++d)
        {
            unsigned char dir = (d+doffset)%6;

            // Nicht über den Rand gehen!
            if(x == 0 && (dir == 0 || dir == 1 || dir == 5)) continue;
            if(y == 0 && (dir == 1 || dir == 2)) continue;
            if(x == gwg->GetWidth()-1 && (dir == 2 || dir == 3 || dir == 4)) continue;
            if(y == gwg->GetHeight()-1 && (dir == 4 || dir == 5)) continue;

            if(gwg->IsNodeForFigures(gwg->GetXA(x,y,dir),gwg->GetYA(x,y,dir))
                && gwg->IsNodeToNodeForFigure(x,y,dir))
            {
                StartWalkingFailedTrade(dir);
                --wander_way;
                return;
            }
        }

        // Wir sind eingesperrt! Kein Weg mehr gefunden --> Sterben
        Die();
    }
    // Wir laufen schon zur Flagge
    else
    {
        WanderToFlagFailedTrade();
    }*/
}

void noFigure::WanderToFlag()
{
    // Existiert die Flagge überhaupt noch?
    noBase* no = gwg->GetNO(flag_x, flag_y);
    if(no->GetObjId() != flag_obj_id)
    {
        // Wenn nicht, wieder normal weiter rumirren
        StartWandering();
        Wander();
        return;
    }

    // Sind wir schon da?
    if(x == flag_x && y == flag_y)
    {
        // Gibts noch nen Weg zu einem Lagerhaus?

        if(nobBaseWarehouse* wh = gwg->GetPlayer(player)->FindWarehouse(
                                      gwg->GetSpecObj<noRoadNode>(x, y), FW::Condition_StoreFigure, 0, true, &job, false))
        {
            // ja, dann können wir ja hingehen
            fs = FS_GOTOGOAL;
            goal = wh;
            cur_rs = 0;
            rs_pos = 0;
            fs = FS_GOHOME;
            wh->AddDependentFigure(this);
            WalkToGoal();
            return;
        }
        else
        {
            // Wenn nicht, wieder normal weiter rumirren
            StartWandering();
            Wander();
            return;
        }
    }

    // Weiter zur Flagge gehen
    // Gibts noch nen Weg dahin bzw. existiert die Flagge noch?
    if((dir = gwg->FindHumanPath(x, y, flag_x, flag_y, 60, false)) != 0xFF)
    {
        // weiter hinlaufen
        StartWalking(dir);
    }
    else
    {
        // Wenn nicht, wieder normal weiter rumirren
        StartWandering();
        Wander();
    }
}

/*void noFigure::WanderToFlagFailedTrade()
{
    // Existiert die Flagge überhaupt noch?
    noBase * no = gwg->GetNO(flag_x,flag_y);
    if(no->GetObjId() != flag_obj_id)
    {
        // Wenn nicht, wieder normal weiter rumirren
        StartWanderingFailedTrade();
        WanderFailedTrade();
        return;
    }

    // Sind wir schon da?
    if(x == flag_x && y == flag_y)
    {
        // Gibts noch nen Weg zu einem Lagerhaus?

        if(nobBaseWarehouse * wh = gwg->GetPlayer(player)->FindWarehouse(
            gwg->GetSpecObj<noRoadNode>(x,y),FW::Condition_StoreFigure,0,true,&job,false))
        {
            // ja, dann können wir ja hingehen
            fs = FS_GOTOGOAL;
            goal = wh;
            cur_rs = 0;
            rs_pos = 0;
            fs = FS_GOHOME;
            //wh->AddDependentFigure(this); we do that when we arrive
            WalkToGoalFailedTrade();
            return;
        }
        else
        {
            // Wenn nicht, wieder normal weiter rumirren
            StartWanderingFailedTrade();
            WanderFailedTrade();
            return;
        }
    }

    // Weiter zur Flagge gehen
    // Gibts noch nen Weg dahin bzw. existiert die Flagge noch?
    if((dir = gwg->FindHumanPath(x,y,flag_x,flag_y,60,false)) != 0xFF)
    {
        // weiter hinlaufen
        StartWalkingFailedTrade(dir);
    }
    else
    {
        // Wenn nicht, wieder normal weiter rumirren
        StartWanderingFailedTrade();
        WanderFailedTrade();
    }
}*/

//void noFigure::Wander()
//{
//  // Durch die Gegend irren und eine Flagge suchen
//
//  // Sind wir grad an einer Flagge?
//  if(gwg->GetNO(x,y)->GetType() == NOP_FLAG)
//  {
//      // Geht ein Weg zu einem Lagerhaus?
//      unsigned length;
//      if(nobBaseWarehouse * wh = gwg->GetPlayer(player)->FindWarehouse(gwg->GetSpecObj<noRoadNode>(x,y),GD_NOTHING,JOB_NOTHING,0,1,&length))
//      {
//          // ja, dann können wir ja hingehen
//          fs = FS_GOTOGOAL;
//          goal = wh;
//          // Vorgaukeln, dass wir ein Stück StraÃƒÂŸe bereits geschafft haben
//          // damit wir mit WalkToGoal weiter bis zum Ziel laufen können
//          cur_rs = &emulated_wanderroad;
//          rs_pos = 0;
//          fs = FS_GOHOME;
//          WalkToGoal();
//          return;
//      }
//
//  }
//
//  // Flagge in der Nähe? (Betonung liegt auf "Nähe" :D )
//  for(unsigned i = 0;i<6;++i)
//  {
//      if(gwg->GetNO(gwg->GetXA(x,y,i),gwg->GetYA(x,y,i))->GetType() == NOP_FLAG)
//      {
//          // ja, eine Flagge in der Nähe, gucken, ob ein Weg zu einem Warenhaus führt
//          unsigned length;
//          if(gwg->GetPlayer(player)->FindWarehouse(gwg->GetSpecObj<noRoadNode>(gwg->GetXA(x,y,i),gwg->GetYA(x,y,i)),GD_NOTHING,JOB_NOTHING,0,1,&length))
//          {
//              // ja, dann gehen wir mal zu der Flagge
//              StartWalking(i);
//              return;
//          }
//
//          // Pech gehabt, weitersuchen...
//      }
//  }
//
//  // Nix gefunden, dann müssen wir halt weiter umherirren, einfach in eine zufälige Richtung
//  // Müssen dabei natrlich aufpassen, dass wir nur dorthin gehen wo es auch für Figuren möglich ist
//  unsigned char doffset = Random(6);
//  for(unsigned char d = 0;d<6;++d)
//  {
//      dir = (d+doffset)%6;
//      if(gwg->IsNodeForFigures(gwg->GetXA(x,y,dir),gwg->GetYA(x,y,dir)))
//      {
//          StartWalking(dir);
//          return;
//      }
//  }
//
//  // Wir sind eingesperrt! Kein Weg mehr gefunden --> Sterben
//  Die();
//}




void noFigure::CorrectSplitData(const RoadSegment* const rs2)
{
    // cur_rs entspricht Teilstück 1 !

    // Wenn man sich auf den ersten Teilstück befindet...
    if((rs_pos < cur_rs->GetLength() && !rs_dir) || (rs_pos > rs2->GetLength() && rs_dir))
    {
        // Nur Position berichtigen
        if(rs_dir)
            rs_pos -= rs2->GetLength();
    }

    // Wenn man auf dem 2. steht, ...
    else if((rs_pos > cur_rs->GetLength() && !rs_dir) || (rs_pos < rs2->GetLength() && rs_dir))
    {
        // Position berichtigen (wenn man in umgekehrter Richtung läuft, beibehalten!)
        if(!rs_dir)
            rs_pos -= cur_rs->GetLength();

        // wir laufen auf dem 2. Teilstück
        cur_rs = rs2;
    }
    else if((rs_pos == cur_rs->GetLength() && !rs_dir) || (rs_pos == rs2->GetLength() && rs_dir))
    {
        // wir stehen genau in der Mitte
        // abhängig von der Richtung machen, in die man gerade läuft
        if(dir == rs2->GetRoute(0))
        {
            // wir laufen auf dem 2. Teilstück
            cur_rs = rs2;
            // und wir sind da noch am Anfang
            rs_pos = 0;
        }
        else if(dir == (cur_rs->GetRoute(cur_rs->GetLength() - 1) + 3) % 6)
        {
            // wir laufen auf dem 1. Teilstück

            // und wir sind da noch am Anfang
            rs_pos = 0;
        }
        else
        {
            // Wahrscheinlich stehen wir
            // dann einfach auf das 2. gehen
            cur_rs = rs2;
            rs_pos = 0;
            rs_dir = 0;
        }
    }

    CorrectSplitData_Derived();
}

/// Wird aufgerufen, wenn die StraÃŸe unter der Figur geteilt wurde (für abgeleitete Klassen)
void noFigure::CorrectSplitData_Derived()
{
}


noFigure* CreateJob(const Job job_id, const unsigned short x, const unsigned short y, const unsigned char player, noRoadNode* const goal)
{
    switch(job_id)
    {
        case JOB_BUILDER:
        {
            if(!goal)
                return new nofBuilder(x, y, player, goal);
            else if(goal->GetGOT() == GOT_NOB_HARBORBUILDING)
                return new nofPassiveWorker(JOB_BUILDER, x, y, player, goal);
            else return new nofBuilder(x, y, player, goal);
        }
        case JOB_PLANER: return new nofPlaner(x, y, player, static_cast<noBuildingSite*>(goal));
        case JOB_CARPENTER: return new nofCarpenter(x, y, player, static_cast<nobUsual*>(goal));
        case JOB_ARMORER: return new nofArmorer(x, y, player, static_cast<nobUsual*>(goal));
        case JOB_STONEMASON: return new nofStonemason(x, y, player, static_cast<nobUsual*>(goal));
        case JOB_BREWER: return new nofBrewer(x, y, player, static_cast<nobUsual*>(goal));
        case JOB_MINTER: return new nofMinter(x, y, player, static_cast<nobUsual*>(goal));
        case JOB_BUTCHER: return new nofButcher(x, y, player, static_cast<nobUsual*>(goal));
        case JOB_IRONFOUNDER: return new nofIronfounder(x, y, player, static_cast<nobUsual*>(goal));
        case JOB_MILLER: return new nofMiller(x, y, player, static_cast<nobUsual*>(goal));
        case JOB_METALWORKER: return new nofMetalworker(x, y, player, static_cast<nobUsual*>(goal));
        case JOB_BAKER: return new nofBaker(x, y, player, static_cast<nobUsual*>(goal));
        case JOB_HELPER:
        {
            // Wenn goal = 0 oder Lagerhaus, dann Auslagern anscheinend und mann kann irgendeinen Typ nehmen
            if(!goal)
                return new nofWellguy(x, y, player, static_cast<nobUsual*>(goal));
            if(goal->GetGOT() == GOT_NOB_STOREHOUSE || goal->GetGOT() == GOT_NOB_HARBORBUILDING
                    || goal->GetGOT() == GOT_NOB_HQ)
                return new nofWellguy(x, y, player, static_cast<nobUsual*>(goal));
            else if(static_cast<nobUsual*>(goal)->GetBuildingType() == BLD_WELL)
                return new nofWellguy(x, y, player, static_cast<nobUsual*>(goal));
            else if(static_cast<nobUsual*>(goal)->GetBuildingType() == BLD_CATAPULT)
                return new nofCatapultMan(x, y, player, static_cast<nobUsual*>(goal));
            else
            {
                assert(false);
                return 0;
            }

        }
        case JOB_GEOLOGIST: return new nofGeologist(x, y, player, static_cast<noFlag*>(goal));
        case JOB_SCOUT:
        {
            // Im Spähturm arbeitet ein anderer Spähter-Typ
            // Wenn goal = 0 oder Lagerhaus, dann Auslagern anscheinend und mann kann irgendeinen Typ nehmen
            if(!goal)
                return new nofScout_LookoutTower(x, y, player, static_cast<nobUsual*>(goal));
            if(goal->GetGOT() == GOT_NOB_HARBORBUILDING || goal->GetGOT() == GOT_NOB_STOREHOUSE || goal->GetGOT                                                                 () == GOT_NOB_HQ)
                return new nofPassiveWorker(JOB_SCOUT, x, y, player, goal);
            // Spähturm / Lagerhaus?
            else if(goal->GetGOT() == GOT_NOB_USUAL || goal->GetGOT() == GOT_NOB_HARBORBUILDING)
                return new nofScout_LookoutTower(x, y, player, static_cast<nobUsual*>(goal));
            else if(goal->GetGOT() == GOT_FLAG)
                return new nofScout_Free(x, y, player, goal);
            else
            {
                assert(false);
                return 0;
            }
        } break;
        case JOB_MINER: return new nofMiner(x, y, player, static_cast<nobUsual*>(goal));
        case JOB_FARMER: return new nofFarmer(x, y, player, static_cast<nobUsual*>(goal));
        case JOB_FORESTER: return new nofForester(x, y, player, static_cast<nobUsual*>(goal));
        case JOB_WOODCUTTER: return new nofWoodcutter(x, y, player, static_cast<nobUsual*>(goal));
        case JOB_PIGBREEDER: return new nofPigbreeder(x, y, player, static_cast<nobUsual*>(goal));
        case JOB_DONKEYBREEDER: return new nofDonkeybreeder(x, y, player, static_cast<nobUsual*>(goal) );
        case JOB_HUNTER: return new nofHunter(x, y, player, static_cast<nobUsual*>(goal));
        case JOB_FISHER: return new nofFisher(x, y, player, static_cast<nobUsual*>(goal));
        case JOB_PRIVATE: case JOB_PRIVATEFIRSTCLASS: case JOB_SERGEANT: case JOB_OFFICER: case JOB_GENERAL:
            return new nofPassiveSoldier(x, y, player, static_cast<nobBaseMilitary*>(goal), static_cast<nobBaseMilitary*>(goal), job_id - JOB_PRIVATE);
        case JOB_PACKDONKEY: return new nofCarrier(nofCarrier::CT_DONKEY, x, y, player, 0, goal);
        case JOB_SHIPWRIGHT: return new nofShipWright(x, y, player, static_cast<nobUsual*>(goal));
        case JOB_CHARBURNER: return new nofCharburner(x, y, player, static_cast<nobUsual*>(goal));
        default: return 0;
    }
}

void noFigure::DrawWalkingBobCarrier(int x, int y, unsigned int ware, bool fat)
{
    // Wenn wir warten auf ein freies Plätzchen, müssen wir den stehend zeichnen!
    unsigned ani_step = waiting_for_free_node ? 2 : GAMECLIENT.Interpolate(ASCENT_ANIMATION_STEPS[ascent], current_ev) % 8;

    // Wenn man wartet, stehend zeichnen, es sei denn man wartet mittem auf dem Weg!
    if(!waiting_for_free_node || pause_walked_gf)
        CalcFigurRelative(x, y);

    Loader::carrier_cache[ware][dir][ani_step][fat].draw(x, y, COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);
}



void noFigure::DrawWalkingBobJobs(int x, int y, unsigned int job)
{
    if ((job == JOB_SCOUT) || ((job >= JOB_PRIVATE) && (job <= JOB_GENERAL)))
    {
        DrawWalking(x, y, LOADER.GetBobN("jobs"), JOB_CONSTS[job].jobs_bob_id + NATION_RTTR_TO_S2[gwg->GetPlayer(player)->nation] * 6, false);
        return;
    }

    // Wenn wir warten auf ein freies Plätzchen, müssen wir den stehend zeichnen!
    unsigned ani_step = waiting_for_free_node ? 2 : GAMECLIENT.Interpolate(ASCENT_ANIMATION_STEPS[ascent], current_ev) % 8;

    // Wenn man wartet, stehend zeichnen, es sei denn man wartet mittem auf dem Weg!
    if(!waiting_for_free_node || pause_walked_gf)
        CalcFigurRelative(x, y);

    Loader::bob_jobs_cache[gwg->GetPlayer(player)->nation][job][dir][ani_step].draw(x, y, 0xFFFFFFFF, COLORS[gwg->GetPlayer(player)->color]);
}


void noFigure::DrawWalking(int x, int y, glArchivItem_Bob* file, unsigned int id, bool fat, bool waitingsoldier)
{
    // Wenn wir warten auf ein freies Plätzchen, müssen wir den stehend zeichnen!
    unsigned ani_step = waiting_for_free_node || waitingsoldier ? 2 : GAMECLIENT.Interpolate(ASCENT_ANIMATION_STEPS[ascent], current_ev) % 8;

    // Wenn man wartet, stehend zeichnen, es sei denn man wartet mittem auf dem Weg!
    if(!waitingsoldier && (!waiting_for_free_node || pause_walked_gf))
        CalcFigurRelative(x, y);
    if(file)
        file->Draw(id, dir, fat, ani_step, x, y, COLORS[gwg->GetPlayer(player)->color]);
    DrawShadow(x, y, ani_step, dir);

    /*char number[256];
    sprintf(number,"%u",obj_id);
    NormalFont->Draw(x,y,number,0,0xFFFF0000);*/
}

/// Zeichnet standardmäßig die Figur, wenn sie läuft aus einem bestimmten normalen LST Archiv
void noFigure::DrawWalking(int x, int y, const char* const file, unsigned int id)
{
    // Wenn wir warten, ani-step 2 benutzen
    unsigned ani_step = waiting_for_free_node ? 2 : GAMECLIENT.Interpolate(ASCENT_ANIMATION_STEPS[ascent], current_ev) % 8;

    // Wenn man wartet, stehend zeichnen, es sei denn man wartet mittem auf dem Weg!
    if(!waiting_for_free_node || pause_walked_gf)
        CalcFigurRelative(x, y);

    LOADER.GetImageN(file, id + ((dir + 3) % 6) * 8 + ani_step)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);
    DrawShadow(x, y, ani_step, dir);
}

void noFigure::DrawWalking(int x, int y)
{
    // Figurentyp unterscheiden
    switch(job)
    {
        case JOB_PACKDONKEY:
        {
            // Wenn wir warten, ani-step 2 benutzen
            unsigned ani_step = waiting_for_free_node ? 2 : GAMECLIENT.Interpolate(ASCENT_ANIMATION_STEPS[ascent], current_ev) % 8;

            // Wenn man wartet, stehend zeichnen, es sei denn man wartet mittem auf dem Weg!
            if(!waiting_for_free_node || pause_walked_gf)
                CalcFigurRelative(x, y);

            // Esel
            LOADER.GetMapImageN(2000 + ((dir + 3) % 6) * 8 + ani_step)->Draw(x, y);
            // Schatten des Esels
            LOADER.GetMapImageN(2048 + dir % 3)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_SHADOW);
        } return;
        case JOB_CHARBURNER:
        {
            DrawWalking(x, y, "charburner_bobs", 53);
        } return;
        default:
        {
            DrawWalkingBobJobs(x, y, job);
        } return;
    }


}

void noFigure::Die()
{
    // Weg mit mir
    gwg->RemoveFigure(this, x, y);
    em->AddToKillList(this);
    // ggf. Leiche hinlegen, falls da nix ist
    if(!gwg->GetSpecObj<noBase>(x, y))
        gwg->SetNO(new noSkeleton(x, y), x, y);

    // Wars ein Bootmann? Dann Boot und Träger abziehen
    if(job == JOB_BOATCARRIER)
    {
        gwg->GetPlayer(player)->DecreaseInventoryJob(JOB_HELPER, 1);
        gwg->GetPlayer(player)->DecreaseInventoryWare(GD_BOAT, 1);
    }
    else
        gwg->GetPlayer(player)->DecreaseInventoryJob(job, 1);

    // Sichtbarkeiten neu berechnen für Erkunder und Soldaten
    CalcVisibilities(x, y);
}
void noFigure::DieFailedTrade()
{
    // Weg mit mir
    gwg->RemoveFigure(this, x, y);
    em->AddToKillList(this);
    // ggf. Leiche hinlegen, falls da nix ist
    if(!gwg->GetSpecObj<noBase>(x, y))
        gwg->SetNO(new noSkeleton(x, y), x, y);
    // Sichtbarkeiten neu berechnen für Erkunder und Soldaten
    //CalcVisibilities(x,y);
}

void noFigure::NodeFreed(const unsigned short x, const unsigned short y)
{
    // Stehen wir gerade aus diesem Grund?
    if(waiting_for_free_node)
    {
        // Ist das der Punkt, zu dem wir hin wollen?
        if(x == gwg->GetXA(this->x, this->y, dir) && y == gwg->GetYA(this->x, this->y, dir))
        {


            // Gehen wir in ein Gebäude? Dann wieder ausgleichen, weil wir die Türen sonst doppelt aufmachen!
            if(dir == 1 && gwg->GetNO(gwg->GetXA(this->x, this->y, 1), gwg->GetYA(this->x, this->y, 1))->GetType() == NOP_BUILDING)
                gwg->GetSpecObj<noBuilding>(gwg->GetXA(this->x, this->y, 1), gwg->GetYA(this->x, this->y, 1))->CloseDoor();
            // oder aus einem raus?
            if(dir == 4 && gwg->GetNO(this->x, this->y)->GetType() == NOP_BUILDING)
                gwg->GetSpecObj<noBuilding>(this->x, this->y)->CloseDoor();

            // Wir stehen nun nicht mehr
            waiting_for_free_node = false;

            // Dann loslaufen
            StartWalking(dir);

            // anderen Leuten noch ggf Bescheid sagen
            gwg->RoadNodeAvailable(this->x, this->y);


        }
    }
}

void noFigure::Abrogate()
{
    // Arbeisplatz oder Laghaus Bescheid sagen
    if(fs == FS_GOHOME)
    {
        if(goal) //goal might by NULL if goal was a harbor that got destroyed during sea travel
            static_cast<nobBaseWarehouse*>(goal)->RemoveDependentFigure(this);
        else
        {
            if(!on_ship) //no goal but going home - should not happen
            {
                LOG.lprintf("noFigure::Abrogate - GOHOME figure has no goal and is not on a ship - player %i state %i pos %u,%u \n", player, fs, x, y);
                //assert(false);
            }
        }
    }
    else
        AbrogateWorkplace();
}


void noFigure::StopIfNecessary(const unsigned short x, const unsigned short y)
{
    // Lauf ich auf Wegen --> wenn man zum Ziel oder Weg läuft oder die Träger, die natürlich auch auf Wegen arbeiten
    if(fs == FS_GOHOME || fs == FS_GOTOGOAL || (fs == FS_JOB && GetGOT() == GOT_NOF_CARRIER))
    {
        // Laufe ich zu diesem Punkt?
        if(current_ev)
        {
            if(!waiting_for_free_node && gwg->GetXA(this->x, this->y, dir) == x &&
                    gwg->GetYA(this->x, this->y, dir) == y)
            {
                // Dann stehenbleiben
                PauseWalking();
                waiting_for_free_node = true;
                gwg->StopOnRoads(this->x, this->y, dir);
            }
        }
    }
}


/// Sichtbarkeiten berechnen für Figuren mit Sichtradius (Soldaten, Erkunder) vor dem Laufen
void noFigure::CalcVisibilities(const MapCoord x, const MapCoord y)
{
    // Sichtbarkeiten neu berechnen für Erkunder und Soldaten
    if(GetVisualRange())
        // An alter Position neu berechnen
        gwg->RecalcVisibilitiesAroundPoint(x, y, GetVisualRange(), player, NULL);
}

/// Informiert die Figur, dass für sie eine Schiffsreise beginnt
void noFigure::StartShipJourney(const Point<MapCoord> goal)
{
    // remove us from where we are, so nobody will ever draw us :)
    gwg->RemoveFigure(this, this->x, this->y);

    x = goal.x;
    y = goal.y;
    on_ship = true;
}

/// Informiert die Figur, wenn Kreuzfahrt beendet ist
void noFigure::ShipJourneyEnded()
{
    on_ship = false;
}

/// Examines the route (maybe harbor, road destroyed?) before start shipping
Point<MapCoord> noFigure::ExamineRouteBeforeShipping()
{
    Point<MapCoord> next_harbor;
    // Calc new route
    dir = gwg->FindHumanPathOnRoads(gwg->GetSpecObj<noRoadNode>(x, y), goal, NULL, &next_harbor);


    if(dir == 0xff)
        Abrogate();

    // Going by ship?
    if(dir == SHIP_DIR)
        // All ok, return next harbor (could be another one!)
        return next_harbor;
    else
        return Point<MapCoord>(0, 0);
}

///////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////

