// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofCarrier.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "RoadSegment.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "Ware.h"
#include "enum_cast.hpp"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "ogl/glSmartBitmap.h"
#include "pathfinding/PathConditionHuman.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noRoadNode.h"
#include "gameData/JobConsts.h"
#include "s25util/Log.h"
#include "s25util/MyTime.h"
#include "s25util/colors.h"
#include <array>

///////////////////////////////////////////////////////////////////////////////
// Konstanten

/// Zeitabstand, in dem die Produktivität vom Träger gemessen wird
const unsigned PRODUCTIVITY_GF = 6000;
/// Ab wieviel Prozent Auslastung in Prozent eines Trägers ein Esel kommen soll
const unsigned DONKEY_PRODUCTIVITY = 80;

/// Abstand zur nächsten Animation (Wert ergibt sich aus NEXT_ANIMATION + rand(NEXT_ANIMATION_RANDOM) )
const unsigned NEXT_ANIMATION = 200;        // fest
const unsigned NEXT_ANIMATION_RANDOM = 200; // was noch dazu zufälliges addiert wird

/// Dauer in GF eines Frames
const unsigned FRAME_GF = 3;

/// Animation indices, 1st Dim: small or big, 2nd Dim: Animation, 3rd Dim: Index in map.lst of the frame
static const std::array<std::vector<std::vector<unsigned short>>, 2> ANIMATIONS = {
  {// Small ones
   {
     // Hoola Hoop
     {1745, 1746, 1747, 1748, 1749, 1750, 1751, 1748, 1748, 1747, 1746},
     // Wink
     {1752, 1753, 1754, 1755, 1756, 1757, 1758, 1754, 1753, 1752},
     // Read newspaper
     {1759, 1760, 1761, 1762, 1763, 1763, 1763, 1765, 1763, 1763, 1763, 1765, 1763, 1762, 1765, 1763, 1764, 1764,
      1763, 1763, 1763, 1765, 1765, 1765, 1763, 1763, 1763, 1765, 1763, 1763, 1763, 1765, 1765, 1764, 1761},
     // Yawn
     {1752, 1753, 1754, 1755, 1756, 1757, 1758, 1754, 1753, 1752},
     // Wink
     {1752, 1770, 1771, 1772, 1773, 1772, 1773, 1772, 1773, 1772,
      1773, 1771, 1771, 1773, 1771, 1771, 1771, 1771, 1770, 1752},
   },
   // Fat ones
   {
     // Sneeze
     {1726, 1727, 1728, 1729, 1730, 1730, 1729, 1728, 1727},
     // Chew bubblegum
     {1731, 1732, 1733, 1734, 1734, 1735, 1736, 1737, 1737, 1736, 1736, 1737},
     // Blow bubblegum
     {1738, 1739, 1740, 1739, 1738, 1739, 1740, 1739, 1741, 1742, 1743, 1744},
     // Touch pocket
     {1726, 1766, 1767, 1768, 1769, 1768, 1769, 1768, 1769, 1766, 1767, 1766, 1726},
   }}};

const helpers::EnumArray<Job, CarrierType> JOB_TYPES = {{Job::Helper, Job::PackDonkey, Job::BoatCarrier}};

nofCarrier::nofCarrier(const CarrierType ct, const MapPoint pos, unsigned char player, RoadSegment* workplace,
                       noRoadNode* const goal)
    : noFigure(JOB_TYPES[ct], pos, player, goal), ct(ct), state(CarrierState::FigureWork), fat((RANDOM_RAND(2) != 0)),
      workplace(workplace), carried_ware(nullptr), productivity_ev(nullptr), productivity(0), worked_gf(0),
      since_working_gf(0xFFFFFFFF), next_animation(0)
{}

nofCarrier::nofCarrier(SerializedGameData& sgd, unsigned obj_id)
    : noFigure(sgd, obj_id), ct(sgd.pop<CarrierType>()), state(sgd.pop<CarrierState>()), fat(sgd.popBool()),
      workplace(sgd.popObject<RoadSegment>(GO_Type::Roadsegment)), carried_ware(sgd.popObject<Ware>(GO_Type::Ware)),
      productivity_ev(sgd.popEvent()), productivity(sgd.popUnsignedInt()), worked_gf(sgd.popUnsignedInt()),
      since_working_gf(sgd.popUnsignedInt()), next_animation(0)
{
    if(state == CarrierState::BoatcarrierWanderOnWater)
    {
        if(sgd.getGameDataVersion() < 7)
        {
            shore_path.resize(sgd.popUnsignedInt());
            helpers::popContainer(sgd, shore_path, true);
        } else
            helpers::popContainer(sgd, shore_path);
    }
}

