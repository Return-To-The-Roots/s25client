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
#include "nofCarrier.h"

#include "EventManager.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "RoadSegment.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "Ware.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "ogl/glSmartBitmap.h"
#include "pathfinding/PathConditionHuman.h"
#include "random/Random.h"
#include "world/GameWorldGame.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noRoadNode.h"
#include "gameData/JobConsts.h"
#include "s25util/Log.h"
#include "s25util/MyTime.h"
#include "s25util/colors.h"
#include <boost/assign/std/vector.hpp>
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
using AnimationsType = std::array<std::vector<std::vector<unsigned short>>, 2>;

static AnimationsType fillAnimations()
{
    AnimationsType animations;
    using namespace boost::assign; // Adds the vector += operator
    std::vector<unsigned short> idxs;
    // Small ones
    // Hoola Hoop
    idxs += 1745, 1746, 1747, 1748, 1749, 1750, 1751, 1748, 1748, 1747, 1746;
    animations[0] += idxs;
    // Wink
    idxs.clear();
    idxs += 1752, 1753, 1754, 1755, 1756, 1757, 1758, 1754, 1753, 1752;
    animations[0] += idxs;
    // Read newspaper
    idxs.clear();
    idxs += 1759, 1760, 1761, 1762, 1763, 1763, 1763, 1765, 1763, 1763, 1763, 1765, 1763, 1762, 1765, 1763, 1764, 1764, 1763, 1763, 1763,
      1765, 1765, 1765, 1763, 1763, 1763, 1765, 1763, 1763, 1763, 1765, 1765, 1764, 1761;
    animations[0] += idxs;
    // Yawn
    idxs.clear();
    idxs += 1752, 1753, 1754, 1755, 1756, 1757, 1758, 1754, 1753, 1752;
    animations[0] += idxs;
    // Wink
    idxs.clear();
    idxs += 1752, 1770, 1771, 1772, 1773, 1772, 1773, 1772, 1773, 1772, 1773, 1771, 1771, 1773, 1771, 1771, 1771, 1771, 1770, 1752;
    animations[0] += idxs;

    // Fat ones
    // Sneeze
    idxs.clear();
    idxs += 1726, 1727, 1728, 1729, 1730, 1730, 1729, 1728, 1727;
    animations[1] += idxs;
    // Chew bubblegum
    idxs.clear();
    idxs += 1731, 1732, 1733, 1734, 1734, 1735, 1736, 1737, 1737, 1736, 1736, 1737;
    animations[1] += idxs;
    // Blow bubblegum
    idxs.clear();
    idxs += 1738, 1739, 1740, 1739, 1738, 1739, 1740, 1739, 1741, 1742, 1743, 1744;
    animations[1] += idxs;
    // Touch pocket
    idxs.clear();
    idxs += 1726, 1766, 1767, 1768, 1769, 1768, 1769, 1768, 1769, 1766, 1767, 1766, 1726;
    animations[1] += idxs;

    return animations;
}
static const AnimationsType ANIMATIONS = fillAnimations();

const std::array<Job, 3> JOB_TYPES = {{JOB_HELPER, JOB_PACKDONKEY, JOB_BOATCARRIER}};

nofCarrier::nofCarrier(const CarrierType ct, const MapPoint pos, unsigned char player, RoadSegment* workplace, noRoadNode* const goal)
    : noFigure(JOB_TYPES[ct], pos, player, goal), ct(ct), state(CARRS_FIGUREWORK),
      fat((RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 2) != 0)), workplace(workplace), carried_ware(nullptr), productivity_ev(nullptr),
      productivity(0), worked_gf(0), since_working_gf(0xFFFFFFFF), next_animation(0)
{}

nofCarrier::nofCarrier(SerializedGameData& sgd, unsigned obj_id)
    : noFigure(sgd, obj_id), ct(CarrierType(sgd.PopUnsignedChar())), state(CarrierState(sgd.PopUnsignedChar())), fat(sgd.PopBool()),
      workplace(sgd.PopObject<RoadSegment>(GOT_ROADSEGMENT)), carried_ware(sgd.PopObject<Ware>(GOT_WARE)), productivity_ev(sgd.PopEvent()),
      productivity(sgd.PopUnsignedInt()), worked_gf(sgd.PopUnsignedInt()), since_working_gf(sgd.PopUnsignedInt()), next_animation(0)
{
    if(state == CARRS_BOATCARRIER_WANDERONWATER)
    {
        shore_path.resize(sgd.PopUnsignedInt());
        for(auto& it : shore_path)
            it = Direction::fromInt(sgd.PopUnsignedChar());
    }
}

