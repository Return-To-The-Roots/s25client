//
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

#include "rttrDefines.h" // IWYU pragma: keep
#include "figures/noFigure.h"
#include "EventManager.h"
#include "FindWhConditions.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHarborBuilding.h"
#include "helpers/containerUtils.h"
#include "network/GameClient.h"
#include "nofCarrier.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "ogl/glArchivItem_Bob.h"
#include "ogl/glSmartBitmap.h"
#include "pathfinding/PathConditionHuman.h"
#include "random/Random.h"
#include "world/GameWorldGame.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noRoadNode.h"
#include "nodeObjs/noSkeleton.h"
#include "gameData/GameConsts.h"
#include "gameData/JobConsts.h"
#include "s25util/Log.h"
#include "s25util/colors.h"

const RoadSegment noFigure::emulated_wanderroad(RoadSegment::RT_NORMAL, nullptr, nullptr, std::vector<Direction>(0, Direction::EAST));
/// Welche Strecke soll minimal und maximal zurückgelegt werden beim Rumirren, bevor eine Flagge gesucht wird
const unsigned short WANDER_WAY_MIN = 20;
const unsigned short WANDER_WAY_MAX = 40;
/// Versuche, eine Flagge zu finden, bis er stirbt beim Rumirren
const unsigned short WANDER_TRYINGS = 3;
// Größe des Rechtecks um den Punkt, wo er die Flaggen sucht beim Rumirren
const unsigned short WANDER_RADIUS = 10;
/// Dasselbe nochmal für Soldaten
const unsigned short WANDER_TRYINGS_SOLDIERS = 6;
const unsigned short WANDER_RADIUS_SOLDIERS = 15;

noFigure::noFigure(const Job job, const MapPoint pos, const unsigned char player, noRoadNode* const goal)
    : noMovable(NOP_FIGURE, pos), fs(FS_GOTOGOAL), job_(job), player(player), cur_rs(nullptr), rs_pos(0), rs_dir(false), on_ship(false),
      goal_(goal), waiting_for_free_node(false), wander_way(0), wander_tryings(0), flagPos_(MapPoint::Invalid()), flag_obj_id(0),
      burned_wh_id(0xFFFFFFFF), last_id(0xFFFFFFFF)
{
    // Haben wir ein Ziel?
    // Gehen wir in ein Lagerhaus? Dann dürfen wir da nicht unsere Arbeit ausführen, sondern
    // gehen quasi nach Hause von Anfang an aus
    if(goal && (goal->GetGOT() == GOT_NOB_HARBORBUILDING || goal->GetGOT() == GOT_NOB_STOREHOUSE || goal->GetGOT() == GOT_NOB_HQ))
        fs = FS_GOHOME;
}

noFigure::noFigure(const Job job, const MapPoint pos, const unsigned char player)
    : noMovable(NOP_FIGURE, pos), fs(FS_JOB), job_(job), player(player), cur_rs(nullptr), rs_pos(0), rs_dir(false), on_ship(false),
      goal_(nullptr), waiting_for_free_node(false), wander_way(0), wander_tryings(0), flagPos_(MapPoint::Invalid()), flag_obj_id(0),
      burned_wh_id(0xFFFFFFFF), last_id(0xFFFFFFFF)
{}

void noFigure::Destroy_noFigure()
{
    RTTR_Assert(HasNoGoal());
    RTTR_Assert(!cur_rs);
    noMovable::Destroy();

    RTTR_Assert(!gwg->GetPlayer(player).IsDependentFigure(this));
}

void noFigure::Serialize_noFigure(SerializedGameData& sgd) const
{
    noMovable::Serialize(sgd);

    sgd.PushUnsignedChar(static_cast<unsigned char>(fs));
    sgd.PushUnsignedChar(static_cast<unsigned char>(job_));
    sgd.PushUnsignedChar(player);
    sgd.PushObject(cur_rs, true);
    sgd.PushUnsignedShort(rs_pos);
    sgd.PushBool(rs_dir);
    sgd.PushBool(on_ship);

    if(fs == FS_GOTOGOAL || fs == FS_GOHOME)
        sgd.PushObject(goal_, false);

    sgd.PushBool(waiting_for_free_node);

    if(fs == FS_WANDER)
    {
        sgd.PushUnsignedShort(wander_way);
        sgd.PushUnsignedShort(wander_tryings);
        sgd.PushMapPoint(flagPos_);
        sgd.PushUnsignedInt(flag_obj_id);
        sgd.PushUnsignedInt(burned_wh_id);
    }
}