void nofCarrier::serialize(SerializedGameData& sgd) const
{
    noFigure::serialize(sgd);

    sgd.pushEnum<uint8_t>(ct);
    sgd.pushEnum<uint8_t>(state);
    sgd.pushBool(fat);
    sgd.pushObject(workplace, true);
    sgd.pushObject(carried_ware, true);
    sgd.pushEvent(productivity_ev);
    sgd.pushUnsignedInt(productivity);
    sgd.pushUnsignedInt(worked_gf);
    sgd.pushUnsignedInt(since_working_gf);

    if(state == CarrierState::BoatcarrierWanderOnWater)
    {
        helpers::pushContainer(sgd, shore_path);
    }
}

nofCarrier::~nofCarrier() = default;

void nofCarrier::destroy()
{
    RTTR_Assert(!workplace);
    // Ware vernichten (abmelden)
    RTTR_Assert(!carried_ware); // TODO: Check if this is ok so keep the looseWare call below
    looseWare();
    getEvMgr().removeEvent(productivity_ev);

    noFigure::destroy();
}

void nofCarrier::draw(DrawPoint drawPt)
{
    // Unterscheiden, um was für eine Art von Träger es sich handelt
    switch(ct)
    {
        case CarrierType::Normal:
        {
            if(state == CarrierState::WaitForWare
               || (waiting_for_free_node && !isStoppedBetweenNodes() && !carried_ware))
            {
                bool animation = false;

                // Ist es schon Zeit für eine Animation?
                unsigned currentGf = getEvMgr().getCurrentGF();

                if(currentGf >= next_animation)
                {
                    // Animationstype bestimmen
                    unsigned animationId = next_animation % ANIMATIONS[fat ? 1 : 0].size();
                    bool useNewyearsEgg = false;

                    // <Silvesteregg>
                    // day of year, 0-365, accuracy about 1/4 day
                    int doy = (s25util::Time::currentTime() % 31556925) / 86400;

                    // last hours of last or first day of year
                    if((doy > 364) || (doy < 1))
                    {
                        useNewyearsEgg =
                          next_animation % (ANIMATIONS[fat ? 1 : 0].size() + 1) == ANIMATIONS[fat ? 1 : 0].size();
                    }
                    // </Silvesteregg>

                    // Ist die Animation schon vorbei?
                    if((!useNewyearsEgg
                        && (currentGf >= next_animation + ANIMATIONS[fat ? 1 : 0][animationId].size() * FRAME_GF))
                       || (useNewyearsEgg && (currentGf >= next_animation + 32 * 3)))
                    {
                        // Neuen nächsten Animationszeitpunkt bestimmen
                        setNewAnimationMoment();
                    } else
                    {
                        animation = true;

                        if(!useNewyearsEgg)
                        {
                            // Nein, dann Animation abspielen
                            LOADER
                              .getPlayerImage(
                                "rom_bobs",
                                ANIMATIONS[fat ? 1 : 0][animationId][(currentGf - next_animation) / FRAME_GF])
                              ->drawFull(drawPt, COLOR_WHITE, world->getPlayer(player).color);
                        } else // Silvesteregg
                        {
                            glArchivItem_Bitmap_Player* bmp =
                              LOADER.getPlayerImage("firework", (currentGf - next_animation) / 3 + 1);

                            if(bmp)
                                bmp->drawFull(drawPt - DrawPoint(26, 104), COLOR_WHITE, world->getPlayer(player).color);
                            else
                            {
                                setNewAnimationMoment();
                                animation = false;
                            }
                        }
                    }
                }

                if(!animation)
                {
                    LOADER.getCarrierBobSprite(world->getPlayer(player).nation, fat, getCurMoveDir(), 2)
                      .draw(drawPt, COLOR_WHITE, world->getPlayer(player).color);
                } else
                    // Steht und wartet (ohne Ware)
                    drawShadow(drawPt, 0, getCurMoveDir());
            } else if(state == CarrierState::WaitForWareSpace
                      || (waiting_for_free_node && !isStoppedBetweenNodes() && carried_ware))
            {
                // Steht und wartet (mit Ware)
                LOADER.getCarrierSprite(carried_ware->type, fat, getCurMoveDir(), 2)
                  .draw(drawPt, COLOR_WHITE, world->getPlayer(player).color);
            } else
            {
                // Läuft normal mit oder ohne Ware
                if(carried_ware)
                    drawWalkingCarrier(drawPt, carried_ware->type, fat);
                else
                    drawWalkingCarrier(drawPt, std::nullopt, fat);
            }
        }
        break;
        case CarrierType::Donkey:
        {
            if(state == CarrierState::WaitForWare
               || (waiting_for_free_node && !isStoppedBetweenNodes() && !carried_ware))
            {
                // Steht und wartet (ohne Ware)

                // Esel
                LOADER.getDonkeySprite(getCurMoveDir(), 0).draw(drawPt);
            } else if(state == CarrierState::WaitForWareSpace
                      || (waiting_for_free_node && !isStoppedBetweenNodes() && carried_ware))
            {
                //// Steht und wartet (mit Ware)

                // Esel
                LOADER.getDonkeySprite(getCurMoveDir(), 0).draw(drawPt);

                // Ware im Korb zeichnen
                LOADER.getWareDonkeyTex(carried_ware->type)->drawFull(drawPt + WARE_POS_DONKEY[getCurMoveDir()][0]);
            } else
            {
                const unsigned aniStep = calcWalkAnimationFrame();

                drawPt += calcFigurRelative();

                // Läuft normal mit oder ohne Ware

                // Esel
                LOADER.getDonkeySprite(getCurMoveDir(), aniStep).draw(drawPt);

                if(carried_ware)
                {
                    // Ware im Korb zeichnen
                    LOADER.getWareDonkeyTex(carried_ware->type)
                      ->drawFull(drawPt + WARE_POS_DONKEY[getCurMoveDir()][aniStep]);
                }
            }
        }
        break;
        case CarrierType::Boat:
        {
            if(state == CarrierState::FigureWork)
            {
                // Beim normalen Laufen Träger mit Boot über den Schultern zeichnen
                drawWalkingCarrier(drawPt, GoodType::Boat, fat);
            } else if(state == CarrierState::WaitForWare
                      || (waiting_for_free_node && !isStoppedBetweenNodes() && !carried_ware))
            {
                LOADER.getBoatCarrierSprite(getCurMoveDir(), 0)
                  .draw(drawPt, 0xFFFFFFFF, world->getPlayer(player).color);
            } else if(state == CarrierState::WaitForWareSpace
                      || (waiting_for_free_node && !isStoppedBetweenNodes() && carried_ware))
            {
                LOADER.getBoatCarrierSprite(getCurMoveDir(), 0)
                  .draw(drawPt, 0xFFFFFFFF, world->getPlayer(player).color);

                // Ware im Boot zeichnen
                LOADER.getWareDonkeyTex(carried_ware->type)->drawFull(drawPt + WARE_POS_BOAT[getCurMoveDir()]);
            } else
            {
                const unsigned aniStep = calcWalkAnimationFrame();

                drawPt += calcFigurRelative();

                // ruderndes Boot zeichnen
                LOADER.getBoatCarrierSprite(getCurMoveDir(), aniStep)
                  .draw(drawPt, 0xFFFFFFFF, world->getPlayer(player).color);

                // Läuft normal mit oder ohne Ware
                if(carried_ware)
                    // Ware im Boot zeichnen
                    LOADER.getWareDonkeyTex(carried_ware->type)->drawFull(drawPt + WARE_POS_BOAT[getCurMoveDir()]);

                // Sound ggf. abspielen
                if(aniStep == 2)
                    world->getSoundMgr().playNOSound(84, *this, 0);

                last_id = aniStep;
            }
        }
        break;
    }
}

