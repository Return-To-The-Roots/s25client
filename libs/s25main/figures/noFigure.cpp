// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "figures/noFigure.h"
#include "EventManager.h"
#include "FindWhConditions.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "LeatherLoader.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "WineLoader.h"
#include "addons/const_addons.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHarborBuilding.h"
#include "helpers/containerUtils.h"
#include "network/GameClient.h"
#include "nofCarrier.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "ogl/glArchivItem_Bob.h"
#include "ogl/glFont.h"
#include "ogl/glSmartBitmap.h"
#include "pathfinding/PathConditionHuman.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noRoadNode.h"
#include "nodeObjs/noSkeleton.h"
#include "gameTypes/DirectionToImgDir.h"
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
const unsigned short STRANDED_SOLDIER_RETURN_SEARCH_RADIUS_REDUCED = WANDER_RADIUS_SOLDIERS / 2;

noFigure::noFigure(const Job job, const MapPoint pos, const unsigned char player, noRoadNode* const goal)
    : noMovable(NodalObjectType::Figure, pos), fs(FigureState::GotToGoal), job_(job), player(player), cur_rs(nullptr),
      rs_pos(0), rs_dir(false), on_ship(false), goal_(goal), waiting_for_free_node(false), wander_way(0),
      wander_tryings(0), flagPos_(MapPoint::invalid()), flag_obj_id(0), burned_wh_id(0xFFFFFFFF), last_id(0xFFFFFFFF),
      hasArmor_(false)
{
    // If the goal is a storehouse we won't work there but go to the new home
    if(goal && nobBaseWarehouse::isStorehouseGOT(goal->getGOT()))
        fs = FigureState::GoHome;
}

noFigure::noFigure(const Job job, const MapPoint pos, const unsigned char player)
    : noMovable(NodalObjectType::Figure, pos), fs(FigureState::Job), job_(job), player(player), cur_rs(nullptr),
      rs_pos(0), rs_dir(false), on_ship(false), goal_(nullptr), waiting_for_free_node(false), wander_way(0),
      wander_tryings(0), flagPos_(MapPoint::invalid()), flag_obj_id(0), burned_wh_id(0xFFFFFFFF), last_id(0xFFFFFFFF),
      hasArmor_(false)
{}

void noFigure::destroy()
{
    RTTR_Assert(!goal_);
    RTTR_Assert(!cur_rs);
    noMovable::destroy();

    RTTR_Assert(!world->getPlayer(player).isDependentFigure(*this));
}

void noFigure::serialize(SerializedGameData& sgd) const
{
    noMovable::serialize(sgd);

    sgd.pushEnum<uint8_t>(fs);
    sgd.pushEnum<uint8_t>(job_);
    sgd.pushUnsignedChar(player);
    sgd.pushObject(cur_rs, true);
    sgd.pushUnsignedShort(rs_pos);
    sgd.pushBool(rs_dir);
    sgd.pushBool(on_ship);
    sgd.pushBool(hasArmor_);

    if(fs == FigureState::GotToGoal || fs == FigureState::GoHome)
        sgd.pushObject(goal_);

    sgd.pushBool(waiting_for_free_node);

    if(fs == FigureState::Wander)
    {
        sgd.pushUnsignedShort(wander_way);
        sgd.pushUnsignedShort(wander_tryings);
        helpers::pushPoint(sgd, flagPos_);
        sgd.pushUnsignedInt(flag_obj_id);
        sgd.pushUnsignedInt(burned_wh_id);
    }
}

noFigure::noFigure(SerializedGameData& sgd, const unsigned obj_id)
    : noMovable(sgd, obj_id), fs(sgd.pop<FigureState>()), job_(sgd.pop<Job>()), player(sgd.popUnsignedChar()),
      cur_rs(sgd.popObject<RoadSegment>(GO_Type::Roadsegment)), rs_pos(sgd.popUnsignedShort()), rs_dir(sgd.popBool()),
      on_ship(sgd.popBool()), last_id(0xFFFFFFFF), hasArmor_(sgd.getGameDataVersion() >= 12 ? sgd.popBool() : false)
{
    if(fs == FigureState::GotToGoal || fs == FigureState::GoHome)
        goal_ = sgd.popObject<noRoadNode>();
    else
        goal_ = nullptr;

    waiting_for_free_node = sgd.popBool();

    if(fs == FigureState::Wander)
    {
        wander_way = sgd.popUnsignedShort();
        wander_tryings = sgd.popUnsignedShort();
        flagPos_ = sgd.popMapPoint();
        flag_obj_id = sgd.popUnsignedInt();
        burned_wh_id = sgd.popUnsignedInt();
    }
}