noFigure::noFigure(SerializedGameData& sgd, const unsigned obj_id)
    : noMovable(sgd, obj_id), fs(FigureState(sgd.PopUnsignedChar())), job_(Job(sgd.PopUnsignedChar())), player(sgd.PopUnsignedChar()),
      cur_rs(sgd.PopObject<RoadSegment>(GOT_ROADSEGMENT)), rs_pos(sgd.PopUnsignedShort()), rs_dir(sgd.PopBool()), on_ship(sgd.PopBool()),
      last_id(0xFFFFFFFF)
{
    if(fs == FS_GOTOGOAL || fs == FS_GOHOME)
        goal_ = sgd.PopObject<noRoadNode>(GOT_UNKNOWN);
    else
        goal_ = nullptr;

    waiting_for_free_node = sgd.PopBool();

    if(fs == FS_WANDER)
    {
        wander_way = sgd.PopUnsignedShort();
        wander_tryings = sgd.PopUnsignedShort();
        flagPos_ = sgd.PopMapPoint();
        flag_obj_id = sgd.PopUnsignedInt();
        burned_wh_id = sgd.PopUnsignedInt();
    }
}

bool noFigure::IsSoldier() const
{
    switch(job_)
    {
        case JOB_PRIVATE:
        case JOB_PRIVATEFIRSTCLASS:
        case JOB_SERGEANT:
        case JOB_OFFICER:
        case JOB_GENERAL: return true;
        default: break;
    }
    return false;
}