void nofCarrier::Serialize_nofCarrier(SerializedGameData& sgd) const
{
    Serialize_noFigure(sgd);

    sgd.PushUnsignedChar(static_cast<unsigned char>(ct));
    sgd.PushUnsignedChar(static_cast<unsigned char>(state));
    sgd.PushBool(fat);
    sgd.PushObject(workplace, true);
    sgd.PushObject(carried_ware, true);
    sgd.PushEvent(productivity_ev);
    sgd.PushUnsignedInt(productivity);
    sgd.PushUnsignedInt(worked_gf);
    sgd.PushUnsignedInt(since_working_gf);

    if(state == CARRS_BOATCARRIER_WANDERONWATER)
    {
        sgd.PushUnsignedInt(shore_path.size());
        for(auto it : shore_path)
            sgd.PushUnsignedChar(it.toUInt());
    }
}

nofCarrier::~nofCarrier()
{
    // Ware vernichten (physisch)
    delete carried_ware;
}

void nofCarrier::Destroy_nofCarrier()
{
    RTTR_Assert(!workplace);
    // Ware vernichten (abmelden)
    RTTR_Assert(!carried_ware); // TODO: Check if this is ok so keep the LooseWare call below
    LooseWare();
    GetEvMgr().RemoveEvent(productivity_ev);

    Destroy_noFigure();
}