bool noFigure::isSoldier() const
{
    return ::isSoldier(job_);
}

void noFigure::actAtFirst()
{
    // Je nach unserem Status bestimmte Dinge tun
    switch(fs)
    {
        default: break;
        case FigureState::GotToGoal: walkToGoal(); break;
        case FigureState::Job:
            startWalking(Direction::SouthEast);
            break; // erstmal rauslaufen, darum kümmern sich dann die abgeleiteten Klassen
        case FigureState::GoHome:
        {
            // Wenn ich gleich wieder nach Hause geschickt wurde und aus einem Lagerhaus rauskomme, gar nicht erst
            // rausgehen!
            if(goal_->getPos() == pos)
            {
                // Reset goal before re-adding to wh
                auto* wh = checkedCast<nobBaseWarehouse*>(goal_);
                goal_ = nullptr;
                cur_rs = nullptr;
                wh->addFigure(world->removeFigure(pos, *this));
            } else
                // ansonsten ganz normal rausgehen
                walkToGoal();
        }
        break;
        case FigureState::Wander:
            startWalking(Direction::SouthEast);
            break; // erstmal rauslaufen, darum kümmern sich dann die Wander-Funktionen
    }
}

/// Gibt den Sichtradius dieser Figur zurück (0, falls nicht-spähend)
unsigned noFigure::getVisualRange() const
{
    return 0;
}

/// Legt die Anfangsdaten für das Laufen auf Wegen fest
void noFigure::initializeRoadWalking(const RoadSegment* const road, const unsigned short rs_pos, const bool rs_dir)
{
    this->cur_rs = road;
    this->rs_pos = rs_pos;
    this->rs_dir = rs_dir;
}

DrawPoint noFigure::calcFigurRelative() const
{
    MapPoint targetPt = world->getNeighbour(pos, getCurMoveDir());
    Position curPt = world->getNodePos(pos);
    Position nextPt = world->getNodePos(targetPt);

    Position offset(0, 0);

    if(getCurMoveDir() == Direction::NorthWest
       && (world->getNO(targetPt)->getType() == NodalObjectType::Buildingsite
           || world->getNO(targetPt)->getType() == NodalObjectType::Building))
    {
        auto* const bld = world->getSpecObj<noBaseBuilding>(targetPt);
        nextPt += bld->getDoorPoint();
    } else if(getCurMoveDir() == Direction::SouthEast
              && (world->getNO(pos)->getType() == NodalObjectType::Buildingsite
                  || world->getNO(pos)->getType() == NodalObjectType::Building))
    {
        auto* const bld = world->getSpecObj<noBaseBuilding>(pos);
        curPt += bld->getDoorPoint();
        offset = bld->getDoorPoint();
    }

    return offset + calcRelative(curPt, nextPt);
}

void noFigure::startWalking(const Direction dir)
{
    RTTR_Assert(!(getGOT() == GO_Type::NofPassivesoldier && fs == FigureState::Job));

    // Gehen wir in ein Gebäude?
    if(dir == Direction::NorthWest
       && world->getNO(world->getNeighbour(pos, Direction::NorthWest))->getType() == NodalObjectType::Building)
        world->getSpecObj<noBuilding>(world->getNeighbour(pos, Direction::NorthWest))
          ->openDoor(); // Dann die Tür aufmachen
    // oder aus einem raus?
    else if(dir == Direction::SouthEast && world->getNO(pos)->getType() == NodalObjectType::Building)
        world->getSpecObj<noBuilding>(pos)->openDoor(); // Dann die Tür aufmachen

    // Ist der Platz schon besetzt, wo wir hinlaufen wollen und laufen wir auf Straßen?
    if(!world->isRoadNodeForFigures(world->getNeighbour(pos, dir)) && cur_rs)
    {
        // Dann stehen bleiben!
        faceDir(dir);
        waiting_for_free_node = true;
        // Andere Figuren stoppen
        world->stopOnRoads(pos, dir);
    } else
    {
        // Normal hinlaufen
        startMoving(dir, 20);
    }
}

void noFigure::drawShadow(DrawPoint drawPt, const unsigned char anistep, Direction dir)
{
    auto* bitmap = LOADER.getMapTexture(calcWalkFrameIndex(900, dir, anistep));
    if(bitmap)
        bitmap->drawFull(drawPt, COLOR_SHADOW);
}