void noFigure::ActAtFirst()
{
    // Je nach unserem Status bestimmte Dinge tun
    switch(fs)
    {
        default: break;
        case FS_GOTOGOAL: WalkToGoal(); break;
        case FS_JOB:
            StartWalking(Direction::SOUTHEAST);
            break; // erstmal rauslaufen, darum kümmern sich dann die abgeleiteten Klassen
        case FS_GOHOME:
        {
            // Wenn ich gleich wieder nach Hause geschickt wurde und aus einem Lagerhaus rauskomme, gar nicht erst rausgehen!
            if(goal_->GetPos() == pos)
            {
                gwg->RemoveFigure(pos, this);
                RTTR_Assert(dynamic_cast<nobBaseWarehouse*>(goal_));
                // Reset goal before re-adding to wh
                auto* wh = static_cast<nobBaseWarehouse*>(goal_);
                goal_ = nullptr;
                cur_rs = nullptr;
                wh->AddFigure(this);
            } else
                // ansonsten ganz normal rausgehen
                WalkToGoal();
        }
        break;
        case FS_WANDER:
            StartWalking(Direction::SOUTHEAST);
            break; // erstmal rauslaufen, darum kümmern sich dann die Wander-Funktionen
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

DrawPoint noFigure::CalcFigurRelative() const
{
    MapPoint targetPt = gwg->GetNeighbour(pos, GetCurMoveDir());
    Position curPt = gwg->GetNodePos(pos);
    Position nextPt = gwg->GetNodePos(targetPt);

    Position offset(0, 0);

    if(GetCurMoveDir() == Direction::NORTHWEST
       && (gwg->GetNO(targetPt)->GetType() == NOP_BUILDINGSITE || gwg->GetNO(targetPt)->GetType() == NOP_BUILDING))
    {
        auto* const bld = gwg->GetSpecObj<noBaseBuilding>(targetPt);
        nextPt += bld->GetDoorPoint();
    } else if(GetCurMoveDir() == Direction::SOUTHEAST
              && (gwg->GetNO(pos)->GetType() == NOP_BUILDINGSITE || gwg->GetNO(pos)->GetType() == NOP_BUILDING))
    {
        auto* const bld = gwg->GetSpecObj<noBaseBuilding>(pos);
        curPt += bld->GetDoorPoint();
        offset = bld->GetDoorPoint();
    }

    return offset + CalcRelative(curPt, nextPt);
}

void noFigure::StartWalking(const Direction dir)
{
    RTTR_Assert(!(GetGOT() == GOT_NOF_PASSIVESOLDIER && fs == FS_JOB));

    // Gehen wir in ein Gebäude?
    if(dir == Direction::NORTHWEST && gwg->GetNO(gwg->GetNeighbour(pos, Direction::NORTHWEST))->GetType() == NOP_BUILDING)
        gwg->GetSpecObj<noBuilding>(gwg->GetNeighbour(pos, Direction::NORTHWEST))->OpenDoor(); // Dann die Tür aufmachen
    // oder aus einem raus?
    else if(dir == Direction::SOUTHEAST && gwg->GetNO(pos)->GetType() == NOP_BUILDING)
        gwg->GetSpecObj<noBuilding>(pos)->OpenDoor(); // Dann die Tür aufmachen

    // Ist der Platz schon besetzt, wo wir hinlaufen wollen und laufen wir auf Straßen?
    if(!gwg->IsRoadNodeForFigures(gwg->GetNeighbour(pos, dir)) && cur_rs)
    {
        // Dann stehen bleiben!
        FaceDir(dir);
        waiting_for_free_node = true;
        // Andere Figuren stoppen
        gwg->StopOnRoads(pos, dir.toUInt());
    } else
    {
        // Normal hinlaufen
        StartMoving(dir, 20);
    }
}

void noFigure::DrawShadow(DrawPoint drawPt, const unsigned char anistep, Direction dir)
{
    glArchivItem_Bitmap* bitmap = LOADER.GetMapImageN(900 + (dir + 3u).toUInt() * 8 + anistep);
    if(bitmap)
        bitmap->DrawFull(drawPt, COLOR_SHADOW);
}

void noFigure::WalkFigure()
{
    // Tür hinter sich zumachen, wenn wir aus einem Gebäude kommen
    if(GetCurMoveDir() == Direction::SOUTHEAST && gwg->GetNO(pos)->GetType() == NOP_BUILDING)
        gwg->GetSpecObj<noBuilding>(pos)->CloseDoor();

    Walk();

    if(cur_rs)
        ++rs_pos;

    // oder in eins reingegangen sind
    if(GetCurMoveDir() == Direction::NORTHWEST && gwg->GetNO(pos)->GetType() == NOP_BUILDING)
        gwg->GetSpecObj<noBuilding>(pos)->CloseDoor();
}

void noFigure::WalkToGoal()
{
    // Kein Ziel mehr --> Rumirren
    if(!goal_)
    {
        StartWandering();
        Wander();
        return;
    }

    // Straße abgelaufen oder noch gar keine Straße vorhanden?
    if(!cur_rs || rs_pos == cur_rs->GetLength())
    {
        // Ziel erreicht?
        // Bei dem Träger können das beide Flaggen sein!
        bool reachedGoal;
        if(GetGOT() == GOT_NOF_CARRIER && fs == FS_GOTOGOAL)
        {
            RTTR_Assert(dynamic_cast<nofCarrier*>(this));
            auto* carrier = static_cast<nofCarrier*>(this);
            noRoadNode* flag = carrier->GetFirstFlag();
            if(flag && flag->GetPos() == pos)
                reachedGoal = true;
            else
            {
                flag = carrier->GetSecondFlag();
                reachedGoal = flag && flag->GetPos() == pos;
            }
        } else
        {
            reachedGoal = goal_->GetPos() == pos;
        }

        if(reachedGoal)
        {
            noRoadNode* goal = goal_;
            // Zeug nullen
            cur_rs = nullptr;
            goal_ = nullptr;
            rs_dir = false;
            rs_pos = 0;
            if(fs == FS_GOHOME)
            {
                // Mann im Lagerhaus angekommen
                gwg->RemoveFigure(pos, this);
                static_cast<nobBaseWarehouse*>(goal)->AddFigure(this);
            } else
            {
                // abgeleiteter Klasse sagen, dass das Ziel erreicht wurde
                fs = FS_JOB;
                GoalReached();
            }

        } else
        {
            MapPoint next_harbor;
            // Neuen Weg berechnen
            auto* const curRoadNode = gwg->GetSpecObj<noRoadNode>(pos);
            unsigned char route = curRoadNode ? gwg->FindHumanPathOnRoads(*curRoadNode, *goal_, nullptr, &next_harbor) : 0xFF;
            // Kein Weg zum Ziel... nächstes Lagerhaus suchen
            if(route == 0xFF)
            {
                // Arbeisplatz oder Laghaus Bescheid sagen
                Abrogate();
                // Wir gehen jetzt nach Hause
                GoHome();
                // Evtl wurde kein Lagerhaus gefunden und wir sollen rumirren, dann tun wir das gleich
                if(fs == FS_WANDER)
                    Wander();
                else
                    WalkToGoal(); // Nach Hause laufen...
            }
            // Oder müssen wir das Schiff nehmen?
            else if(route == SHIP_DIR)
            {
                // Uns in den Hafen einquartieren
                noBase* hb = gwg->GetNO(pos);
                if(hb->GetGOT() != GOT_NOB_HARBORBUILDING)
                {
                    // Es gibt keinen Hafen mehr -> nach Hause gehen

                    // Arbeitsplatz oder Lagerhaus Bescheid sagen
                    Abrogate();
                    // Wir gehen jetzt nach Hause
                    GoHome();
                    // Evtl wurde kein Lagerhaus gefunden und wir sollen rumirren, dann tun wir das gleich
                    if(fs == FS_WANDER)
                        Wander();
                    else
                        WalkToGoal(); // Nach Hause laufen...
                } else
                {
                    // Uns in den Hafen einquartieren
                    cur_rs = nullptr; // wir laufen nicht mehr auf einer Straße
                    gwg->RemoveFigure(pos, this);
                    static_cast<nobHarborBuilding*>(hb)->AddFigureForShip(this, next_harbor);
                }
            } else
            {
                // Get next street we are walking on
                Direction walkDir = Direction::fromInt(route);
                cur_rs = curRoadNode->GetRoute(walkDir);
                StartWalking(walkDir);
                rs_pos = 0;
                rs_dir = curRoadNode != cur_rs->GetF1();
            }
        }

    } else
    {
        StartWalking(cur_rs->GetDir(rs_dir, rs_pos));
    }
}

void noFigure::HandleEvent(const unsigned id)
{
    // Bei ID = 0 ists ein Laufevent, bei allen anderen an abgeleitete Klassen weiterleiten
    if(id)
    {
        HandleDerivedEvent(id);
    } else
    {
        current_ev = nullptr;
        WalkFigure();

        // Alte Richtung und Position für die Berechnung der Sichtbarkeiten merken
        Direction old_dir = GetCurMoveDir();

        MapPoint old_pos(pos);

        switch(fs)
        {
            case FS_GOHOME:
            case FS_GOTOGOAL: { WalkToGoal();
            }
            break;

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
            gwg->RecalcMovingVisibilities(old_pos, player, GetVisualRange(), old_dir, nullptr);

            // Wenn Figur verschwunden ist, muss ihr ehemaliger gesamter Sichtbereich noch einmal
            // neue berechnet werden
            if(!helpers::contains(gwg->GetFigures(old_pos), this))
                CalcVisibilities(old_pos);
        }
    }
}

void noFigure::GoHome(noRoadNode* goal)
{
    if(on_ship)
    {
        // Wir befinden uns gerade an Deck, also einfach goal auf Null setzen und dann sehen wir, was so passiert
        this->goal_ = nullptr;
        return;
    }
    // Nächstes Lagerhaus suchen
    else if(!goal)
    {
        // Wenn wir cur_rs == 0, dann hängen wir wahrscheinlich noch im Lagerhaus in der Warteschlange
        if(cur_rs == nullptr)
        {
            RTTR_Assert(gwg->GetNO(pos)->GetGOT() == GOT_NOB_HQ || //-V807
                        gwg->GetNO(pos)->GetGOT() == GOT_NOB_STOREHOUSE || gwg->GetNO(pos)->GetGOT() == GOT_NOB_HARBORBUILDING);

            goal_ = nullptr;
            gwg->GetSpecObj<nobBaseWarehouse>(pos)->CancelFigure(this);
            return;
        } else
            this->goal_ =
              gwg->GetPlayer(player).FindWarehouse((rs_dir) ? *cur_rs->GetF1() : *cur_rs->GetF2(), FW::AcceptsFigure(job_), true, false);
    } else
        this->goal_ = goal;

    if(this->goal_)
    {
        fs = FS_GOHOME;
        // Lagerhaus Bescheid sagen
        static_cast<nobBaseWarehouse*>(this->goal_)->AddDependentFigure(this);

        // Wenn wir stehen, zusätzlich noch loslaufen!
        if(waiting_for_free_node)
        {
            waiting_for_free_node = false;
            WalkToGoal();
            // anderen Leuten noch ggf Bescheid sagen
            gwg->RoadNodeAvailable(this->pos);
        }
    } else
    {
        // Kein Lagerhaus gefunden --> Rumirren
        StartWandering();
        cur_rs = nullptr;
    }
}

void noFigure::StartWandering(const unsigned burned_wh_id)
{
    RTTR_Assert(HasNoGoal());
    fs = FS_WANDER;
    cur_rs = nullptr;
    rs_pos = 0;
    this->burned_wh_id = burned_wh_id;
    // eine bestimmte Strecke rumirren und dann eine Flagge suchen
    // 3x rumirren und eine Flagge suchen, wenn dann keine gefunden wurde, stirbt die Figur
    wander_way = WANDER_WAY_MIN + RANDOM.Rand(__FILE__, __LINE__, GetObjId(), WANDER_WAY_MAX - WANDER_WAY_MIN);
    // Soldaten sind härter im Nehmen
    wander_tryings = IsSoldier() ? WANDER_TRYINGS_SOLDIERS : WANDER_TRYINGS;

    // Wenn wir stehen, zusätzlich noch loslaufen!
    if(waiting_for_free_node)
    {
        waiting_for_free_node = false;
        // We should be paused, so just continue moving now
        if(IsStoppedBetweenNodes())
            StartMoving(GetCurMoveDir(), GetPausedEvent().length);
        else
            Wander();
    }
}

namespace {
struct Point2Flag
{
    using result_type = noFlag*;
    World& gwb;

    Point2Flag(World& gwb) : gwb(gwb) {}

    result_type operator()(const MapPoint pt, unsigned /*r*/) const { return gwb.GetSpecObj<noFlag>(pt); }
};

struct IsValidFlag
{
    const unsigned playerId_;

    IsValidFlag(const unsigned playerId) : playerId_(playerId) {}

    bool operator()(const noFlag* const flag) const { return flag && flag->GetPlayer() == playerId_; }
};
} // namespace

void noFigure::Wander()
{
    // Sind wir noch auf der Suche nach einer Flagge?
    if(wander_way == 0xFFFF)
    {
        // Wir laufen schon zur Flagge
        WanderToFlag();
        return;
    }

    // Ist es mal wieder an der Zeit, eine Flagge zu suchen?
    if(!wander_way)
    {
        // Soldaten sind härter im Nehmen
        const unsigned short wander_radius = IsSoldier() ? WANDER_RADIUS_SOLDIERS : WANDER_RADIUS;

        // Flaggen sammeln und dann zufällig eine auswählen
        const std::vector<noFlag*> flags = gwg->GetPointsInRadius<-1>(pos, wander_radius, Point2Flag(*gwg), IsValidFlag(player));

        unsigned best_way = 0xFFFFFFFF;
        noFlag const* best_flag = nullptr;

        for(auto flag : flags)
        {
            // Ist das ein Flüchtling aus einem abgebrannten Lagerhaus?
            if(burned_wh_id != 0xFFFFFFFF)
            {
                // Dann evtl gucken, ob anderen Mitglieder schon gesagt haben, dass die Flagge nicht zugänglich ist
                if(flag->IsImpossibleForBWU(burned_wh_id))
                {
                    // Dann können wir die Flagge überspringen
                    continue;
                }
            }

            // würde die die bisher beste an Weg unterbieten?
            unsigned way = gwg->CalcDistance(pos, flag->GetPos());
            if(way < best_way)
            {
                // Are we at that flag or is there a path to it?
                if(way == 0 || gwg->FindHumanPath(pos, flag->GetPos(), wander_radius, false, &way) != 0xFF)
                {
                    // gucken, ob ein Weg zu einem Warenhaus führt
                    if(gwg->GetPlayer(player).FindWarehouse(*flag, FW::AcceptsFigure(job_), true, false))
                    {
                        // dann nehmen wir die doch glatt
                        best_way = way;
                        best_flag = flag;
                        if(way == 0)
                            break; // Can't get better
                    }
                } else if(burned_wh_id != 0xFFFFFFFF)
                {
                    // Flagge nicht möglich zugänglich bei einem Flüchting aus einem abgebrannten Lagerhaus?
                    // --> der ganzen Gruppe Bescheid sagen, damit die nicht auch alle sinnlos einen Weg zu
                    // dieser Flagge suchen

                    // TODO: Actually it is possible! E.g. between us and the flag is a river, so we won't find a path within the radius
                    // but others (on the other side) could --> Remove ImpossibleForBWU?
                    flag->ImpossibleForBWU(burned_wh_id);
                }
            }
        }

        if(best_flag)
        {
            // bestmögliche schließlich nehmen
            wander_way = 0xFFFF;
            flagPos_ = best_flag->GetPos();
            flag_obj_id = best_flag->GetObjId();
            WanderToFlag();
            return;
        }

        // Wurde keine Flagge gefunden?

        // Haben wir noch Versuche?
        RTTR_Assert(wander_tryings > 0);
        if(--wander_tryings > 0)
        {
            // von vorne beginnen wieder mit Rumirren
            wander_way = WANDER_WAY_MIN + RANDOM.Rand(__FILE__, __LINE__, GetObjId(), WANDER_WAY_MAX - WANDER_WAY_MIN);
        } else
        {
            // Genug rumgeirrt, wir finden halt einfach nichts --> Sterben
            Die();
            return;
        }
    }

    if(WalkInRandomDir())
    {
        RTTR_Assert(wander_way > 0);
        --wander_way;
    } else
    {
        // Wir sind eingesperrt! Kein Weg mehr gefunden --> Sterben
        Die();
    }
}

bool noFigure::WalkInRandomDir()
{
    PathConditionHuman pathChecker(*gwg);
    // Check all dirs starting with a random one and taking the first possible
    unsigned char dirOffset = RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 6);
    for(unsigned char iDir = 0; iDir < Direction::COUNT; ++iDir)
    {
        Direction dir(iDir + dirOffset);

        if(pathChecker.IsNodeOk(gwg->GetNeighbour(pos, dir)) && pathChecker.IsEdgeOk(pos, dir))
        {
            StartWalking(dir);
            return true;
        }
    }
    return false;
}

void noFigure::WanderFailedTrade()
{
    DieFailedTrade();
}

void noFigure::WanderToFlag()
{
    // Existiert die Flagge überhaupt noch?
    noBase* no = gwg->GetNO(flagPos_);
    if(no->GetObjId() != flag_obj_id)
    {
        // Wenn nicht, wieder normal weiter rumirren
        StartWandering();
        Wander();
        return;
    }

    // Sind wir schon da?
    if(pos == flagPos_)
    {
        // Gibts noch nen Weg zu einem Lagerhaus?
        RTTR_Assert(gwg->GetSpecObj<noRoadNode>(pos));
        if(nobBaseWarehouse* wh =
             gwg->GetPlayer(player).FindWarehouse(*gwg->GetSpecObj<noRoadNode>(pos), FW::AcceptsFigure(job_), true, false))
        {
            // ja, dann können wir ja hingehen
            goal_ = wh;
            cur_rs = nullptr;
            rs_pos = 0;
            fs = FS_GOHOME;
            wh->AddDependentFigure(this);
            WalkToGoal();
            return;
        } else
        {
            // Wenn nicht, wieder normal weiter rumirren
            StartWandering();
            Wander();
            return;
        }
    }

    // Weiter zur Flagge gehen
    // Gibts noch nen Weg dahin bzw. existiert die Flagge noch?
    unsigned char dir = gwg->FindHumanPath(pos, flagPos_, 60, false);
    if(dir != 0xFF)
    {
        // weiter hinlaufen
        StartWalking(Direction::fromInt(dir));
    } else
    {
        // Wenn nicht, wieder normal weiter rumirren
        StartWandering();
        Wander();
    }
}

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
    } else if((rs_pos == cur_rs->GetLength() && !rs_dir) || (rs_pos == rs2->GetLength() && rs_dir))
    {
        // wir stehen genau in der Mitte
        // abhängig von der Richtung machen, in die man gerade läuft
        if(GetCurMoveDir() == rs2->GetRoute(0))
        {
            // wir laufen auf dem 2. Teilstück
            cur_rs = rs2;
            // und wir sind da noch am Anfang
            rs_pos = 0;
        } else if(GetCurMoveDir() == cur_rs->GetRoute(cur_rs->GetLength() - 1) + 3u)
        {
            // wir laufen auf dem 1. Teilstück

            // und wir sind da noch am Anfang
            rs_pos = 0;
        } else
        {
            // Wahrscheinlich stehen wir
            // dann einfach auf das 2. gehen
            cur_rs = rs2;
            rs_pos = 0;
            rs_dir = false;
        }
    }

    CorrectSplitData_Derived();
}