void nofCarrier::Draw(DrawPoint drawPt)
{
    // Unterscheiden, um was für eine Art von Träger es sich handelt
    switch(ct)
    {
        case CT_NORMAL:
        {
            if(state == CARRS_WAITFORWARE || (waiting_for_free_node && !IsStoppedBetweenNodes() && !carried_ware))
            {
                bool animation = false;

                // Ist es schon Zeit für eine Animation?
                unsigned current_gf = GetEvMgr().GetCurrentGF();

                if(current_gf >= next_animation)
                {
                    // Animationstype bestimmen
                    unsigned animation_id = next_animation % ANIMATIONS[fat ? 1 : 0].size();
                    bool useNewyearsEgg = false;

                    // <Silvesteregg>
                    // day of year, 0-365, accuracy about 1/4 day
                    int doy = (s25util::Time::CurrentTime() % 31556925) / 86400;

                    // last hours of last or first day of year
                    if((doy > 364) || (doy < 1))
                    {
                        useNewyearsEgg = next_animation % (ANIMATIONS[fat ? 1 : 0].size() + 1) == ANIMATIONS[fat ? 1 : 0].size();
                    }
                    // </Silvesteregg>

                    // Ist die Animation schon vorbei?
                    if((!useNewyearsEgg && (current_gf >= next_animation + ANIMATIONS[fat ? 1 : 0][animation_id].size() * FRAME_GF))
                       || (useNewyearsEgg && (current_gf >= next_animation + 32 * 3)))
                    {
                        // Neuen nächsten Animationszeitpunkt bestimmen
                        SetNewAnimationMoment();
                    } else
                    {
                        animation = true;

                        if(!useNewyearsEgg)
                        {
                            // Nein, dann Animation abspielen
                            LOADER
                              .GetPlayerImage("rom_bobs", ANIMATIONS[fat ? 1 : 0][animation_id][(current_gf - next_animation) / FRAME_GF])
                              ->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);
                        } else // Silvesteregg
                        {
                            glArchivItem_Bitmap_Player* bmp = LOADER.GetPlayerImage("firework", (current_gf - next_animation) / 3 + 1);

                            if(bmp)
                                bmp->DrawFull(drawPt - DrawPoint(26, 104), COLOR_WHITE, gwg->GetPlayer(player).color);
                            else
                            {
                                SetNewAnimationMoment();
                                animation = false;
                            }
                        }
                    }
                }

                if(!animation)
                {
                    LOADER.bob_jobs_cache[gwg->GetPlayer(player).nation][fat ? NUM_JOB_TYPES : 0][GetCurMoveDir().toUInt()][2].draw(
                      drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);
                } else
                    // Steht und wartet (ohne Ware)
                    DrawShadow(drawPt, 0, GetCurMoveDir());
            } else if(state == CARRS_WAITFORWARESPACE || (waiting_for_free_node && !IsStoppedBetweenNodes() && carried_ware))
            {
                // Steht und wartet (mit Ware)
                LOADER.carrier_cache[carried_ware->type][GetCurMoveDir().toUInt()][2][fat].draw(drawPt, COLOR_WHITE,
                                                                                                gwg->GetPlayer(player).color);
            } else
            {
                // Läuft normal mit oder ohne Ware
                if(carried_ware)
                    DrawWalkingBobCarrier(drawPt, carried_ware->type, fat);
                else
                    DrawWalkingBobJobs(drawPt, fat ? NUM_JOB_TYPES : 0);
            }
        }
        break;
        case CT_DONKEY:
        {
            if(state == CARRS_WAITFORWARE || (waiting_for_free_node && !IsStoppedBetweenNodes() && !carried_ware))
            {
                // Steht und wartet (ohne Ware)

                // Esel
                LOADER.donkey_cache[GetCurMoveDir().toUInt()][0].draw(drawPt);
            } else if(state == CARRS_WAITFORWARESPACE || (waiting_for_free_node && !IsStoppedBetweenNodes() && carried_ware))
            {
                //// Steht und wartet (mit Ware)
                //// Japaner-Schild-Animation existiert leider nicht --> Römerschild nehmen

                // Esel
                LOADER.donkey_cache[GetCurMoveDir().toUInt()][0].draw(drawPt);

                // Ware im Korb zeichnen
                LOADER.GetMapImageN(2350 + carried_ware->type)->DrawFull(drawPt + WARE_POS_DONKEY[GetCurMoveDir().toUInt()][0]);
            } else
            {
                // Wenn wir warten auf ein freies Plätzchen, müssen wir den stehend zeichnen!
                // Wenn event = 0, dann sind wir mittem auf dem Weg angehalten!
                unsigned ani_step = waiting_for_free_node ? 2 : GAMECLIENT.Interpolate(ASCENT_ANIMATION_STEPS[GetAscent()], current_ev) % 8;

                drawPt += CalcFigurRelative();

                // Läuft normal mit oder ohne Ware

                // Esel
                LOADER.donkey_cache[GetCurMoveDir().toUInt()][ani_step].draw(drawPt);

                if(carried_ware)
                {
                    // Ware im Korb zeichnen
                    LOADER.GetMapImageN(2350 + carried_ware->type)->DrawFull(drawPt + WARE_POS_DONKEY[GetCurMoveDir().toUInt()][ani_step]);
                }
            }
        }
        break;
        case CT_BOAT:
        {
            if(state == CARRS_FIGUREWORK)
            {
                // Beim normalen Laufen Träger mit Boot über den Schultern zeichnen
                DrawWalkingBobCarrier(drawPt, GD_BOAT, fat);
            } else if(state == CARRS_WAITFORWARE || (waiting_for_free_node && !IsStoppedBetweenNodes() && !carried_ware))
            {
                LOADER.boat_cache[GetCurMoveDir().toUInt()][0].draw(drawPt, 0xFFFFFFFF, gwg->GetPlayer(player).color);
            } else if(state == CARRS_WAITFORWARESPACE || (waiting_for_free_node && !IsStoppedBetweenNodes() && carried_ware))
            {
                LOADER.boat_cache[GetCurMoveDir().toUInt()][0].draw(drawPt, 0xFFFFFFFF, gwg->GetPlayer(player).color);

                // Ware im Boot zeichnen
                LOADER.GetMapImageN(2350 + carried_ware->type)->DrawFull(drawPt + WARE_POS_BOAT[GetCurMoveDir().toUInt()]);
            } else
            {
                // Wenn wir warten auf ein freies Plätzchen, müssen wir den (fest)stehend zeichnen!
                // Wenn event = 0, dann sind wir mittem auf dem Weg angehalten!
                unsigned ani_step = waiting_for_free_node ? 2 : GAMECLIENT.Interpolate(ASCENT_ANIMATION_STEPS[GetAscent()], current_ev) % 8;

                drawPt += CalcFigurRelative();

                // ruderndes Boot zeichnen
                LOADER.boat_cache[GetCurMoveDir().toUInt()][ani_step].draw(drawPt, 0xFFFFFFFF, gwg->GetPlayer(player).color);

                // Läuft normal mit oder ohne Ware
                if(carried_ware)
                    // Ware im Boot zeichnen
                    LOADER.GetMapImageN(2350 + carried_ware->type)->DrawFull(drawPt + WARE_POS_BOAT[GetCurMoveDir().toUInt()]);

                // Sound ggf. abspielen
                if(ani_step == 2)
                    SOUNDMANAGER.PlayNOSound(84, this, 0);

                last_id = ani_step;
            }
        }
        break;
    }
}