/// Bestimmt neuen Animationszeitpunkt
void nofCarrier::setNewAnimationMoment()
{
    next_animation = getEvMgr().getCurrentGF() + NEXT_ANIMATION + rand() % NEXT_ANIMATION_RANDOM;
}

void nofCarrier::walked()
{
    // Bootssounds ggf. löschen
    if(ct == CarrierType::Boat && state != CarrierState::FigureWork)
        world->getSoundMgr().stopSounds(*this);

    switch(state)
    {
        default: break;
        case CarrierState::GotoMiddleOfRoad:
        {
            // Gibts an der Flagge in der entgegengesetzten Richtung, in die ich laufe, evtl Waren zu tragen
            // (da wir darüber nicht unmittelbar informiert werden!)
            if(workplace->areWareJobs(rs_dir, ct, false))
            {
                // Dann umdrehen und holen
                rs_dir = !rs_dir;
                rs_pos = workplace->getLength() - rs_pos;
                state = CarrierState::FetchWare;

                startWalking(cur_rs->getDir(rs_dir, rs_pos));
            } else if(rs_pos == cur_rs->getLength() / 2 || rs_pos == cur_rs->getLength() / 2 + cur_rs->getLength() % 2)
            {
                // Wir sind in der Mitte angekommen
                state = CarrierState::WaitForWare;
                if(getCurMoveDir() == Direction::West || getCurMoveDir() == Direction::NorthWest
                   || getCurMoveDir() == Direction::SouthWest)
                    faceDir(Direction::SouthWest);
                else
                    faceDir(Direction::SouthEast);

                current_ev = nullptr;

                // Jetzt wird wieder nur rumgegammelt, dann kriegen wir aber evtl keinen schönen IH-AH!
                stopWorking();

                // Animation auf später verschieben, damit die nicht mittendrin startet
                setNewAnimationMoment();
            } else
            {
                // Eventuell laufen wir in die falsche Richtung?
                if(rs_pos > cur_rs->getLength() / 2)
                {
                    rs_dir = !rs_dir;
                    rs_pos = cur_rs->getLength() - rs_pos;
                }

                startWalking(cur_rs->getDir(rs_dir, rs_pos));
            }
        }
        break;
        case CarrierState::FetchWare:
        {
            // Zur Flagge laufen, um die Ware zu holen

            // Sind wir schon da?
            if(rs_pos == cur_rs->getLength())
                // Dann Ware aufnehmnen
                fetchWare(false);
            else
                startWalking(cur_rs->getDir(rs_dir, rs_pos));
        }
        break;
        case CarrierState::CarryWare:
        {
            // Reached target flag?
            if(rs_pos == cur_rs->getLength())
            {
                // Flag just reached
                auto& this_flag = assertNonNull<noFlag>(((rs_dir) ? workplace->getF1() : workplace->getF2()));

                bool calculated = false;

                // Check if the ware should go into the building connected to the flag
                if(wantInBuilding(&calculated))
                {
                    // Walk to building or building site
                    state = CarrierState::CarryWareToBuilding;
                    startWalking(Direction::NorthWest);
                    cur_rs = this_flag.getRoute(Direction::NorthWest);
                    // Set location to next road node, i.e. building
                    carried_ware->carry(
                      assertNonNull((cur_rs->getF1() == &this_flag) ? cur_rs->getF2() : cur_rs->getF1()));
                } else
                {
                    // Put ware at flag if there is space.
                    // If not try swapping it with another ware at that flag
                    if(this_flag.hasSpaceForWare())
                    {
                        carried_ware->waitAtFlag(this_flag);

                        // Calc next route if not done in the wantInBuilding call
                        if(!calculated)
                            carried_ware->recalcRoute();

                        // Put down ware
                        this_flag.addWare(std::move(carried_ware));
                        RTTR_Assert(carried_ware == nullptr);
                        // Check if we can pick up another ware at this flag, else go back to middle of road
                        lookForWares();
                    } else if(workplace->areWareJobs(!rs_dir, ct, true))
                    {
                        // We can swap the ware with another one at this flag

                        // Temporary store carried ware, so we can fetch another one before putting down the old one
                        // which triggers a search for an available carrier which must not be us
                        auto tmpWare = std::move(carried_ware);
                        fetchWare(true);

                        tmpWare->waitAtFlag(this_flag);
                        if(!calculated)
                            tmpWare->recalcRoute();
                        this_flag.addWare(std::move(tmpWare));
                    } else
                    {
                        // No space at flag, go back and try again later
                        state = CarrierState::GoBackFromFlag;
                        rs_dir = !rs_dir;
                        rs_pos = cur_rs->getLength() - rs_pos;
                        startWalking(getCurMoveDir() + 3u);
                    }
                }
            } else if(rs_pos == cur_rs->getLength() - 1)
            {
                // If we are one step before the flag, check if we have to wait for space
                auto* this_flag = static_cast<noFlag*>(((rs_dir) ? workplace->getF1() : workplace->getF2()));
                // If there is space at the flag, or we can carry it directly to the building or swap it with another
                // ware continue to the flag
                if(this_flag->hasSpaceForWare() || wantInBuilding(nullptr) || cur_rs->areWareJobs(!rs_dir, ct, true))
                {
                    startWalking(cur_rs->getDir(rs_dir, rs_pos));
                } else
                {
                    // No space at flag, wait here
                    state = CarrierState::WaitForWareSpace;
                    faceDir(cur_rs->getDir(rs_dir, rs_pos));
                }
            } else
            {
                startWalking(cur_rs->getDir(rs_dir, rs_pos));
            }
        }
        break;
        case CarrierState::CarryWareToBuilding:
        {
            // Ware ablegen
            world->getSpecObj<noRoadNode>(pos)->addWare(std::move(carried_ware));
            RTTR_Assert(carried_ware == nullptr);
            // Wieder zurück zu meinem Weg laufen
            state = CarrierState::LeaveBuilding;
            startWalking(Direction::SouthEast);
        }
        break;
        case CarrierState::LeaveBuilding:
        {
            // So tun, als ob der Träger gerade vom anderen Ende des Weges kommt, damit alles korrekt funktioniert
            cur_rs = workplace;
            faceDir(workplace->getDir(rs_dir, workplace->getLength() - 1));
            lookForWares();
        }
        break;
        case CarrierState::GoBackFromFlag:
        {
            // Wieder umdrehen und so tun, als wären wir gerade normal angekommen
            rs_dir = !rs_dir;
            rs_pos = cur_rs->getLength() - rs_pos;
            state = CarrierState::CarryWare;
            walked();
        }
        break;
        case CarrierState::BoatcarrierWanderOnWater:
        {
            wanderOnWater();
        }
        break;
    }
}

