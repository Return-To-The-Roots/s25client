// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "figures/noFigure.h"
#include "EventManager.h"
#include "FindWhConditions.h"
#include "GamePlayer.h"
#include "LeatherLoader.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "WineLoader.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHarborBuilding.h"
#include "figures/nofArmored.h"
#include "helpers/containerUtils.h"
#include "network/GameClient.h"
#include "nofCarrier.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "ogl/glArchivItem_Bob.h"
#include "ogl/glSmartBitmap.h"
#include "pathfinding/PathConditionHuman.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noRoadNode.h"
#include "nodeObjs/noSkeleton.h"
#include "gameTypes/DirectionToImgDir.h"
#include "gameData/GameConsts.h"
#include "gameData/JobConsts.h"
#include "s25util/Log.h"
#include "s25util/colors.h"

const RoadSegment noFigure::emulated_wanderroad(RoadType::Normal, nullptr, nullptr,
                                                std::vector<Direction>(0, Direction::East));
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
    : noMovable(NodalObjectType::Figure, pos), fs(FigureState::GotToGoal), job_(job), player(player), cur_rs(nullptr),
      rs_pos(0), rs_dir(false), on_ship(false), goal_(goal), waiting_for_free_node(false), wander_way(0),
      wander_tryings(0), flagPos_(MapPoint::Invalid()), flag_obj_id(0), burned_wh_id(0xFFFFFFFF), last_id(0xFFFFFFFF)
{
    // If the goal is a storehouse we won't work there but go to the new home
    if(goal && nobBaseWarehouse::isStorehouseGOT(goal->GetGOT()))
        fs = FigureState::GoHome;
}

noFigure::noFigure(const Job job, const MapPoint pos, const unsigned char player)
    : noMovable(NodalObjectType::Figure, pos), fs(FigureState::Job), job_(job), player(player), cur_rs(nullptr),
      rs_pos(0), rs_dir(false), on_ship(false), goal_(nullptr), waiting_for_free_node(false), wander_way(0),
      wander_tryings(0), flagPos_(MapPoint::Invalid()), flag_obj_id(0), burned_wh_id(0xFFFFFFFF), last_id(0xFFFFFFFF)
{}

void noFigure::Destroy()
{
    RTTR_Assert(HasNoGoal());
    RTTR_Assert(!cur_rs);
    noMovable::Destroy();

    RTTR_Assert(!world->GetPlayer(player).IsDependentFigure(*this));
}

void noFigure::Serialize(SerializedGameData& sgd) const
{
    noMovable::Serialize(sgd);

    sgd.PushEnum<uint8_t>(fs);
    sgd.PushEnum<uint8_t>(job_);
    sgd.PushUnsignedChar(player);
    sgd.PushObject(cur_rs, true);
    sgd.PushUnsignedShort(rs_pos);
    sgd.PushBool(rs_dir);
    sgd.PushBool(on_ship);

    if(fs == FigureState::GotToGoal || fs == FigureState::GoHome)
        sgd.PushObject(goal_);

    sgd.PushBool(waiting_for_free_node);

    if(fs == FigureState::Wander)
    {
        sgd.PushUnsignedShort(wander_way);
        sgd.PushUnsignedShort(wander_tryings);
        helpers::pushPoint(sgd, flagPos_);
        sgd.PushUnsignedInt(flag_obj_id);
        sgd.PushUnsignedInt(burned_wh_id);
    }
}

noFigure::noFigure(SerializedGameData& sgd, const unsigned obj_id)
    : noMovable(sgd, obj_id), fs(sgd.Pop<FigureState>()), job_(sgd.Pop<Job>()), player(sgd.PopUnsignedChar()),
      cur_rs(sgd.PopObject<RoadSegment>(GO_Type::Roadsegment)), rs_pos(sgd.PopUnsignedShort()), rs_dir(sgd.PopBool()),
      on_ship(sgd.PopBool()), last_id(0xFFFFFFFF)
{
    if(fs == FigureState::GotToGoal || fs == FigureState::GoHome)
        goal_ = sgd.PopObject<noRoadNode>();
    else
        goal_ = nullptr;

    waiting_for_free_node = sgd.PopBool();

    if(fs == FigureState::Wander)
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
    return isSoldierJob(job_);
}