/// Wird aufgerufen, wenn die Straße unter der Figur geteilt wurde (für abgeleitete Klassen)
void noFigure::CorrectSplitData_Derived() {}

void noFigure::DrawWalkingBobCarrier(DrawPoint drawPt, unsigned ware, bool fat)
{
    // Wenn wir warten auf ein freies Plätzchen, müssen wir den stehend zeichnen!
    unsigned ani_step = waiting_for_free_node ? 2 : GAMECLIENT.Interpolate(ASCENT_ANIMATION_STEPS[GetAscent()], current_ev) % 8;

    // Wenn man wartet, stehend zeichnen, es sei denn man wartet mittem auf dem Weg!
    if(!waiting_for_free_node || IsStoppedBetweenNodes())
        drawPt += CalcFigurRelative();

    LOADER.carrier_cache[ware][GetCurMoveDir().toUInt()][ani_step][fat].draw(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);
}

void noFigure::DrawWalkingBobJobs(DrawPoint drawPt, unsigned job)
{
    if((job == JOB_SCOUT) || ((job >= JOB_PRIVATE) && (job <= JOB_GENERAL)))
    {
        DrawWalking(drawPt, LOADER.GetBobN("jobs"), JOB_CONSTS[job].jobs_bob_id + NATION_RTTR_TO_S2[gwg->GetPlayer(player).nation] * 6,
                    false);
        return;
    }

    // Wenn wir warten auf ein freies Plätzchen, müssen wir den stehend zeichnen!
    unsigned ani_step = waiting_for_free_node ? 2 : GAMECLIENT.Interpolate(ASCENT_ANIMATION_STEPS[GetAscent()], current_ev) % 8;

    // Wenn man wartet, stehend zeichnen, es sei denn man wartet mittem auf dem Weg!
    if(!waiting_for_free_node || IsStoppedBetweenNodes())
        drawPt += CalcFigurRelative();

    LOADER.bob_jobs_cache[gwg->GetPlayer(player).nation][job][GetCurMoveDir().toUInt()][ani_step].draw(drawPt, 0xFFFFFFFF,
                                                                                                       gwg->GetPlayer(player).color);
}