/// Bestimmt neuen Animationszeitpunkt
void nofCarrier::SetNewAnimationMoment()
{
    next_animation = GetEvMgr().GetCurrentGF() + NEXT_ANIMATION + rand() % NEXT_ANIMATION_RANDOM;
}

void nofCarrier::Walked()
{
    // Bootssounds ggf. löschen
    if(ct == CT_BOAT && state != CARRS_FIGUREWORK)
        SOUNDMANAGER.WorkingFinished(this);

    switch(state)
    {
        default: break;
        case CARRS_GOTOMIDDLEOFROAD:
        {
            // Gibts an der Flagge in der entgegengesetzten Richtung, in die ich laufe, evtl Waren zu tragen
            // (da wir darüber nicht unmittelbar informiert werden!)
            if(workplace->AreWareJobs(rs_dir, ct, false))
            {
                // Dann umdrehen und holen
                rs_dir = !rs_dir;
                rs_pos = workplace->GetLength() - rs_pos;
                state = CARRS_FETCHWARE;

                StartWalking(cur_rs->GetDir(rs_dir, rs_pos));
            } else if(rs_pos == cur_rs->GetLength() / 2 || rs_pos == cur_rs->GetLength() / 2 + cur_rs->GetLength() % 2)
            {
                // Wir sind in der Mitte angekommen
                state = CARRS_WAITFORWARE;
                if(GetCurMoveDir() == Direction::WEST || GetCurMoveDir() == Direction::NORTHWEST || GetCurMoveDir() == Direction::SOUTHWEST)
                    FaceDir(Direction::SOUTHWEST);
                else
                    FaceDir(Direction::SOUTHEAST);

                current_ev = nullptr;

                // Jetzt wird wieder nur rumgegammelt, dann kriegen wir aber evtl keinen schönen IH-AH!
                StopWorking();

                // Animation auf später verschieben, damit die nicht mittendrin startet
                SetNewAnimationMoment();
            } else
            {
                // Eventuell laufen wir in die falsche Richtung?
                if(rs_pos > cur_rs->GetLength() / 2)
                {
                    rs_dir = !rs_dir;
                    rs_pos = cur_rs->GetLength() - rs_pos;
                }

                StartWalking(cur_rs->GetDir(rs_dir, rs_pos));
            }
        }
        break;
        case CARRS_FETCHWARE:
        {
            // Zur Flagge laufen, um die Ware zu holen

            // Sind wir schon da?
            if(rs_pos == cur_rs->GetLength())
                // Dann Ware aufnehmnen
                FetchWare(false);
            else
                StartWalking(cur_rs->GetDir(rs_dir, rs_pos));
        }
        break;
        case CARRS_CARRYWARE:
        {
            // Sind wir schon da?
            if(rs_pos == cur_rs->GetLength())
            {
                // Flagge, an der wir gerade stehen
                auto* this_flag = static_cast<noFlag*>(((rs_dir) ? workplace->GetF1() : workplace->GetF2()));

                bool calculated = false;

                // Will die Waren jetzt gleich zur Baustelle neben der Flagge?
                if(WantInBuilding(&calculated))
                {
                    // Erst noch zur Baustelle bzw Gebäude laufen
                    state = CARRS_CARRYWARETOBUILDING;
                    StartWalking(Direction::NORTHWEST);
                    cur_rs = this_flag->GetRoute(Direction::NORTHWEST);
                    // location wird immer auf nächste Flagge gesetzt --> in dem Fall aktualisieren
                    carried_ware->Carry((cur_rs->GetF1() == this_flag) ? cur_rs->GetF2() : cur_rs->GetF1());
                } else
                {
                    // Ist an der Flagge noch genügend Platz (wenn wir wieder eine Ware mitnehmen, kann sie auch voll sein)
                    if(this_flag->IsSpaceForWare())
                    {
                        carried_ware->WaitAtFlag(this_flag);

                        // Ware soll ihren weiteren Weg berechnen
                        if(!calculated)
                        {
                            carried_ware->RecalcRoute();
                        }

                        // Ware ablegen
                        this_flag->AddWare(carried_ware);
                        // Wir tragen erstmal keine Ware mehr
                        carried_ware = nullptr;
                        // Gibts an den Flaggen etwas, was ich tragen muss, ansonsten wieder in die Mitte gehen und warten
                        LookForWares();
                    } else if(workplace->AreWareJobs(!rs_dir, ct, true))
                    {
                        // die Flagge ist voll, aber wir können eine Ware mitnehmen, daher erst Ware nehmen und dann erst ablegen

                        // Ware "merken"
                        Ware* tmp_ware = carried_ware;
                        // neue Ware aufnehmen
                        FetchWare(true);

                        // alte Ware ablegen
                        tmp_ware->WaitAtFlag(this_flag);

                        if(!calculated)
                        {
                            tmp_ware->RecalcRoute();
                        }
                        this_flag->AddWare(tmp_ware);
                    } else
                    {
                        // wenn kein Platz mehr ist --> wieder umdrehen und zurückgehen
                        state = CARRS_GOBACKFROMFLAG;
                        rs_dir = !rs_dir;
                        rs_pos = cur_rs->GetLength() - rs_pos;
                        StartWalking(GetCurMoveDir() + 3u);
                    }
                }
            } else if(rs_pos == cur_rs->GetLength() - 1)
            {
                // Wenn wir fast da sind, gucken, ob an der Flagge noch ein freier Platz ist
                auto* this_flag = static_cast<noFlag*>(((rs_dir) ? workplace->GetF1() : workplace->GetF2()));

                if(this_flag->IsSpaceForWare() || WantInBuilding(nullptr) || cur_rs->AreWareJobs(!rs_dir, ct, true))
                {
                    // Es ist Platz, dann zur Flagge laufen
                    StartWalking(cur_rs->GetDir(rs_dir, rs_pos));
                } else
                {
                    // Wenn kein Platz ist, stehenbleiben und warten!
                    state = CARRS_WAITFORWARESPACE;
                    FaceDir(cur_rs->GetDir(rs_dir, rs_pos));
                }
            } else
            {
                StartWalking(cur_rs->GetDir(rs_dir, rs_pos));
            }
        }
        break;
        case CARRS_CARRYWARETOBUILDING:
        {
            // Ware ablegen
            gwg->GetSpecObj<noRoadNode>(pos)->AddWare(carried_ware);
            // Ich trag' keine Ware mehr
            carried_ware = nullptr;
            // Wieder zurück zu meinem Weg laufen
            state = CARRS_LEAVEBUILDING;
            StartWalking(Direction::SOUTHEAST);
        }
        break;
        case CARRS_LEAVEBUILDING:
        {
            // So tun, als ob der Träger gerade vom anderen Ende des Weges kommt, damit alles korrekt funktioniert
            cur_rs = workplace;
            FaceDir(workplace->GetDir(rs_dir, workplace->GetLength() - 1));
            LookForWares();
        }
        break;
        case CARRS_GOBACKFROMFLAG:
        {
            // Wieder umdrehen und so tun, als wären wir gerade normal angekommen
            rs_dir = !rs_dir;
            rs_pos = cur_rs->GetLength() - rs_pos;
            state = CARRS_CARRYWARE;
            Walked();
        }
        break;
        case CARRS_BOATCARRIER_WANDERONWATER: { WanderOnWater();
        }
        break;
    }
}