void nofCarrier::lookForWares()
{
    // Gibts an dieser Flagge etwas, das ich tragen muss?
    if(workplace->areWareJobs(!rs_dir, ct, true))
    {
        // Dann soll das FetchWare übernehmen
        fetchWare(false);
    } else if(workplace->areWareJobs(rs_dir, ct, false))
    {
        // Oder evtl auf der anderen Seite?
        state = CarrierState::FetchWare;
        rs_dir = !rs_dir;
        rs_pos = 0;
        startWalking(cur_rs->getDir(rs_dir, rs_pos));
    } else
    {
        // Wieder zurück in die Mitte gehen
        state = CarrierState::GotoMiddleOfRoad;
        rs_dir = !rs_dir;
        rs_pos = 0;
        startWalking(cur_rs->getDir(rs_dir, rs_pos));
    }
}

void nofCarrier::goalReached()
{
    // Erstes Produktivitätsevent anmelden
    productivity_ev = getEvMgr().addEvent(this, PRODUCTIVITY_GF, 1);
    // Wir arbeiten schonmal
    startWorking();

    auto* rn = world->getSpecObj<noRoadNode>(pos);
    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        // noRoadNode * rn = world->getSpecObj<noRoadNode>(x,y);
        if(rn->getRoute(dir) == workplace)
        {
            // Am neuen Arbeitsplatz angekommen
            startWalking(dir);
            cur_rs = workplace;
            rs_pos = 0;
            rs_dir = rn != cur_rs->getF1();

            state = CarrierState::GotoMiddleOfRoad;

            // Wenn hier schon Waren liegen, diese gleich transportieren
            if(workplace->areWareJobs(rs_dir, ct, true))
            {
                // Ware aufnehmen
                carried_ware = static_cast<noFlag*>(rn)->selectWare(getCurMoveDir(), false, this);

                if(carried_ware)
                {
                    carried_ware->carry(assertNonNull(rs_dir ? workplace->getF1() : workplace->getF2()));
                    state = CarrierState::CarryWare;
                }
            }
            // wenn was an der gegenüberliegenden Flaggge liegt, ebenfalls holen
            else if(workplace->areWareJobs(!rs_dir, ct, false))
                state = CarrierState::FetchWare;
            return;
        }
    }

    LOG.write("nofCarrier::GoalReached: ERROR: Road of carrier (id: %u) not found!\n") % getObjId();
}