void noFigure::DrawWalking(DrawPoint drawPt, glArchivItem_Bob* file, unsigned id, bool fat, bool waitingsoldier)
{
    // Wenn wir warten auf ein freies Plätzchen, müssen wir den stehend zeichnen!
    unsigned ani_step =
      waiting_for_free_node || waitingsoldier ? 2 : GAMECLIENT.Interpolate(ASCENT_ANIMATION_STEPS[GetAscent()], current_ev) % 8;

    // Wenn man wartet, stehend zeichnen, es sei denn man wartet mittem auf dem Weg!
    if(!waitingsoldier && (!waiting_for_free_node || IsStoppedBetweenNodes()))
        drawPt += CalcFigurRelative();
    if(file)
        file->Draw(id, GetCurMoveDir().toUInt(), fat, ani_step, drawPt, gwg->GetPlayer(player).color);
    DrawShadow(drawPt, ani_step, GetCurMoveDir());
}

/// Zeichnet standardmäßig die Figur, wenn sie läuft aus einem bestimmten normalen LST Archiv
void noFigure::DrawWalking(DrawPoint drawPt, const char* const file, unsigned id)
{
    // Wenn wir warten, ani-step 2 benutzen
    unsigned ani_step = waiting_for_free_node ? 2 : GAMECLIENT.Interpolate(ASCENT_ANIMATION_STEPS[GetAscent()], current_ev) % 8;

    // Wenn man wartet, stehend zeichnen, es sei denn man wartet mittem auf dem Weg!
    if(!waiting_for_free_node || IsStoppedBetweenNodes())
        drawPt += CalcFigurRelative();

    LOADER.GetPlayerImage(file, id + (GetCurMoveDir() + 3u).toUInt() * 8 + ani_step)
      ->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);
    DrawShadow(drawPt, ani_step, GetCurMoveDir());
}