void nofCarrier::LookForWares()
{
    // Gibts an dieser Flagge etwas, das ich tragen muss?
    if(workplace->AreWareJobs(!rs_dir, ct, true))
    {
        // Dann soll das CARRS_FETCHWARE übernehmen
        FetchWare(false);
    } else if(workplace->AreWareJobs(rs_dir, ct, false))
    {
        // Oder evtl auf der anderen Seite?
        state = CARRS_FETCHWARE;
        rs_dir = !rs_dir;
        rs_pos = 0;
        StartWalking(cur_rs->GetDir(rs_dir, rs_pos));
    } else
    {
        // Wieder zurück in die Mitte gehen
        state = CARRS_GOTOMIDDLEOFROAD;
        rs_dir = !rs_dir;
        rs_pos = 0;
        StartWalking(cur_rs->GetDir(rs_dir, rs_pos));
    }
}

void nofCarrier::GoalReached()
{
    // Erstes Produktivitätsevent anmelden
    productivity_ev = GetEvMgr().AddEvent(this, PRODUCTIVITY_GF, 1);
    // Wir arbeiten schonmal
    StartWorking();

    auto* rn = gwg->GetSpecObj<noRoadNode>(pos);
    for(unsigned char i = 0; i < 6; ++i)
    {
        // noRoadNode * rn = gwg->GetSpecObj<noRoadNode>(x,y);
        if(rn->GetRoute(Direction::fromInt(i)) == workplace)
        {
            // Am neuen Arbeitsplatz angekommen
            StartWalking(Direction::fromInt(i));
            cur_rs = workplace;
            rs_pos = 0;
            rs_dir = rn != cur_rs->GetF1();

            state = CARRS_GOTOMIDDLEOFROAD;

            // Wenn hier schon Waren liegen, diese gleich transportieren
            if(workplace->AreWareJobs(rs_dir, ct, true))
            {
                // Ware aufnehmen
                carried_ware = static_cast<noFlag*>(rn)->SelectWare(GetCurMoveDir(), false, this);

                if(carried_ware)
                {
                    carried_ware->Carry((rs_dir ? workplace->GetF1() : workplace->GetF2()));
                    state = CARRS_CARRYWARE;
                }
            }
            // wenn was an der gegenüberliegenden Flaggge liegt, ebenfalls holen
            else if(workplace->AreWareJobs(!rs_dir, ct, false))
                state = CARRS_FETCHWARE;
            return;
        }
    }

    LOG.write("nofCarrier::GoalReached: ERROR: Road of carrier (id: %u) not found!\n") % GetObjId();
}