void nofCarrier::abrogateWorkplace()
{
    if(workplace)
    {
        getEvMgr().removeEvent(productivity_ev);

        // anderen Träger herausfinden
        unsigned other = (ct == CarrierType::Donkey) ? 0 : 1;

        // wenn ich in ein Gebäude gegangen bin und dann vom Weg geworfen wurde, muss der andere
        // ggf. die Waren tragen, die ich jetzt nicht mehr tragen kann
        if((state == CarrierState::LeaveBuilding || state == CarrierState::CarryWareToBuilding)
           && workplace->hasCarrier(other))
        {
            if(workplace->areWareJobs(false, ct, true))
                workplace->getCarrier(other)->addWareJob(workplace->getF1());
            else if(workplace->areWareJobs(true, ct, true))
                workplace->getCarrier(other)->addWareJob(workplace->getF2());
        }

        workplace->carrierAbrogated(this);
        workplace = nullptr;
        looseWare();

        state = CarrierState::FigureWork;
    }
}

void nofCarrier::looseWare()
{
    // Wenn ich noch ne Ware in der Hand habe, muss die gelöscht werden
    if(carried_ware)
    {
        carried_ware->wareLost(player);
        destroyAndDelete(carried_ware);
    }
}

namespace {
struct IsCoastalAndForFigs
{
    const World& world;
    IsCoastalAndForFigs(const World& world) : world(world) {}