void noFigure::DrawWalking(DrawPoint drawPt)
{
    // Figurentyp unterscheiden
    switch(job_)
    {
        case JOB_PACKDONKEY:
        {
            // Wenn wir warten, ani-step 2 benutzen
            unsigned ani_step = waiting_for_free_node ? 2 : GAMECLIENT.Interpolate(ASCENT_ANIMATION_STEPS[GetAscent()], current_ev) % 8;

            // Wenn man wartet, stehend zeichnen, es sei denn man wartet mittem auf dem Weg!
            if(!waiting_for_free_node || IsStoppedBetweenNodes())
                drawPt += CalcFigurRelative();

            // Esel
            LOADER.GetMapImageN(2000 + (GetCurMoveDir() + 3u).toUInt() * 8 + ani_step)->DrawFull(drawPt);
            // Schatten des Esels
            LOADER.GetMapImageN(2048 + GetCurMoveDir().toUInt() % 3)->DrawFull(drawPt, COLOR_SHADOW);
        }
            return;
        case JOB_CHARBURNER: { DrawWalking(drawPt, "charburner_bobs", 53);
        }
            return;
        default: { DrawWalkingBobJobs(drawPt, job_);
        }
            return;
    }
}

void noFigure::Die()
{
    // Weg mit mir
    gwg->RemoveFigure(pos, this);
    GetEvMgr().AddToKillList(this);
    // ggf. Leiche hinlegen, falls da nix ist
    if(!gwg->GetSpecObj<noBase>(pos))
        gwg->SetNO(pos, new noSkeleton(pos));

    RemoveFromInventory();

    // Sichtbarkeiten neu berechnen für Erkunder und Soldaten
    CalcVisibilities(pos);
}