void noFigure::walkFigure()
{
    // Tür hinter sich zumachen, wenn wir aus einem Gebäude kommen
    if(getCurMoveDir() == Direction::SouthEast && world->getNO(pos)->getType() == NodalObjectType::Building)
        world->getSpecObj<noBuilding>(pos)->closeDoor();

    walk();

    if(cur_rs)
        ++rs_pos;

    // oder in eins reingegangen sind
    if(getCurMoveDir() == Direction::NorthWest && world->getNO(pos)->getType() == NodalObjectType::Building)
        world->getSpecObj<noBuilding>(pos)->closeDoor();
}

void noFigure::walkToGoal()
{
    // Kein Ziel mehr --> Rumirren
    if(!goal_)
    {
        startWandering();
        wander();
        return;
    }

    // Straße abgelaufen oder noch gar keine Straße vorhanden?
    if(!cur_rs || rs_pos == cur_rs->getLength())
    {
        // Ziel erreicht?
        // Bei dem Träger können das beide Flaggen sein!
        bool reachedGoal;
        if(getGOT() == GO_Type::NofCarrier && fs == FigureState::GotToGoal)
        {
            RTTR_Assert(dynamic_cast<nofCarrier*>(this));
            auto* carrier = static_cast<nofCarrier*>(this);
            noRoadNode* flag = carrier->getFirstFlag();
            if(flag && flag->getPos() == pos)
                reachedGoal = true;
            else
            {
                flag = carrier->getSecondFlag();
                reachedGoal = flag && flag->getPos() == pos;
            }
        } else
        {
            reachedGoal = goal_->getPos() == pos;
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
                static_cast<nobBaseWarehouse*>(goal)->addFigure(world->removeFigure(pos, *this));
            } else
            {
                // abgeleiteter Klasse sagen, dass das Ziel erreicht wurde
                fs = FigureState::Job;
                goalReached();
            }

        } else
        {
            MapPoint next_harbor;
            // Neuen Weg berechnen
            auto* const curRoadNode = world->getSpecObj<noRoadNode>(pos);
            RoadPathDirection route = curRoadNode ?
                                        world->findHumanPathOnRoads(*curRoadNode, *goal_, nullptr, &next_harbor) :
                                        RoadPathDirection::None;
            // Kein Weg zum Ziel... nächstes Lagerhaus suchen
            if(route == RoadPathDirection::None)
            {
                // Arbeisplatz oder Laghaus Bescheid sagen
                abrogate();
                // Wir gehen jetzt nach Hause
                goHome();
                // Evtl wurde kein Lagerhaus gefunden und wir sollen rumirren, dann tun wir das gleich
                if(fs == FigureState::Wander)
                    wander();
                else
                    walkToGoal(); // Nach Hause laufen...
            }
            // Oder müssen wir das Schiff nehmen?
            else if(route == RoadPathDirection::Ship)
            {
                // Uns in den Hafen einquartieren
                noBase* hb = world->getNO(pos);
                if(hb->getGOT() != GO_Type::NobHarborbuilding)
                {
                    // Es gibt keinen Hafen mehr -> nach Hause gehen

                    // Arbeitsplatz oder Lagerhaus Bescheid sagen
                    abrogate();
                    // Wir gehen jetzt nach Hause
                    goHome();
                    // Evtl wurde kein Lagerhaus gefunden und wir sollen rumirren, dann tun wir das gleich
                    if(fs == FigureState::Wander)
                        wander();
                    else
                        walkToGoal(); // Nach Hause laufen...
                } else
                {
                    // Uns in den Hafen einquartieren
                    cur_rs = nullptr; // wir laufen nicht mehr auf einer Straße
                    static_cast<nobHarborBuilding*>(hb)->addFigureForShip(world->removeFigure(pos, *this), next_harbor);
                }
            } else
            {
                // Get next street we are walking on
                const Direction walkDir = toDirection(route);
                cur_rs = curRoadNode->getRoute(walkDir);
                startWalking(walkDir);
                rs_pos = 0;
                rs_dir = curRoadNode != cur_rs->getF1();
            }
        }

    } else
    {
        startWalking(cur_rs->getDir(rs_dir, rs_pos));
    }
}

void noFigure::handleEvent(const unsigned id)
{
    // Bei ID = 0 ists ein Laufevent, bei allen anderen an abgeleitete Klassen weiterleiten
    if(id)
    {
        handleDerivedEvent(id);
    } else
    {
        current_ev = nullptr;
        walkFigure();

        // Alte Richtung und Position für die Berechnung der Sichtbarkeiten merken
        const Direction old_dir = getCurMoveDir();

        const MapPoint old_pos(pos);

        switch(fs)
        {
            case FigureState::GoHome:
            case FigureState::GotToGoal: walkToGoal(); break;

            case FigureState::Job: walked(); break;
            case FigureState::Wander: wander(); break;
        }

        // Ggf. Sichtbereich testen
        const unsigned visualRange = getVisualRange();
        if(visualRange)
        {
            // Use old position (don't use this->pos because it might be different now
            // Figure could be in a ship etc.)
            world->recalcMovingVisibilities(old_pos, player, visualRange, old_dir, nullptr);

            // Wenn Figur verschwunden ist, muss ihr ehemaliger gesamter Sichtbereich noch einmal
            // neue berechnet werden
            if(!world->hasFigureAt(old_pos, *this))
                calcVisibilities(old_pos);
        }
    }
}