    bool operator()(const MapPoint& pt) const
    {
        return world.getSeaFromCoastalPoint(pt) && PathConditionHuman(world).isNodeOk(pt);
    }
};
} // namespace

void nofCarrier::lostWork()
{
    workplace = nullptr;
    getEvMgr().removeEvent(productivity_ev);

    if(state == CarrierState::FigureWork)
        goHome();
    else
    {
        // Wenn ich noch ne Ware in der Hand habe, muss die gelöscht werden
        looseWare();

        // Is this a boat carrier (i.e. he is on the water)
        if(ct == CarrierType::Boat)
        {
            MapPoint tmpPos(pos);
            if(state != CarrierState::WaitForWare && state != CarrierState::WaitForWareSpace)
            {
                // If we are walking choose the destination point as start point
                // for the pathfinding!
                tmpPos = world->getNeighbour(tmpPos, getCurMoveDir());
            }

            // Look for the shore
            const unsigned maxNodeDistance = 5;
            const auto coastPoints =
              world->getMatchingPointsInRadius(tmpPos, maxNodeDistance, IsCoastalAndForFigs(*world));
            shore_path.clear();
            for(const auto& it : coastPoints)
            {
                // 10x the node distance should be enough, otherwise it would be to far to paddle
                const unsigned maxDistance = maxNodeDistance * 10;
                if(it == tmpPos || world->findShipPath(tmpPos, it, maxDistance, &shore_path, nullptr))
                {
                    // Ok let's paddle to the coast
                    rs_pos = 0;
                    cur_rs = nullptr;
                    const auto oldState = state;
                    state = CarrierState::BoatcarrierWanderOnWater;
                    if(oldState == CarrierState::WaitForWare || oldState == CarrierState::WaitForWareSpace)
                        wanderOnWater();
                    return;
                }
            }
        }

        startWandering();
        if(state == CarrierState::WaitForWare || state == CarrierState::WaitForWareSpace)
            wander();
    }
    state = CarrierState::FigureWork;
}