void noFigure::RemoveFromInventory()
{
    // Wars ein Bootmann? Dann Boot und Träger abziehen
    if(job_ == JOB_BOATCARRIER)
    {
        gwg->GetPlayer(player).DecreaseInventoryJob(JOB_HELPER, 1);
        gwg->GetPlayer(player).DecreaseInventoryWare(GD_BOAT, 1);
    } else
        gwg->GetPlayer(player).DecreaseInventoryJob(job_, 1);
}

void noFigure::DieFailedTrade()
{
    // Weg mit mir
    gwg->RemoveFigure(pos, this);
    GetEvMgr().AddToKillList(this);
    // ggf. Leiche hinlegen, falls da nix ist
    if(!gwg->GetSpecObj<noBase>(pos))
        gwg->SetNO(pos, new noSkeleton(pos));
}

void noFigure::NodeFreed(const MapPoint pt)
{
    // Stehen wir gerade aus diesem Grund?
    if(!waiting_for_free_node || pt != gwg->GetNeighbour(this->pos, GetCurMoveDir()))
        return;

    // Gehen wir in ein Gebäude? Dann wieder ausgleichen, weil wir die Türen sonst doppelt aufmachen!
    if(GetCurMoveDir() == Direction::NORTHWEST && gwg->GetNO(gwg->GetNeighbour(this->pos, Direction::NORTHWEST))->GetType() == NOP_BUILDING)
        gwg->GetSpecObj<noBuilding>(gwg->GetNeighbour(this->pos, Direction::NORTHWEST))->CloseDoor();
    // oder aus einem raus?
    if(GetCurMoveDir() == Direction::SOUTHEAST && gwg->GetNO(this->pos)->GetType() == NOP_BUILDING)
        gwg->GetSpecObj<noBuilding>(this->pos)->CloseDoor();

    // Wir stehen nun nicht mehr
    waiting_for_free_node = false;

    // Dann loslaufen
    StartWalking(GetCurMoveDir());

    // anderen Leuten noch ggf Bescheid sagen
    gwg->RoadNodeAvailable(this->pos);
}