void nofCarrier::AbrogateWorkplace()
{
    if(workplace)
    {
        GetEvMgr().RemoveEvent(productivity_ev);

        // anderen Träger herausfinden
        unsigned other = (ct == CT_DONKEY) ? 0 : 1;

        // wenn ich in ein Gebäude gegangen bin und dann vom Weg geworfen wurde, muss der andere
        // ggf. die Waren tragen, die ich jetzt nicht mehr tragen kann
        if((state == CARRS_LEAVEBUILDING || state == CARRS_CARRYWARETOBUILDING) && workplace->hasCarrier(other))
        {
            if(workplace->AreWareJobs(false, ct, true))
                workplace->getCarrier(other)->AddWareJob(workplace->GetF1());
            else if(workplace->AreWareJobs(true, ct, true))
                workplace->getCarrier(other)->AddWareJob(workplace->GetF2());
        }

        workplace->CarrierAbrogated(this);
        workplace = nullptr;
        LooseWare();

        state = CARRS_FIGUREWORK;
    }
}

void nofCarrier::LooseWare()
{
    // Wenn ich noch ne Ware in der Hand habe, muss die gelöscht werden
    if(carried_ware)
    {
        carried_ware->WareLost(player);
        destroyAndDelete(carried_ware);
    }
}

namespace {
struct IsCoastalAndForFigs
{
    const World& world;
    IsCoastalAndForFigs(const World& world) : world(world) {}

    bool operator()(const MapPoint& pt) const { return world.GetSeaFromCoastalPoint(pt) && PathConditionHuman(world).IsNodeOk(pt); }
};
} // namespace

void nofCarrier::LostWork()
{
    workplace = nullptr;
    GetEvMgr().RemoveEvent(productivity_ev);

    if(state == CARRS_FIGUREWORK)
        GoHome();
    else
    {
        // Wenn ich noch ne Ware in der Hand habe, muss die gelöscht werden
        LooseWare();

        // Is this a boat carrier (i.e. he is on the water)
        if(ct == CT_BOAT)
        {
            MapPoint tmpPos(pos);
            if(state != CARRS_WAITFORWARE && state != CARRS_WAITFORWARESPACE)
            {
                // If we are walking choose the destination point as start point
                // for the pathfinding!
                tmpPos = gwg->GetNeighbour(tmpPos, GetCurMoveDir());
            }

            // Look for the shore
            const unsigned maxNodeDistance = 5;
            std::vector<MapPoint> coastPoints =
              gwg->GetPointsInRadius<-1>(tmpPos, maxNodeDistance, Identity<MapPoint>(), IsCoastalAndForFigs(*gwg));
            for(const auto& it : coastPoints)
            {
                // 10x the node distance should be enough, otherwise it would be to far to paddle
                const unsigned maxDistance = maxNodeDistance * 10;
                if(gwg->FindShipPath(tmpPos, it, maxDistance, &shore_path, nullptr))
                {
                    // Ok let's paddle to the coast
                    rs_pos = 0;
                    cur_rs = nullptr;
                    if(state == CARRS_WAITFORWARE || state == CARRS_WAITFORWARESPACE)
                        WanderOnWater();
                    state = CARRS_BOATCARRIER_WANDERONWATER;
                    return;
                }
            }
        }

        StartWandering();
        if(state == CARRS_WAITFORWARE || state == CARRS_WAITFORWARESPACE)
            Wander();
    }
    state = CARRS_FIGUREWORK;
}