void noFigure::goHome(noRoadNode* goal)
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
            RTTR_Assert(nobBaseWarehouse::isStorehouseGOT(world->getNO(pos)->getGOT()));
            goal_ = nullptr;
            world->getSpecObj<nobBaseWarehouse>(pos)->cancelFigure(this);
            return;
        } else
            this->goal_ = world->getPlayer(player).findWarehouse((rs_dir) ? *cur_rs->getF1() : *cur_rs->getF2(),
                                                                 FW::AcceptsFigure(job_), true, false);
    } else
        this->goal_ = goal;

    if(this->goal_)
    {
        fs = FigureState::GoHome;
        // Lagerhaus Bescheid sagen
        static_cast<nobBaseWarehouse*>(this->goal_)->addDependentFigure(*this);

        // Wenn wir stehen, zusätzlich noch loslaufen!
        if(waiting_for_free_node)
        {
            waiting_for_free_node = false;
            walkToGoal();
            // anderen Leuten noch ggf Bescheid sagen
            world->roadNodeAvailable(pos);
        }
    } else
    {
        // Kein Lagerhaus gefunden --> Rumirren
        startWandering();
        cur_rs = nullptr;
    }
}

void noFigure::startWandering(const unsigned burned_wh_id)
{
    RTTR_Assert(!goal_);
    fs = FigureState::Wander;
    cur_rs = nullptr;
    rs_pos = 0;
    this->burned_wh_id = burned_wh_id;
    // eine bestimmte Strecke rumirren und dann eine Flagge suchen
    // 3x rumirren und eine Flagge suchen, wenn dann keine gefunden wurde, stirbt die Figur
    wander_way = WANDER_WAY_MIN + RANDOM_RAND(WANDER_WAY_MAX - WANDER_WAY_MIN);
    // Soldaten sind härter im Nehmen
    wander_tryings = isSoldier() ? getStrandedSoldierReturnSearchTryings(world->getGGS()) : WANDER_TRYINGS;

    // Wenn wir stehen, zusätzlich noch loslaufen!
    if(waiting_for_free_node)
    {
        waiting_for_free_node = false;
        // We should be paused, so just continue moving now
        if(isStoppedBetweenNodes())
            startMoving(getCurMoveDir(), getPausedEvent().length);
        else
            wander();
    }
}

namespace {
struct Point2Flag
{
    World& gwb;
    Point2Flag(World& gwb) : gwb(gwb) {}
    noFlag* operator()(const MapPoint pt, unsigned /*r*/) const { return gwb.getSpecObj<noFlag>(pt); }
};

struct IsValidFlag
{
    const unsigned playerId_;
    IsValidFlag(const unsigned playerId) : playerId_(playerId) {}
    bool operator()(const noFlag* const flag) const { return flag && flag->getPlayer() == playerId_; }
};

enum class StrandedSoldierReturnSearchSelection
{
    ReducedRadius = 1,
    WideSecondStage = 2,
    WideThirdStage = 3
};

} // namespace

unsigned short getStrandedSoldierReturnSearchTryings(const GlobalGameSettings& ggs)
{
    switch(static_cast<StrandedSoldierReturnSearchSelection>(ggs.getSelection(AddonId::STRANDED_SOLDIER_RETURN_SEARCH)))
    {
        case StrandedSoldierReturnSearchSelection::WideSecondStage: return 2 * WANDER_TRYINGS_SOLDIERS;
        case StrandedSoldierReturnSearchSelection::WideThirdStage: return 3 * WANDER_TRYINGS_SOLDIERS;
        default: return WANDER_TRYINGS_SOLDIERS;
    }
}