void noFigure::ActAtFirst()
{
    // Je nach unserem Status bestimmte Dinge tun
    switch(fs)
    {
        default: break;
        case FigureState::GotToGoal: WalkToGoal(); break;
        case FigureState::Job:
            StartWalking(Direction::SouthEast);
            break; // erstmal rauslaufen, darum kümmern sich dann die abgeleiteten Klassen
        case FigureState::GoHome:
        {
            // Wenn ich gleich wieder nach Hause geschickt wurde und aus einem Lagerhaus rauskomme, gar nicht erst
            // rausgehen!
            if(goal_->GetPos() == pos)
            {
                // Reset goal before re-adding to wh
                auto* wh = checkedCast<nobBaseWarehouse*>(goal_);
                goal_ = nullptr;
                cur_rs = nullptr;
                wh->AddFigure(world->RemoveFigure(pos, *this));
            } else
                // ansonsten ganz normal rausgehen
                WalkToGoal();
        }
        break;
        case FigureState::Wander:
            StartWalking(Direction::SouthEast);
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
    MapPoint targetPt = world->GetNeighbour(pos, GetCurMoveDir());
    Position curPt = world->GetNodePos(pos);
    Position nextPt = world->GetNodePos(targetPt);

    Position offset(0, 0);

    if(GetCurMoveDir() == Direction::NorthWest
       && (world->GetNO(targetPt)->GetType() == NodalObjectType::Buildingsite
           || world->GetNO(targetPt)->GetType() == NodalObjectType::Building))
    {
        auto* const bld = world->GetSpecObj<noBaseBuilding>(targetPt);
        nextPt += bld->GetDoorPoint();
    } else if(GetCurMoveDir() == Direction::SouthEast
              && (world->GetNO(pos)->GetType() == NodalObjectType::Buildingsite
                  || world->GetNO(pos)->GetType() == NodalObjectType::Building))
    {
        auto* const bld = world->GetSpecObj<noBaseBuilding>(pos);
        curPt += bld->GetDoorPoint();
        offset = bld->GetDoorPoint();
    }

    return offset + CalcRelative(curPt, nextPt);
}

void noFigure::StartWalking(const Direction dir)
{
    RTTR_Assert(!(GetGOT() == GO_Type::NofPassivesoldier && fs == FigureState::Job));

    // Gehen wir in ein Gebäude?
    if(dir == Direction::NorthWest
       && world->GetNO(world->GetNeighbour(pos, Direction::NorthWest))->GetType() == NodalObjectType::Building)
        world->GetSpecObj<noBuilding>(world->GetNeighbour(pos, Direction::NorthWest))
          ->OpenDoor(); // Dann die Tür aufmachen
    // oder aus einem raus?
    else if(dir == Direction::SouthEast && world->GetNO(pos)->GetType() == NodalObjectType::Building)
        world->GetSpecObj<noBuilding>(pos)->OpenDoor(); // Dann die Tür aufmachen

    // Ist der Platz schon besetzt, wo wir hinlaufen wollen und laufen wir auf Straßen?
    if(!world->IsRoadNodeForFigures(world->GetNeighbour(pos, dir)) && cur_rs)
    {
        // Dann stehen bleiben!
        FaceDir(dir);
        waiting_for_free_node = true;
        // Andere Figuren stoppen
        world->StopOnRoads(pos, dir);
    } else
    {
        // Normal hinlaufen
        StartMoving(dir, 20);
    }
}

void noFigure::DrawShadow(DrawPoint drawPt, const unsigned char anistep, Direction dir)
{
    auto* bitmap = LOADER.GetMapTexture(calcWalkFrameIndex(900, dir, anistep));
    if(bitmap)
        bitmap->DrawFull(drawPt, COLOR_SHADOW);
}

void noFigure::WalkFigure()
{
    // Tür hinter sich zumachen, wenn wir aus einem Gebäude kommen
    if(GetCurMoveDir() == Direction::SouthEast && world->GetNO(pos)->GetType() == NodalObjectType::Building)
        world->GetSpecObj<noBuilding>(pos)->CloseDoor();

    Walk();

    if(cur_rs)
        ++rs_pos;

    // oder in eins reingegangen sind
    if(GetCurMoveDir() == Direction::NorthWest && world->GetNO(pos)->GetType() == NodalObjectType::Building)
        world->GetSpecObj<noBuilding>(pos)->CloseDoor();
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
        if(GetGOT() == GO_Type::NofCarrier && fs == FigureState::GotToGoal)
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
            if(fs == FigureState::GoHome)
            {
                // Mann im Lagerhaus angekommen
                static_cast<nobBaseWarehouse*>(goal)->AddFigure(world->RemoveFigure(pos, *this));
            } else
            {
                // abgeleiteter Klasse sagen, dass das Ziel erreicht wurde
                fs = FigureState::Job;
                GoalReached();
            }

        } else
        {
            MapPoint next_harbor;
            // Neuen Weg berechnen
            auto* const curRoadNode = world->GetSpecObj<noRoadNode>(pos);
            RoadPathDirection route = curRoadNode ?
                                        world->FindHumanPathOnRoads(*curRoadNode, *goal_, nullptr, &next_harbor) :
                                        RoadPathDirection::None;
            // Kein Weg zum Ziel... nächstes Lagerhaus suchen
            if(route == RoadPathDirection::None)
            {
                // Arbeisplatz oder Laghaus Bescheid sagen
                Abrogate();
                // Wir gehen jetzt nach Hause
                GoHome();
                // Evtl wurde kein Lagerhaus gefunden und wir sollen rumirren, dann tun wir das gleich
                if(fs == FigureState::Wander)
                    Wander();
                else
                    WalkToGoal(); // Nach Hause laufen...
            }
            // Oder müssen wir das Schiff nehmen?
            else if(route == RoadPathDirection::Ship)
            {
                // Uns in den Hafen einquartieren
                noBase* hb = world->GetNO(pos);
                if(hb->GetGOT() != GO_Type::NobHarborbuilding)
                {
                    // Es gibt keinen Hafen mehr -> nach Hause gehen

                    // Arbeitsplatz oder Lagerhaus Bescheid sagen
                    Abrogate();
                    // Wir gehen jetzt nach Hause
                    GoHome();
                    // Evtl wurde kein Lagerhaus gefunden und wir sollen rumirren, dann tun wir das gleich
                    if(fs == FigureState::Wander)
                        Wander();
                    else
                        WalkToGoal(); // Nach Hause laufen...
                } else
                {
                    // Uns in den Hafen einquartieren
                    cur_rs = nullptr; // wir laufen nicht mehr auf einer Straße
                    static_cast<nobHarborBuilding*>(hb)->AddFigureForShip(world->RemoveFigure(pos, *this), next_harbor);
                }
            } else
            {
                // Get next street we are walking on
                const Direction walkDir = toDirection(route);
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
        const Direction old_dir = GetCurMoveDir();

        const MapPoint old_pos(pos);

        switch(fs)
        {
            case FigureState::GoHome:
            case FigureState::GotToGoal: WalkToGoal(); break;

            case FigureState::Job: Walked(); break;
            case FigureState::Wander: Wander(); break;
        }

        // Ggf. Sichtbereich testen
        const unsigned visualRange = GetVisualRange();
        if(visualRange)
        {
            // Use old position (don't use this->pos because it might be different now
            // Figure could be in a ship etc.)
            world->RecalcMovingVisibilities(old_pos, player, visualRange, old_dir, nullptr);

            // Wenn Figur verschwunden ist, muss ihr ehemaliger gesamter Sichtbereich noch einmal
            // neue berechnet werden
            if(!world->HasFigureAt(old_pos, *this))
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
            RTTR_Assert(nobBaseWarehouse::isStorehouseGOT(world->GetNO(pos)->GetGOT()));
            goal_ = nullptr;
            world->GetSpecObj<nobBaseWarehouse>(pos)->CancelFigure(this);
            return;
        } else
            this->goal_ = world->GetPlayer(player).FindWarehouse((rs_dir) ? *cur_rs->GetF1() : *cur_rs->GetF2(),
                                                                 FW::AcceptsFigure(job_), true, false);
    } else
        this->goal_ = goal;

    if(this->goal_)
    {
        fs = FigureState::GoHome;
        // Lagerhaus Bescheid sagen
        static_cast<nobBaseWarehouse*>(this->goal_)->AddDependentFigure(*this);

        // Wenn wir stehen, zusätzlich noch loslaufen!
        if(waiting_for_free_node)
        {
            waiting_for_free_node = false;
            WalkToGoal();
            // anderen Leuten noch ggf Bescheid sagen
            world->RoadNodeAvailable(pos);
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
    fs = FigureState::Wander;
    cur_rs = nullptr;
    rs_pos = 0;
    this->burned_wh_id = burned_wh_id;
    // eine bestimmte Strecke rumirren und dann eine Flagge suchen
    // 3x rumirren und eine Flagge suchen, wenn dann keine gefunden wurde, stirbt die Figur
    wander_way = WANDER_WAY_MIN + RANDOM_RAND(WANDER_WAY_MAX - WANDER_WAY_MIN);
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
    World& gwb;
    Point2Flag(World& gwb) : gwb(gwb) {}
    noFlag* operator()(const MapPoint pt, unsigned /*r*/) const { return gwb.GetSpecObj<noFlag>(pt); }
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
        const std::vector<noFlag*> flags =
          world->GetPointsInRadius(pos, wander_radius, Point2Flag(*world), IsValidFlag(player));

        unsigned best_way = 0xFFFFFFFF;
        const noFlag* best_flag = nullptr;

        for(auto* flag : flags)
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
            unsigned way = world->CalcDistance(pos, flag->GetPos());
            if(way < best_way)
            {
                // Are we at that flag or is there a path to it?
                if(way == 0 || world->FindHumanPath(pos, flag->GetPos(), wander_radius, false, &way))
                {
                    // gucken, ob ein Weg zu einem Warenhaus führt
                    if(world->GetPlayer(player).FindWarehouse(*flag, FW::AcceptsFigure(job_), true, false))
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

                    // TODO: Actually it is possible! E.g. between us and the flag is a river, so we won't find a path
                    // within the radius but others (on the other side) could --> Remove ImpossibleForBWU?
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
            wander_way = WANDER_WAY_MIN + RANDOM_RAND(WANDER_WAY_MAX - WANDER_WAY_MIN);
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
    PathConditionHuman pathChecker(*world);
    // Check all dirs starting with a random one and taking the first possible
    for(const Direction dir : helpers::enumRange(RANDOM_ENUM(Direction)))
    {
        if(pathChecker.IsNodeOk(world->GetNeighbour(pos, dir)) && pathChecker.IsEdgeOk(pos, dir))
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
    noBase* no = world->GetNO(flagPos_);
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
        RTTR_Assert(world->GetSpecObj<noRoadNode>(pos));
        if(nobBaseWarehouse* wh = world->GetPlayer(player).FindWarehouse(*world->GetSpecObj<noRoadNode>(pos),
                                                                         FW::AcceptsFigure(job_), true, false))
        {
            // ja, dann können wir ja hingehen
            goal_ = wh;
            cur_rs = nullptr;
            rs_pos = 0;
            fs = FigureState::GoHome;
            wh->AddDependentFigure(*this);
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
    const auto dir = world->FindHumanPath(pos, flagPos_, 60, false);
    if(dir)
    {
        // weiter hinlaufen
        StartWalking(*dir);
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

unsigned noFigure::CalcWalkAnimationFrame() const
{
    // If we are waiting for a free node use the 2nd frame, else interpolate
    return waiting_for_free_node ? 2 : GAMECLIENT.Interpolate(ASCENT_ANIMATION_STEPS[GetAscent()], current_ev) % 8;
}

/// Calculate the index of the current frame for a figure walking in the given direction
/// The sprites start at `imgSetIndex` and contain 8 images per direction, starting at EAST going clockwise.
/// Note that some of the 48 indices might be empty entries in the archive if only specific directions can ever be drawn
unsigned noFigure::calcWalkFrameIndex(const unsigned imgSetIndex, const Direction dir, const unsigned animationStep)
{
    RTTR_Assert(animationStep < 8);
    return imgSetIndex + rttr::enum_cast(toImgDir(dir)) * 8 + animationStep;
}

DrawPoint noFigure::InterpolateWalkDrawPos(DrawPoint drawPt) const
{
    // Add an offset relative to the starting point calculated by how far we already walked
    // Don't if we are waiting on our starting node
    if(!waiting_for_free_node || IsStoppedBetweenNodes())
        drawPt += CalcFigurRelative();
    return drawPt;
}

void noFigure::DrawWalkingCarrier(DrawPoint drawPt, helpers::OptionalEnum<GoodType> ware, bool fat)
{
    const unsigned ani_step = CalcWalkAnimationFrame();
    const GamePlayer& owner = world->GetPlayer(player);
    auto& sprite = ware ? LOADER.getCarrierSprite(*ware, fat, GetCurMoveDir(), ani_step) :
                          LOADER.getCarrierBobSprite(owner.nation, fat, GetCurMoveDir(), ani_step);
    sprite.drawForPlayer(InterpolateWalkDrawPos(drawPt), owner.color);
}

void noFigure::DrawWalkingBobJobs(DrawPoint drawPt, Job job)
{
    const unsigned ani_step = CalcWalkAnimationFrame();
    const GamePlayer& owner = world->GetPlayer(player);
    LOADER.getBobSprite(owner.nation, job, GetCurMoveDir(), ani_step)
      .drawForPlayer(InterpolateWalkDrawPos(drawPt), owner.color);
}

void noFigure::DrawWalking(DrawPoint drawPt, glArchivItem_Bob* file, unsigned id, bool fat)
{
    const unsigned ani_step = CalcWalkAnimationFrame();
    drawPt = InterpolateWalkDrawPos(drawPt);

    if(file)
        file->Draw(id, toImgDir(GetCurMoveDir()), fat, ani_step, drawPt, world->GetPlayer(player).color);
    DrawShadow(drawPt, ani_step, GetCurMoveDir());
}

void noFigure::DrawWalking(DrawPoint drawPt, const ResourceId& file, unsigned id)
{
    const unsigned ani_step = CalcWalkAnimationFrame();
    drawPt = InterpolateWalkDrawPos(drawPt);

    LOADER.GetPlayerImage(file, calcWalkFrameIndex(id, GetCurMoveDir(), ani_step))
      ->DrawFull(drawPt, COLOR_WHITE, world->GetPlayer(player).color);
    DrawShadow(drawPt, ani_step, GetCurMoveDir());
}

void noFigure::DrawWalking(DrawPoint drawPt)
{
    switch(job_)
    {
        case Job::PackDonkey:
        {
            const unsigned ani_step = CalcWalkAnimationFrame();
            drawPt = InterpolateWalkDrawPos(drawPt);

            LOADER.GetMapTexture(calcWalkFrameIndex(2000, GetCurMoveDir(), ani_step))->DrawFull(drawPt);
            // donkey shadow
            LOADER.GetMapTexture(2048 + rttr::enum_cast(GetCurMoveDir()) % 3)->DrawFull(drawPt, COLOR_SHADOW);
        }
        break;
        case Job::CharBurner: DrawWalking(drawPt, "charburner_bobs", 53); break;
        case Job::Vintner:
            DrawWalking(drawPt, "wine_bobs", wineaddon::bobIndex[wineaddon::BobTypes::VINTNER_WALKING]);
            break;
        case Job::Winegrower:
            DrawWalking(drawPt, "wine_bobs", wineaddon::bobIndex[wineaddon::BobTypes::WINEGROWER_WALKING_WITH_SHOVEL]);
            break;
        case Job::TempleServant:
            DrawWalking(drawPt, "wine_bobs", wineaddon::bobIndex[wineaddon::BobTypes::TEMPLESERVANT_WALKING]);
            break;
        case Job::Skinner:
            DrawWalking(drawPt, "leather_bobs", leatheraddon::bobIndex[leatheraddon::BobTypes::SKINNER_WALKING]);
            break;
        case Job::Tanner:
            DrawWalking(drawPt, "leather_bobs", leatheraddon::bobIndex[leatheraddon::BobTypes::TANNER_WALKING]);
            break;
        case Job::LeatherWorker:
            DrawWalking(drawPt, "leather_bobs", leatheraddon::bobIndex[leatheraddon::BobTypes::LEATHERWORKER_WALKING]);
            break;
        default: DrawWalkingBobJobs(drawPt, job_); break;
    }
}

void noFigure::Die()
{
    // Weg mit mir
    GetEvMgr().AddToKillList(world->RemoveFigure(pos, *this));
    // ggf. Leiche hinlegen, falls da nix ist
    if(!world->GetSpecObj<noBase>(pos))
        world->SetNO(pos, new noSkeleton(pos));

    RemoveFromInventory();

    // Sichtbarkeiten neu berechnen für Erkunder und Soldaten
    CalcVisibilities(pos);
}

void noFigure::RemoveFromInventory()
{
    // Wars ein Bootmann? Dann Boot und Träger abziehen
    if(job_ == Job::BoatCarrier)
    {
        world->GetPlayer(player).DecreaseInventoryJob(Job::Helper, 1);
        world->GetPlayer(player).DecreaseInventoryWare(GoodType::Boat, 1);
    } else
    {
        world->GetPlayer(player).DecreaseInventoryJob(job_, 1);
        nofArmored* armoredFigure = dynamic_cast<nofArmored*>(this);
        if(armoredFigure && armoredFigure->HasArmor())
            world->GetPlayer(player).DecreaseInventoryJob(figureToAmoredSoldierEnum(armoredFigure), 1);
    }
}

void noFigure::DieFailedTrade()
{
    // Weg mit mir
    GetEvMgr().AddToKillList(world->RemoveFigure(pos, *this));
    // ggf. Leiche hinlegen, falls da nix ist
    if(!world->GetSpecObj<noBase>(pos))
        world->SetNO(pos, new noSkeleton(pos));
}

void noFigure::NodeFreed(const MapPoint pt)
{
    // Stehen wir gerade aus diesem Grund?
    if(!waiting_for_free_node || pt != world->GetNeighbour(this->pos, GetCurMoveDir()))
        return;

    // Gehen wir in ein Gebäude? Dann wieder ausgleichen, weil wir die Türen sonst doppelt aufmachen!
    if(GetCurMoveDir() == Direction::NorthWest
       && world->GetNO(world->GetNeighbour(this->pos, Direction::NorthWest))->GetType() == NodalObjectType::Building)
        world->GetSpecObj<noBuilding>(world->GetNeighbour(this->pos, Direction::NorthWest))->CloseDoor();
    // oder aus einem raus?
    if(GetCurMoveDir() == Direction::SouthEast && world->GetNO(this->pos)->GetType() == NodalObjectType::Building)
        world->GetSpecObj<noBuilding>(this->pos)->CloseDoor();

    // Wir stehen nun nicht mehr
    waiting_for_free_node = false;

    // Dann loslaufen
    StartWalking(GetCurMoveDir());

    // anderen Leuten noch ggf Bescheid sagen
    world->RoadNodeAvailable(this->pos);
}

void noFigure::Abrogate()
{
    // Arbeisplatz oder Laghaus Bescheid sagen
    if(fs == FigureState::GoHome)
    {
        // goal might by nullptr if goal was a harbor that got destroyed during sea travel
        if(goal_)
        {
            checkedCast<nobBaseWarehouse*>(goal_)->RemoveDependentFigure(*this);
            goal_ = nullptr;
        } else
        {
            if(!on_ship) // no goal but going home - should not happen
            {
                LOG.write("noFigure::Abrogate - GOHOME figure has no goal and is not on a ship - player %i state %i "
                          "pos %u,%u \n")
                  % player % rttr::enum_cast(fs) % pos.x % pos.y;
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
    if(fs == FigureState::GoHome || fs == FigureState::GotToGoal
       || (fs == FigureState::Job && GetGOT() == GO_Type::NofCarrier))
    {
        // Laufe ich zu diesem Punkt?
        if(current_ev && !waiting_for_free_node && world->GetNeighbour(this->pos, GetCurMoveDir()) == pt)
        {
            // Dann stehenbleiben
            PauseWalking();
            waiting_for_free_node = true;
            world->StopOnRoads(this->pos, GetCurMoveDir());
        }
    }
}

/// Sichtbarkeiten berechnen für Figuren mit Sichtradius (Soldaten, Erkunder) vor dem Laufen
void noFigure::CalcVisibilities(const MapPoint pt)
{
    const unsigned visualRange = GetVisualRange();
    if(visualRange)
        // An alter Position neu berechnen
        world->RecalcVisibilitiesAroundPoint(pt, visualRange, player, nullptr);
}

/// Informiert die Figur, dass für sie eine Schiffsreise beginnt
void noFigure::StartShipJourney()
{
    // We should not be in the world, as we start the journey from a harbor -> We are in that harbor
    RTTR_Assert(!world->HasFigureAt(pos, *this));
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
MapPoint noFigure::ExamineRouteBeforeShipping(RoadPathDirection& newDir)
{
    MapPoint next_harbor;
    // Calc new route
    const noRoadNode* roadNode = world->GetSpecObj<noRoadNode>(pos);
    if(!roadNode || !goal_)
        newDir = RoadPathDirection::None;
    else
        newDir = world->FindHumanPathOnRoads(*roadNode, *goal_, nullptr, &next_harbor);

    if(newDir == RoadPathDirection::None)
        Abrogate();

    // Going by ship?
    if(newDir == RoadPathDirection::Ship)
        // All ok, return next harbor (could be another one!)
        return next_harbor;
    else
        return MapPoint(0, 0);
}