void nofCarrier::RoadSplitted(RoadSegment* rs1, RoadSegment* rs2)
{
    // Bin ich schon auf meinem Arbeitsplatz (=Straße) oder bin ich erst noch auf dem Weg dorthin?
    if(state == CARRS_FIGUREWORK)
    {
        // ich gehe erst noch hin, also gucken, welche Flagge ich anvisiert habe und das jeweilige Teilstück dann als Arbeitsstraße
        if(GetGoal() == rs1->GetF1())
            workplace = rs1;
        else
            workplace = rs2;
    } else if(state == CARRS_CARRYWARETOBUILDING || state == CARRS_LEAVEBUILDING)
    {
        // Wenn ich in ein Gebäude gehen oder rauskomme, auf den Weg gehen, der an dieses Gebäude grenzt
        if(cur_rs->GetF1() == rs1->GetF1() || cur_rs->GetF1() == rs1->GetF2())
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
        case CARRS_WAITFORWARE:
        {
            // Wenn wir stehen, müssen wir in die Mitte laufen
            state = CARRS_GOTOMIDDLEOFROAD;
            Walked();
        }
        break;
        case CARRS_FETCHWARE:
        {
            // Wenn wir zur 2. Flagge vom 1. Wegstück gelaufen sind, können wir das nun vergessen
            if(!workplace->AreWareJobs(!rs_dir, ct, false))
                state = CARRS_GOTOMIDDLEOFROAD;
        }
        break;
    }

    RoadSegment* otherRoad = (workplace == rs1) ? rs2 : rs1;
    unsigned char carrierNr = ct == CT_DONKEY ? 1 : 0;

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

    if(ct == CT_NORMAL)
        gwg->GetPlayer(player).FindCarrierForRoad(otherRoad);
    else if(ct == CT_DONKEY)
        otherRoad->setCarrier(1, gwg->GetPlayer(player).OrderDonkey(otherRoad));
}

void nofCarrier::HandleDerivedEvent(const unsigned id)
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
                worked_gf += static_cast<unsigned short>(GetEvMgr().GetCurrentGF() - since_working_gf);
                // Zähler zurücksetzen
                since_working_gf = GetEvMgr().GetCurrentGF();
            }

            // Produktivität ausrechnen
            productivity = worked_gf * 100 / PRODUCTIVITY_GF;

            // Zähler zurücksetzen
            worked_gf = 0;

            // Nächstes Event anmelden
            productivity_ev = GetEvMgr().AddEvent(this, PRODUCTIVITY_GF, 1);

            // Reif für einen Esel?
            if(productivity >= DONKEY_PRODUCTIVITY && ct == CT_NORMAL)
                workplace->UpgradeDonkeyRoad();
        }
        break;
    }
}

bool nofCarrier::AddWareJob(const noRoadNode* rn)
{
    // Wenn wir rumstehen, sollten wir mal loslaufen! ^^und ggf umdrehen, genauso wie beim Laufen in die Mitte
    if(state == CARRS_WAITFORWARE || state == CARRS_GOTOMIDDLEOFROAD)
    {
        // Stimmt die Richtung nicht? Dann umdrehen (geht aber nur, wenn wir stehen!)
        if(rs_dir == workplace->GetNodeID(*rn) && state == CARRS_WAITFORWARE)
        {
            rs_dir = !rs_dir;
            rs_pos = cur_rs->GetLength() - rs_pos;
        }
        // beim Gehen in die Mitte nicht sofort umdrehen!
        else if(rs_dir == workplace->GetNodeID(*rn))
            return false;

        // Und loslaufen, wenn wir stehen
        if(state == CARRS_WAITFORWARE)
        {
            StartWalking(cur_rs->GetDir(rs_dir, rs_pos));
            // Endlich wird wieder ordentlich gearbeitet!
            StartWorking();
        }

        state = CARRS_FETCHWARE;

        // Wir übernehmen den Job
        return true;
    } else if(state == CARRS_WAITFORWARESPACE && rs_dir == !workplace->GetNodeID(*rn))
    {
        // Wenn wir auf einen freien Platz warten, können wir nun losgehen, da wir ja die Waren dann "tauschen" können
        StartWalking(cur_rs->GetDir(rs_dir, rs_pos));
        state = CARRS_CARRYWARE;

        // Wir übernehmen den Job
        return true;
    }

    // Wir übernehmen den Job nicht
    return false;
}