unsigned short getStrandedSoldierReturnSearchRadius(const GlobalGameSettings& ggs, const unsigned short wanderTriesLeft)
{
    switch(static_cast<StrandedSoldierReturnSearchSelection>(ggs.getSelection(AddonId::STRANDED_SOLDIER_RETURN_SEARCH)))
    {
        case StrandedSoldierReturnSearchSelection::ReducedRadius: return STRANDED_SOLDIER_RETURN_SEARCH_RADIUS_REDUCED;
        case StrandedSoldierReturnSearchSelection::WideSecondStage:
            // Start with the normal soldier radius, then widen the final stage after the first six failed searches.
            return wanderTriesLeft > WANDER_TRYINGS_SOLDIERS ? WANDER_RADIUS_SOLDIERS : 2 * WANDER_RADIUS_SOLDIERS;
        case StrandedSoldierReturnSearchSelection::WideThirdStage:
            // Escalate after each stage: normal radius, then 2x, then 4x for the final stage.
            if(wanderTriesLeft > 2 * WANDER_TRYINGS_SOLDIERS)
                return WANDER_RADIUS_SOLDIERS;
            if(wanderTriesLeft > WANDER_TRYINGS_SOLDIERS)
                return 2 * WANDER_RADIUS_SOLDIERS;
            return 4 * WANDER_RADIUS_SOLDIERS;
        default: return WANDER_RADIUS_SOLDIERS;
    }
}

void noFigure::wander()
{
    // Sind wir noch auf der Suche nach einer Flagge?
    if(wander_way == 0xFFFF)
    {
        // Wir laufen schon zur Flagge
        wanderToFlag();
        return;
    }

    // Ist es mal wieder an der Zeit, eine Flagge zu suchen?
    if(!wander_way)
    {
        // Soldaten sind härter im Nehmen
        const unsigned short wander_radius =
          isSoldier() ? getStrandedSoldierReturnSearchRadius(world->getGGS(), wander_tryings) : WANDER_RADIUS;

        // Flaggen sammeln und dann zufällig eine auswählen
        const std::vector<noFlag*> flags =
          world->getPointsInRadius(pos, wander_radius, Point2Flag(*world), IsValidFlag(player));

        unsigned best_way = 0xFFFFFFFF;
        const noFlag* best_flag = nullptr;

        for(auto* flag : flags)
        {
            // Ist das ein Flüchtling aus einem abgebrannten Lagerhaus?
            if(burned_wh_id != 0xFFFFFFFF)
            {
                // Dann evtl gucken, ob anderen Mitglieder schon gesagt haben, dass die Flagge nicht zugänglich ist
                if(flag->isImpossibleForBWU(burned_wh_id))
                {
                    // Dann können wir die Flagge überspringen
                    continue;
                }
            }

            // würde die die bisher beste an Weg unterbieten?
            unsigned way = world->calcDistance(pos, flag->getPos());
            if(way < best_way)
            {
                // Are we at that flag or is there a path to it?
                if(way == 0 || world->findHumanPath(pos, flag->getPos(), wander_radius, false, &way))
                {
                    // gucken, ob ein Weg zu einem Warenhaus führt
                    if(world->getPlayer(player).findWarehouse(*flag, FW::AcceptsFigure(job_), true, false))
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
                    flag->impossibleForBWU(burned_wh_id);
                }
            }
        }

        if(best_flag)
        {
            // bestmögliche schließlich nehmen
            wander_way = 0xFFFF;
            flagPos_ = best_flag->getPos();
            flag_obj_id = best_flag->getObjId();
            wanderToFlag();
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
            die();
            return;
        }
    }

    if(walkInRandomDir())
    {
        RTTR_Assert(wander_way > 0);
        --wander_way;
    } else
    {
        // Wir sind eingesperrt! Kein Weg mehr gefunden --> Sterben
        die();
    }
}

bool noFigure::walkInRandomDir()
{
    PathConditionHuman pathChecker(*world);
    // Check all dirs starting with a random one and taking the first possible
    for(const Direction dir : helpers::enumRange(RANDOM_ENUM(Direction)))
    {
        if(pathChecker.isNodeOk(world->getNeighbour(pos, dir)) && pathChecker.isEdgeOk(pos, dir))
        {
            startWalking(dir);
            return true;
        }
    }
    return false;
}

void noFigure::wanderFailedTrade()
{
    dieFailedTrade();
}

void noFigure::wanderToFlag()
{
    // Existiert die Flagge überhaupt noch?
    noBase* no = world->getNO(flagPos_);
    if(no->getObjId() != flag_obj_id)
    {
        // Wenn nicht, wieder normal weiter rumirren
        startWandering();
        wander();
        return;
    }

    // Sind wir schon da?
    if(pos == flagPos_)
    {
        // Gibts noch nen Weg zu einem Lagerhaus?
        RTTR_Assert(world->getSpecObj<noRoadNode>(pos));
        if(nobBaseWarehouse* wh = world->getPlayer(player).findWarehouse(*world->getSpecObj<noRoadNode>(pos),
                                                                         FW::AcceptsFigure(job_), true, false))
        {
            // ja, dann können wir ja hingehen
            goal_ = wh;
            cur_rs = nullptr;
            rs_pos = 0;
            fs = FigureState::GoHome;
            wh->addDependentFigure(*this);
            walkToGoal();
            return;
        } else
        {
            // Wenn nicht, wieder normal weiter rumirren
            startWandering();
            wander();
            return;
        }
    }

    // Weiter zur Flagge gehen
    // Gibts noch nen Weg dahin bzw. existiert die Flagge noch?
    const auto dir = world->findHumanPath(pos, flagPos_, 60, false);
    if(dir)
    {
        // weiter hinlaufen
        startWalking(*dir);
    } else
    {
        // Wenn nicht, wieder normal weiter rumirren
        startWandering();
        wander();
    }
}