void nofCarrier::roadSplitted(RoadSegment* rs1, RoadSegment* rs2)
{
    // Bin ich schon auf meinem Arbeitsplatz (=Straße) oder bin ich erst noch auf dem Weg dorthin?
    if(state == CarrierState::FigureWork)
    {
        // ich gehe erst noch hin, also gucken, welche Flagge ich anvisiert habe und das jeweilige Teilstück dann als
        // Arbeitsstraße
        if(getGoal() == rs1->getF1())
            workplace = rs1;
        else
            workplace = rs2;
    } else if(state == CarrierState::CarryWareToBuilding || state == CarrierState::LeaveBuilding)
    {
        // Wenn ich in ein Gebäude gehen oder rauskomme, auf den Weg gehen, der an dieses Gebäude grenzt
        if(cur_rs->getF1() == rs1->getF1() || cur_rs->getF1() == rs1->getF2())
            workplace = rs1;
        else
            workplace = rs2;
    } else
    {
        // sonst wurde es ja schon entschieden
        workplace = (cur_rs == rs1) ? rs1 : rs2;
    }

    // Sonstige Sachen für jeweilige States unternehmen
    switch(state)
    {
        default: break;
        case CarrierState::WaitForWare:
        {
            // Wenn wir stehen, müssen wir in die Mitte laufen
            state = CarrierState::GotoMiddleOfRoad;
            walked();
        }
        break;
        case CarrierState::FetchWare:
        {
            // Wenn wir zur 2. Flagge vom 1. Wegstück gelaufen sind, können wir das nun vergessen
            if(!workplace->areWareJobs(!rs_dir, ct, false))
                state = CarrierState::GotoMiddleOfRoad;
        }
        break;
    }

    RoadSegment* otherRoad = (workplace == rs1) ? rs2 : rs1;
    unsigned char carrierNr = ct == CarrierType::Donkey ? 1 : 0;

    // Switch road if required
    if(workplace->getCarrier(carrierNr) != this)
    {
        RTTR_Assert(otherRoad->getCarrier(carrierNr) == this); // I should have been on other road
        // Mich als Träger für meinen neuen Arbeitsplatz zuweisen
        workplace->setCarrier(carrierNr, this);
        // Für andere Straße neuen Träger/Esel rufen
        otherRoad->setCarrier(carrierNr, nullptr);
    } else
        RTTR_Assert(otherRoad->getCarrier(carrierNr) == nullptr); // No carrier expected

    if(ct == CarrierType::Normal)
        world->getPlayer(player).findCarrierForRoad(*otherRoad);
    else if(ct == CarrierType::Donkey)
        otherRoad->setCarrier(1, world->getPlayer(player).orderDonkey(*otherRoad));
}

void nofCarrier::handleDerivedEvent(const unsigned id)
{
    switch(id)
    {
        // Produktivitätsevent
        case 1:
        {
            productivity_ev = nullptr;

            // Gucken, ob bis jetzt gearbeitet wurde/wird oder nicht, je nachdem noch was dazuzählen
            if(since_working_gf != 0xFFFFFFFF)
            {
                // Es wurde bis jetzt nicht mehr gearbeitet, das also noch dazuzählen
                worked_gf += static_cast<unsigned short>(getEvMgr().getCurrentGF() - since_working_gf);
                // Zähler zurücksetzen
                since_working_gf = getEvMgr().getCurrentGF();
            }

            // Produktivität ausrechnen
            productivity = worked_gf * 100 / PRODUCTIVITY_GF;

            // Zähler zurücksetzen
            worked_gf = 0;

            // Nächstes Event anmelden
            productivity_ev = getEvMgr().addEvent(this, PRODUCTIVITY_GF, 1);

            // Reif für einen Esel?
            if(productivity >= DONKEY_PRODUCTIVITY && ct == CarrierType::Normal)
                workplace->upgradeDonkeyRoad();
        }
        break;
    }
}

bool nofCarrier::addWareJob(const noRoadNode* rn)
{
    // Wenn wir rumstehen, sollten wir mal loslaufen! ^^und ggf umdrehen, genauso wie beim Laufen in die Mitte
    if(state == CarrierState::WaitForWare || state == CarrierState::GotoMiddleOfRoad)
    {
        // Stimmt die Richtung nicht? Dann umdrehen (geht aber nur, wenn wir stehen!)
        if(rs_dir == workplace->getNodeID(*rn) && state == CarrierState::WaitForWare)
        {
            rs_dir = !rs_dir;
            rs_pos = cur_rs->getLength() - rs_pos;
        }
        // beim Gehen in die Mitte nicht sofort umdrehen!
        else if(rs_dir == workplace->getNodeID(*rn))
            return false;

        // Und loslaufen, wenn wir stehen
        if(state == CarrierState::WaitForWare)
        {
            startWalking(cur_rs->getDir(rs_dir, rs_pos));
            // Endlich wird wieder ordentlich gearbeitet!
            startWorking();
        }

        state = CarrierState::FetchWare;

        // Wir übernehmen den Job
        return true;
    } else if(state == CarrierState::WaitForWareSpace && rs_dir == !workplace->getNodeID(*rn))
    {
        // Wenn wir auf einen freien Platz warten, können wir nun losgehen, da wir ja die Waren dann "tauschen" können
        startWalking(cur_rs->getDir(rs_dir, rs_pos));
        state = CarrierState::CarryWare;

        // Wir übernehmen den Job
        return true;
    }

    // Wir übernehmen den Job nicht
    return false;
}