void nofCarrier::RemoveWareJob()
{
    if(state == CARRS_FETCHWARE)
    {
        // ACHTUNG!!!
        // Muss das if dorthin oder nicht?!
        //// Ist gar keine Ware mehr an der Flagge, zu der ich gehe?
        if(!workplace->AreWareJobs(!rs_dir, ct, false))
            //{
            //// Wenn es an der anderen Flagge noch einen gibt, dort hin gehen
            // if(workplace->AreWareJobs(rs_dir))
            //{
            //  rs_pos = cur_rs->GetLength()-rs_pos-2;
            //  rs_dir = !rs_dir;
            //}
            // else
            //{
            // Gibt garnix mehr zu tragen --> wieder in die Mitte gehen
            state = CARRS_GOTOMIDDLEOFROAD;
        /*}*/
        /*}*/
    }
}

void nofCarrier::FetchWare(const bool swap_wares)
{
    // Ware aufnehmnen
    carried_ware = gwg->GetSpecObj<noFlag>(pos)->SelectWare(GetCurMoveDir() + 3u, swap_wares, this);

    if(carried_ware)
    {
        carried_ware->Carry((rs_dir) ? workplace->GetF2() : workplace->GetF1());
        // Und zum anderen Ende laufen
        state = CARRS_CARRYWARE;
        rs_dir = !rs_dir;
        rs_pos = 0;

        StartWalking(cur_rs->GetDir(rs_dir, rs_pos));
    } else // zurücklaufen lassen
        state = CARRS_GOTOMIDDLEOFROAD;
}

bool nofCarrier::SpaceAtFlag(const bool flag)
{
    // Interessiert uns nur, wenn wir auf einen freien Platz warten
    if(state == CARRS_WAITFORWARESPACE && rs_dir == !flag)
    {
        // In Richtung Flagge laufen, um Ware dort abzulegen
        StartWalking(cur_rs->GetDir(rs_dir, rs_pos));
        state = CARRS_CARRYWARE;
        return true;
    } else
        return false;
}

bool nofCarrier::WantInBuilding(bool* calculated)
{
    RoadSegment* rs = static_cast<noFlag*>((rs_dir ? cur_rs->GetF1() : cur_rs->GetF2()))->GetRoute(Direction::NORTHWEST);
    if(!rs)
        return false;

    if(rs->GetLength() != 1)
        return false;

    if(calculated)
    {
        *calculated = true;
    }

    carried_ware->RecalcRoute();
    return (carried_ware->GetNextDir() == 1);
}

/// Für Produktivitätsmessungen: fängt an zu arbeiten
void nofCarrier::StartWorking()
{
    // Wenn noch kein Zeitpunkt festgesetzt wurde, jetzt merken
    if(since_working_gf == 0xFFFFFFFF)
        since_working_gf = GetEvMgr().GetCurrentGF();
}

/// Für Produktivitätsmessungen: hört auf zu arbeiten
void nofCarrier::StopWorking()
{
    // Falls wir vorher nicht gearbeitet haben, diese Zeit merken für die Produktivität
    if(since_working_gf != 0xFFFFFFFF)
    {
        worked_gf += static_cast<unsigned short>(GetEvMgr().GetCurrentGF() - since_working_gf);
        since_working_gf = 0xFFFFFFFF;
    }
}

/// Wird aufgerufen, wenn die Straße unter der Figur geteilt wurde (für abgeleitete Klassen)
void nofCarrier::CorrectSplitData_Derived()
{
    // Tragen wir eine Ware?
    if(state == CARRS_CARRYWARE)
    {
        // Dann die Location von der Ware aktualisieren
        if(!rs_dir)
            carried_ware->Carry(cur_rs->GetF2());
        else
            carried_ware->Carry(cur_rs->GetF1());
    }
}

noRoadNode* nofCarrier::GetFirstFlag() const
{
    return workplace ? workplace->GetF1() : nullptr;
}
noRoadNode* nofCarrier::GetSecondFlag() const
{
    return workplace ? workplace->GetF2() : nullptr;
}

/// Boat carrier paddles to the coast after his road was destroyed
void nofCarrier::WanderOnWater()
{
    // Are we already there?
    if(rs_pos == shore_path.size())
    {
        // Start normal wandering at the land
        state = CARRS_FIGUREWORK;
        StartWandering();
        Wander();
        shore_path.clear();
    } else
    {
        // Continue paddling to the coast
        StartWalking(shore_path[rs_pos]);
        ++rs_pos;
    }
}