void noFigure::Abrogate()
{
    // Arbeisplatz oder Laghaus Bescheid sagen
    if(fs == FS_GOHOME)
    {
        // goal might by nullptr if goal was a harbor that got destroyed during sea travel
        if(goal_)
        {
            RTTR_Assert(dynamic_cast<nobBaseWarehouse*>(goal_));
            static_cast<nobBaseWarehouse*>(goal_)->RemoveDependentFigure(this);
            goal_ = nullptr;
        } else
        {
            if(!on_ship) // no goal but going home - should not happen
            {
                LOG.write("noFigure::Abrogate - GOHOME figure has no goal and is not on a ship - player %i state %i pos %u,%u \n") % player
                  % fs % pos.x % pos.y;
                // RTTR_Assert(false);
            }
        }
    } else
    {
        goal_ = nullptr;
        AbrogateWorkplace();
    }
}

void noFigure::StopIfNecessary(const MapPoint pt)
{
    // Lauf ich auf Wegen --> wenn man zum Ziel oder Weg läuft oder die Träger, die natürlich auch auf Wegen arbeiten
    if(fs == FS_GOHOME || fs == FS_GOTOGOAL || (fs == FS_JOB && GetGOT() == GOT_NOF_CARRIER))
    {
        // Laufe ich zu diesem Punkt?
        if(current_ev && !waiting_for_free_node && gwg->GetNeighbour(this->pos, GetCurMoveDir()) == pt)
        {
            // Dann stehenbleiben
            PauseWalking();
            waiting_for_free_node = true;
            gwg->StopOnRoads(this->pos, GetCurMoveDir().toUInt());
        }
    }
}

/// Sichtbarkeiten berechnen für Figuren mit Sichtradius (Soldaten, Erkunder) vor dem Laufen
void noFigure::CalcVisibilities(const MapPoint pt)
{
    // Sichtbarkeiten neu berechnen für Erkunder und Soldaten
    if(GetVisualRange())
        // An alter Position neu berechnen
        gwg->RecalcVisibilitiesAroundPoint(pt, GetVisualRange(), player, nullptr);
}

/// Informiert die Figur, dass für sie eine Schiffsreise beginnt
void noFigure::StartShipJourney()
{
    // We should not be in the world, as we start the journey from a harbor -> We are in that harbor
    RTTR_Assert(!helpers::contains(gwg->GetFigures(pos), this));
    RTTR_Assert(!on_ship);

    pos = MapPoint::Invalid();
    on_ship = true;
}

void noFigure::ArrivedByShip(const MapPoint harborPos)
{
    RTTR_Assert(on_ship);
    pos = harborPos;
    on_ship = false;
}

/// Examines the route (maybe harbor, road destroyed?) before start shipping
MapPoint noFigure::ExamineRouteBeforeShipping(unsigned char& newDir)
{
    MapPoint next_harbor;
    // Calc new route
    const noRoadNode* roadNode = gwg->GetSpecObj<noRoadNode>(pos);
    if(!roadNode || !goal_)
        newDir = INVALID_DIR;
    else
        newDir = gwg->FindHumanPathOnRoads(*roadNode, *goal_, nullptr, &next_harbor);

    if(newDir == 0xff)
        Abrogate();

    // Going by ship?
    if(newDir == SHIP_DIR)
        // All ok, return next harbor (could be another one!)
        return next_harbor;
    else
        return MapPoint(0, 0);
}