void nofCarrier::removeWareJob()
{
    if(state == CarrierState::FetchWare)
    {
        // ACHTUNG!!!
        // Muss das if dorthin oder nicht?!
        //// Ist gar keine Ware mehr an der Flagge, zu der ich gehe?
        if(!workplace->areWareJobs(!rs_dir, ct, false))
            //{
            //// Wenn es an der anderen Flagge noch einen gibt, dort hin gehen
            // if(workplace->areWareJobs(rs_dir))
            //{
            //  rs_pos = cur_rs->getLength()-rs_pos-2;
            //  rs_dir = !rs_dir;
            //}
            // else
            //{
            // Gibt garnix mehr zu tragen --> wieder in die Mitte gehen
            state = CarrierState::GotoMiddleOfRoad;
        /*}*/
        /*}*/
    }
}

void nofCarrier::fetchWare(const bool swap_wares)
{
    // Ware aufnehmnen
    carried_ware = world->getSpecObj<noFlag>(pos)->selectWare(getCurMoveDir() + 3u, swap_wares, this);

    if(carried_ware)
    {
        carried_ware->carry(assertNonNull(rs_dir ? workplace->getF2() : workplace->getF1()));
        // Und zum anderen Ende laufen
        state = CarrierState::CarryWare;
        rs_dir = !rs_dir;
        rs_pos = 0;

        startWalking(cur_rs->getDir(rs_dir, rs_pos));
    } else // zurücklaufen lassen
        state = CarrierState::GotoMiddleOfRoad;
}

bool nofCarrier::spaceAtFlag(const bool flag)
{
    // Interessiert uns nur, wenn wir auf einen freien Platz warten
    if(state == CarrierState::WaitForWareSpace && rs_dir == !flag)
    {
        // In Richtung Flagge laufen, um Ware dort abzulegen
        startWalking(cur_rs->getDir(rs_dir, rs_pos));
        state = CarrierState::CarryWare;
        return true;
    } else
        return false;
}

bool nofCarrier::wantInBuilding(bool* calculated)
{
    RoadSegment* rs =
      static_cast<noFlag*>((rs_dir ? cur_rs->getF1() : cur_rs->getF2()))->getRoute(Direction::NorthWest);
    if(!rs)
        return false;

    if(rs->getLength() != 1)
        return false;

    if(calculated)
    {
        *calculated = true;
    }

    carried_ware->recalcRoute();
    return carried_ware->getNextDir() == RoadPathDirection::NorthWest;
}

/// Für Produktivitätsmessungen: fängt an zu arbeiten
void nofCarrier::startWorking()
{
    // Wenn noch kein Zeitpunkt festgesetzt wurde, jetzt merken
    if(since_working_gf == 0xFFFFFFFF)
        since_working_gf = getEvMgr().getCurrentGF();
}

/// Für Produktivitätsmessungen: hört auf zu arbeiten
void nofCarrier::stopWorking()
{
    // Falls wir vorher nicht gearbeitet haben, diese Zeit merken für die Produktivität
    if(since_working_gf != 0xFFFFFFFF)
    {
        worked_gf += static_cast<unsigned short>(getEvMgr().getCurrentGF() - since_working_gf);
        since_working_gf = 0xFFFFFFFF;
    }
}

/// Wird aufgerufen, wenn die Straße unter der Figur geteilt wurde (für abgeleitete Klassen)
void nofCarrier::correctSplitDataDerived()
{
    // Tragen wir eine Ware?
    if(state == CarrierState::CarryWare)
    {
        // Dann die Location von der Ware aktualisieren
        if(!rs_dir)
            carried_ware->carry(assertNonNull(cur_rs->getF2()));
        else
            carried_ware->carry(assertNonNull(cur_rs->getF1()));
    }
}

noRoadNode* nofCarrier::getFirstFlag() const
{
    return workplace ? workplace->getF1() : nullptr;
}
noRoadNode* nofCarrier::getSecondFlag() const
{
    return workplace ? workplace->getF2() : nullptr;
}

/// Boat carrier paddles to the coast after his road was destroyed
void nofCarrier::wanderOnWater()
{
    // Are we already there?
    if(rs_pos == shore_path.size())
    {
        // Start normal wandering at the land
        state = CarrierState::FigureWork;
        startWandering();
        wander();
        shore_path.clear();
    } else
    {
        // Continue paddling to the coast
        startWalking(shore_path[rs_pos]);
        ++rs_pos;
    }
}