void noFigure::correctSplitData(const RoadSegment* const rs2)
{
    // cur_rs entspricht Teilstück 1 !

    // Wenn man sich auf den ersten Teilstück befindet...
    if((rs_pos < cur_rs->getLength() && !rs_dir) || (rs_pos > rs2->getLength() && rs_dir))
    {
        // Nur Position berichtigen
        if(rs_dir)
            rs_pos -= rs2->getLength();
    }

    // Wenn man auf dem 2. steht, ...
    else if((rs_pos > cur_rs->getLength() && !rs_dir) || (rs_pos < rs2->getLength() && rs_dir))
    {
        // Position berichtigen (wenn man in umgekehrter Richtung läuft, beibehalten!)
        if(!rs_dir)
            rs_pos -= cur_rs->getLength();

        // wir laufen auf dem 2. Teilstück
        cur_rs = rs2;
    } else if((rs_pos == cur_rs->getLength() && !rs_dir) || (rs_pos == rs2->getLength() && rs_dir))
    {
        // wir stehen genau in der Mitte
        // abhängig von der Richtung machen, in die man gerade läuft
        if(getCurMoveDir() == rs2->getRoute(0))
        {
            // wir laufen auf dem 2. Teilstück
            cur_rs = rs2;
            // und wir sind da noch am Anfang
            rs_pos = 0;
        } else if(getCurMoveDir() == cur_rs->getRoute(cur_rs->getLength() - 1) + 3u)
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

    correctSplitDataDerived();
}

/// Wird aufgerufen, wenn die Straße unter der Figur geteilt wurde (für abgeleitete Klassen)
void noFigure::correctSplitDataDerived() {}

unsigned noFigure::calcWalkAnimationFrame() const
{
    // If we are waiting for a free node use the 2nd frame, else interpolate
    return waiting_for_free_node ? 2 : GAMECLIENT.interpolate(ASCENT_ANIMATION_STEPS[getAscent()], current_ev) % 8;
}

/// Calculate the index of the current frame for a figure walking in the given direction
/// The sprites start at `imgSetIndex` and contain 8 images per direction, starting at EAST going clockwise.
/// Note that some of the 48 indices might be empty entries in the archive if only specific directions can ever be drawn
unsigned noFigure::calcWalkFrameIndex(const unsigned imgSetIndex, const Direction dir, const unsigned animationStep)
{
    RTTR_Assert(animationStep < 8);
    return imgSetIndex + rttr::enumCast(toImgDir(dir)) * 8 + animationStep;
}

DrawPoint noFigure::interpolateWalkDrawPos(DrawPoint drawPt) const
{
    // Add an offset relative to the starting point calculated by how far we already walked
    // Don't if we are waiting on our starting node
    if(!waiting_for_free_node || isStoppedBetweenNodes())
        drawPt += calcFigurRelative();
    return drawPt;
}

void noFigure::drawWalkingCarrier(DrawPoint drawPt, helpers::OptionalEnum<GoodType> ware, bool fat)
{
    const unsigned ani_step = calcWalkAnimationFrame();
    const GamePlayer& owner = world->getPlayer(player);
    auto& sprite = ware ? LOADER.getCarrierSprite(*ware, fat, getCurMoveDir(), ani_step) :
                          LOADER.getCarrierBobSprite(owner.nation, fat, getCurMoveDir(), ani_step);
    sprite.drawForPlayer(interpolateWalkDrawPos(drawPt), owner.color);
}

void noFigure::drawWalkingBobJobs(DrawPoint drawPt, Job job)
{
    const unsigned ani_step = calcWalkAnimationFrame();
    const GamePlayer& owner = world->getPlayer(player);
    LOADER.getBobSprite(owner.nation, job, getCurMoveDir(), ani_step)
      .drawForPlayer(interpolateWalkDrawPos(drawPt), owner.color);

    if(hasArmor_)
        drawArmor(interpolateWalkDrawPos(drawPt));
}

void noFigure::drawWalking(DrawPoint drawPt, glArchivItem_Bob* file, unsigned id, bool fat)
{
    const unsigned ani_step = calcWalkAnimationFrame();
    drawPt = interpolateWalkDrawPos(drawPt);

    if(file)
        file->draw(id, toImgDir(getCurMoveDir()), fat, ani_step, drawPt, world->getPlayer(player).color);
    drawShadow(drawPt, ani_step, getCurMoveDir());
}

void noFigure::drawWalking(DrawPoint drawPt, const ResourceId& file, unsigned id)
{
    const unsigned ani_step = calcWalkAnimationFrame();
    drawPt = interpolateWalkDrawPos(drawPt);

    LOADER.getPlayerImage(file, calcWalkFrameIndex(id, getCurMoveDir(), ani_step))
      ->drawFull(drawPt, COLOR_WHITE, world->getPlayer(player).color);
    drawShadow(drawPt, ani_step, getCurMoveDir());
}

void noFigure::drawWalking(DrawPoint drawPt)
{
    switch(job_)
    {
        case Job::PackDonkey:
        {
            const unsigned ani_step = calcWalkAnimationFrame();
            drawPt = interpolateWalkDrawPos(drawPt);

            LOADER.getMapTexture(calcWalkFrameIndex(2000, getCurMoveDir(), ani_step))->drawFull(drawPt);
            // donkey shadow
            LOADER.getMapTexture(2048 + rttr::enumCast(getCurMoveDir()) % 3)->drawFull(drawPt, COLOR_SHADOW);
        }
        break;
        case Job::CharBurner: drawWalking(drawPt, "charburner_bobs", 53); break;
        case Job::Vintner:
            drawWalking(drawPt, "wine_bobs", wineaddon::bobIndex[wineaddon::BobTypes::VINTNER_WALKING]);
            break;
        case Job::Winegrower:
            drawWalking(drawPt, "wine_bobs", wineaddon::bobIndex[wineaddon::BobTypes::WINEGROWER_WALKING_WITH_SHOVEL]);
            break;
        case Job::TempleServant:
            drawWalking(drawPt, "wine_bobs", wineaddon::bobIndex[wineaddon::BobTypes::TEMPLESERVANT_WALKING]);
            break;
        case Job::Skinner:
            drawWalking(drawPt, "leather_bobs", leatheraddon::bobIndex[leatheraddon::BobType::SkinnerWalking]);
            break;
        case Job::Tanner:
            drawWalking(drawPt, "leather_bobs", leatheraddon::bobIndex[leatheraddon::BobType::TannerWalking]);
            break;
        case Job::LeatherWorker:
            drawWalking(drawPt, "leather_bobs", leatheraddon::bobIndex[leatheraddon::BobType::LeatherworkerWalking]);
            break;
        default: drawWalkingBobJobs(drawPt, job_); break;
    }
}

void noFigure::drawArmor(DrawPoint drawPt)
{
    if(world->getGGS().isEnabled(AddonId::MILITARY_HITPOINTS))
    {
        SmallFont->draw(drawPt + DrawPoint(7, -20), "+", FontStyle::CENTER, COLOR_RED);
        SmallFont->draw(drawPt + DrawPoint(10, -20), "1", FontStyle::CENTER, COLOR_RED);
    }

    LOADER.getImageN("leather_bobs", leatheraddon::bobIndex[leatheraddon::BobType::ArmorIconAboveArmoredSoldier])
      ->drawFull(drawPt + DrawPoint(0, -22));
}

void noFigure::die()
{
    // Weg mit mir
    getEvMgr().addToKillList(world->removeFigure(pos, *this));
    // ggf. Leiche hinlegen, falls da nix ist
    if(!world->getSpecObj<noBase>(pos))
        world->setNO(pos, new noSkeleton(pos));

    removeFromInventory();

    // Sichtbarkeiten neu berechnen für Erkunder und Soldaten
    calcVisibilities(pos);
}

void noFigure::removeFromInventory()
{
    // Wars ein Bootmann? Dann Boot und Träger abziehen
    if(job_ == Job::BoatCarrier)
    {
        world->getPlayer(player).decreaseInventoryJob(Job::Helper, 1);
        world->getPlayer(player).decreaseInventoryWare(GoodType::Boat, 1);
    } else
    {
        world->getPlayer(player).decreaseInventoryJob(job_, 1);
        if(::isSoldier(getJobType()) && hasArmor_)
            world->getPlayer(player).decreaseInventoryJob(jobEnumToAmoredSoldierEnum(getJobType()), 1);
    }
}

void noFigure::dieFailedTrade()
{
    // Weg mit mir
    getEvMgr().addToKillList(world->removeFigure(pos, *this));
    // ggf. Leiche hinlegen, falls da nix ist
    if(!world->getSpecObj<noBase>(pos))
        world->setNO(pos, new noSkeleton(pos));
}

void noFigure::nodeFreed(const MapPoint pt)
{
    // Stehen wir gerade aus diesem Grund?
    if(!waiting_for_free_node || pt != world->getNeighbour(this->pos, getCurMoveDir()))
        return;

    // Gehen wir in ein Gebäude? Dann wieder ausgleichen, weil wir die Türen sonst doppelt aufmachen!
    if(getCurMoveDir() == Direction::NorthWest
       && world->getNO(world->getNeighbour(this->pos, Direction::NorthWest))->getType() == NodalObjectType::Building)
        world->getSpecObj<noBuilding>(world->getNeighbour(this->pos, Direction::NorthWest))->closeDoor();
    // oder aus einem raus?
    if(getCurMoveDir() == Direction::SouthEast && world->getNO(this->pos)->getType() == NodalObjectType::Building)
        world->getSpecObj<noBuilding>(this->pos)->closeDoor();

    // Wir stehen nun nicht mehr
    waiting_for_free_node = false;

    // Dann loslaufen
    startWalking(getCurMoveDir());

    // anderen Leuten noch ggf Bescheid sagen
    world->roadNodeAvailable(this->pos);
}

void noFigure::abrogate()
{
    // Arbeisplatz oder Laghaus Bescheid sagen
    if(fs == FigureState::GoHome)
    {
        // goal might by nullptr if goal was a harbor that got destroyed during sea travel
        if(goal_)
        {
            checkedCast<nobBaseWarehouse*>(goal_)->removeDependentFigure(*this);
            goal_ = nullptr;
        } else
        {
            if(!on_ship) // no goal but going home - should not happen
            {
                LOG.write("noFigure::Abrogate - GOHOME figure has no goal and is not on a ship - player %i state %i "
                          "pos %u,%u \n")
                  % player % rttr::enumCast(fs) % pos.x % pos.y;
                // RTTR_Assert(false);
            }
        }
    } else
    {
        goal_ = nullptr;
        abrogateWorkplace();
    }
}

void noFigure::stopIfNecessary(const MapPoint pt)
{
    // Lauf ich auf Wegen --> wenn man zum Ziel oder Weg läuft oder die Träger, die natürlich auch auf Wegen arbeiten
    if(fs == FigureState::GoHome || fs == FigureState::GotToGoal
       || (fs == FigureState::Job && getGOT() == GO_Type::NofCarrier))
    {
        // Laufe ich zu diesem Punkt?
        if(current_ev && !waiting_for_free_node && world->getNeighbour(this->pos, getCurMoveDir()) == pt)
        {
            // Dann stehenbleiben
            pauseWalking();
            waiting_for_free_node = true;
            world->stopOnRoads(this->pos, getCurMoveDir());
        }
    }
}

/// Sichtbarkeiten berechnen für Figuren mit Sichtradius (Soldaten, Erkunder) vor dem Laufen
void noFigure::calcVisibilities(const MapPoint pt)
{
    const unsigned visualRange = getVisualRange();
    if(visualRange)
        // An alter Position neu berechnen
        world->recalcVisibilitiesAroundPoint(pt, visualRange, player, nullptr);
}

/// Informiert die Figur, dass für sie eine Schiffsreise beginnt
void noFigure::startShipJourney()
{
    // We should not be in the world, as we start the journey from a harbor -> We are in that harbor
    RTTR_Assert(!world->hasFigureAt(pos, *this));
    RTTR_Assert(!on_ship);

    pos = MapPoint::invalid();
    on_ship = true;
}

void noFigure::arrivedByShip(const MapPoint harborPos)
{
    RTTR_Assert(on_ship);
    pos = harborPos;
    on_ship = false;
}

/// Examines the route (maybe harbor, road destroyed?) before start shipping
MapPoint noFigure::examineRouteBeforeShipping(RoadPathDirection& newDir)
{
    MapPoint next_harbor;
    // Calc new route
    const noRoadNode* roadNode = world->getSpecObj<noRoadNode>(pos);
    if(!roadNode || !goal_)
        newDir = RoadPathDirection::None;
    else
        newDir = world->findHumanPathOnRoads(*roadNode, *goal_, nullptr, &next_harbor);

    if(newDir == RoadPathDirection::None)
        abrogate();

    // Going by ship?
    if(newDir == RoadPathDirection::Ship)
        // All ok, return next harbor (could be another one!)
        return next_harbor;
    else
        return MapPoint(0, 0);
}
