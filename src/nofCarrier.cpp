// $Id: nofCarrier.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "nofCarrier.h"

#include "GameWorld.h"
#include "Random.h"
#include "SerializedGameData.h"
#include "GameClient.h"
#include "GameClientPlayer.h"
#include "SoundManager.h"

#include "Ware.h"
#include "RoadSegment.h"
#include "noRoadNode.h"
#include "noFlag.h"
#include "noBaseBuilding.h"
#include "JobConsts.h"

#include "glSmartBitmap.h"


///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
// Konstanten

/// Zeitabstand, in dem die Produktivität vom Träger gemessen wird
const unsigned PRODUCTIVITY_GF = 6000;
/// Ab wieviel Prozent Auslastung in Prozent eines Trägers ein Esel kommen soll
const unsigned DONKEY_PRODUCTIVITY = 80;



/// Abstand zur nächsten Animation (Wert ergibt sich aus NEXT_ANIMATION + rand(NEXT_ANIMATION_RANDOM) )
const unsigned NEXT_ANIMATION = 200; // fest
const unsigned NEXT_ANIMATION_RANDOM = 200; // was noch dazu zufälliges addiert wird


/// Längen in Frames der Animationenen
const unsigned ANIMATION_FRAME_LENGTHS[2][4] =
{ {11, 10, 35, 20}, {9, 12, 12, 13} };
/// Dauer in GF eines Frames
const unsigned FRAME_GF = 3;

/// Animations-Index (von map.lst)
const unsigned short ANIMATION[2][4][35] =
{
    {
        // dünn
        {1745, 1746, 1747, 1748, 1749, 1750, 1751, 1748, 1748, 1747, 1746}, // Reifenspringen
        // dünn
        {1752, 1753, 1754, 1755, 1756, 1757, 1758, 1754, 1753, 1752}, // Winken
        // dünn
        {
            1759, 1760, 1761, 1762, 1763, 1763, 1763, 1765, 1763, 1763, // Zeitung lesen
            1763, 1765, 1763, 1762, 1765, 1763, 1764, 1764, 1763, 1763,
            1763, 1765, 1765, 1765, 1763, 1763, 1763, 1765, 1763, 1763,
            1763, 1765, 1765, 1764, 1761
        },
        // dünn
        {1752, 1770, 1771, 1772, 1773, 1772, 1773, 1772, 1773, 1772, 1773, 1771, 1771, 1773, 1771, 1771, 1771, 1771, 1770, 1752} // Gähnen
    },
    {
        // dick
        {1726, 1727, 1728, 1729, 1730, 1730, 1729, 1728, 1727}, // Niesen?
        // dick
        {1731, 1732, 1733, 1734, 1734, 1735, 1736, 1737, 1737, 1736, 1736, 1737}, // Kaugummi essen
        // dick
        {1738, 1739, 1740, 1739, 1738, 1739, 1740, 1739, 1741, 1742, 1743, 1744}, // Kaugummi-Blasen machen
        // dick
        {1726, 1766, 1767, 1768, 1769, 1768, 1769, 1768, 1769, 1766, 1767, 1766, 1726}, // auf Hosentasche klopfen?!
    }
};

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p nofCarrier.
 *
 *  @author OLiver
 */

const Job JOB_TYPES[3] = { JOB_HELPER, JOB_PACKDONKEY, JOB_BOATCARRIER };

nofCarrier::nofCarrier(const CarrierType ct, unsigned short x,
                       unsigned short y,
                       unsigned char player,
                       RoadSegment* workplace,
                       noRoadNode* const goal)
    : noFigure(JOB_TYPES[ct], x, y, player, goal), ct(ct),
      state(CARRS_FIGUREWORK), fat( ( RANDOM.Rand(__FILE__, __LINE__, obj_id, 2) ? true : false) ),
      workplace(workplace), carried_ware(NULL), productivity_ev(0),
      productivity(0), worked_gf(0), since_working_gf(0xFFFFFFFF), next_animation(0),
      shore_path(NULL)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
nofCarrier::nofCarrier(SerializedGameData* sgd, unsigned int obj_id)
    : noFigure(sgd, obj_id),
      ct( CarrierType(sgd->PopUnsignedChar()) ),
      state( CarrierState(sgd->PopUnsignedChar()) ),
      fat( sgd->PopBool() ),
      workplace( sgd->PopObject<RoadSegment>(GOT_ROADSEGMENT) ),
      carried_ware( sgd->PopObject<Ware>(GOT_WARE) ),
      productivity_ev(sgd->PopObject<EventManager::Event>(GOT_EVENT)),
      productivity(sgd->PopUnsignedInt()),
      worked_gf(sgd->PopUnsignedInt()),
      since_working_gf(sgd->PopUnsignedInt()),
      next_animation(0),
      shore_path(NULL)
{

    if(state == CARRS_BOATCARRIER_WANDERONWATER)
    {
        shore_path = new std::vector<unsigned char>(sgd->PopUnsignedInt());
        for(unsigned i = 0; i < shore_path->size(); ++i)
            shore_path->at(i) = sgd->PopUnsignedChar();
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void nofCarrier::Serialize_nofCarrier(SerializedGameData* sgd) const
{
    Serialize_noFigure(sgd);

    sgd->PushUnsignedChar(static_cast<unsigned char>(ct));
    sgd->PushUnsignedChar(static_cast<unsigned char>(state));
    sgd->PushBool(fat);
    sgd->PushObject(workplace, true);
    sgd->PushObject(carried_ware, true);
    sgd->PushObject(productivity_ev, true);
    sgd->PushUnsignedInt(productivity);
    sgd->PushUnsignedInt(worked_gf);
    sgd->PushUnsignedInt(since_working_gf);

    if(state == CARRS_BOATCARRIER_WANDERONWATER)
    {
        sgd->PushUnsignedInt(shore_path->size());
        for(unsigned i = 0; i < shore_path->size(); ++i)
            sgd->PushUnsignedChar(shore_path->at(i));
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
nofCarrier::~nofCarrier()
{
    // Ware vernichten (physisch)
    delete carried_ware;
}



///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void nofCarrier::Destroy_nofCarrier()
{
    // Ware vernichten (abmelden)
    if(carried_ware)
    {
        gwg->GetPlayer(player)->RemoveWare(carried_ware);
        gwg->GetPlayer(player)->DecreaseInventoryWare(carried_ware->type, 1);
    }

    em->RemoveEvent(productivity_ev);

    Destroy_noFigure();
}


///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void nofCarrier::Draw(int x, int y)
{
    // Unterscheiden, um was für eine Art von Träger es sich handelt
    switch(ct)
    {
        case CT_NORMAL:
        {
            if(state == CARRS_WAITFORWARE || (waiting_for_free_node && !pause_walked_gf && !carried_ware))
            {
                bool animation = false;

                // Ist es schon Zeit für eine Animation?
                unsigned current_gf = GameClient::inst().GetGFNumber();

                if(current_gf >= next_animation)
                {
                    // Animationstype bestimmen
                    unsigned animation_id = next_animation % 4;

// <Silvesteregg>
                    // day of year, 0-365, accuracy about 1/4 day
                    int doy = (TIME.CurrentTime() % 31556925) / 86400;

                    // last hours of last or first day of year
                    if ((doy > 364) || (doy < 1))
                    {
                        animation_id = next_animation % 5;
                    }
// </Silvesteregg>

                    // Ist die Animation schon vorbei?
                    if (((animation_id < 4) && (current_gf >= next_animation + ANIMATION_FRAME_LENGTHS[fat ? 1 : 0][animation_id]*FRAME_GF)) ||
                            ((animation_id == 4) && (current_gf >= next_animation + 32 * 3)))
                    {
                        // Neuen nächsten Animationszeitpunkt bestimmen
                        SetNewAnimationMoment();
                    }
                    else
                    {
                        animation = true;

                        if (animation_id < 4)
                        {
                            // Nein, dann Animation abspielen
                            LOADER.GetImageN("rom_bobs", ANIMATION[fat ? 1 : 0][animation_id][(current_gf - next_animation) / FRAME_GF])
                            ->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);
                        }
                        else if (animation_id == 4)     // Silvesteregg
                        {
                            glArchivItem_Bitmap* bmp = LOADER.GetImageN("firework", (current_gf - next_animation) / 3 + 1);

                            if (bmp)
                            {
                                bmp->Draw(x - 26, y - 104, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);
                            }
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
                    Loader::bob_jobs_cache[gwg->GetPlayer(player)->nation][fat ? JOB_TYPES_COUNT : 0][dir][2].draw(x, y, COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);
                }
                else
                    // Steht und wartet (ohne Ware)
//                  LOADER.GetBobN("jobs")->Draw(0,dir,fat,2,x,y,COLORS[gwg->GetPlayer(player)->color]);

                    DrawShadow(x, y, 0, dir);
            }
            else if(state == CARRS_WAITFORWARESPACE || (waiting_for_free_node && !pause_walked_gf && carried_ware))
            {
                // Steht und wartet (mit Ware)
                Loader::carrier_cache[carried_ware->type][dir][2][fat].draw(x, y, COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);

                // Japaner-Schild-Animation existiert leider nicht --> Römerschild nehmen
//              LOADER.GetBobN("carrier")->Draw((carried_ware->type==GD_SHIELDJAPANESE)?GD_SHIELDROMANS:carried_ware->type,
//                  dir,fat,2,x,y,COLORS[gwg->GetPlayer(player)->color]);
//              DrawShadow(x,y,0,dir);
            }
            else
            {
                // Läuft normal mit oder ohne Ware
                if(carried_ware)
                    DrawWalkingBobCarrier(x, y, carried_ware->type, fat);
//                  DrawWalking(x,y,LOADER.GetBobN("carrier"),(carried_ware->type==GD_SHIELDJAPANESE)?GD_SHIELDROMANS:carried_ware->type,fat);
                else
                    DrawWalkingBobJobs(x, y, fat ? JOB_TYPES_COUNT : 0);
            }
        } break;
        case CT_DONKEY:
        {

            if(state == CARRS_WAITFORWARE || (waiting_for_free_node && !pause_walked_gf && !carried_ware))
            {
                // Steht und wartet (ohne Ware)

                // Esel
                Loader::donkey_cache[dir][0].draw(x, y);
            }
            else if(state == CARRS_WAITFORWARESPACE || (waiting_for_free_node && !pause_walked_gf && carried_ware))
            {
                //// Steht und wartet (mit Ware)
                //// Japaner-Schild-Animation existiert leider nicht --> Römerschild nehmen

                // Esel
                Loader::donkey_cache[dir][0].draw(x, y);

                // Ware im Korb zeichnen
                LOADER.GetMapImageN(2350 + carried_ware->type)
                ->Draw(x + WARE_POS_DONKEY[(dir) * 16], y + WARE_POS_DONKEY[(dir) * 16 + 1]);
            }
            else
            {
                // Wenn wir warten auf ein freies Plätzchen, müssen wir den stehend zeichnen!
                // Wenn event = 0, dann sind wir mittem auf dem Weg angehalten!
                unsigned ani_step = waiting_for_free_node ? 2 : GAMECLIENT.Interpolate(ASCENT_ANIMATION_STEPS[ascent], current_ev) % 8;

                CalcFigurRelative(x, y);

                // Läuft normal mit oder ohne Ware

                // Esel
                Loader::donkey_cache[dir][ani_step].draw(x, y);

                if(carried_ware)
                {
                    // Ware im Korb zeichnen
                    LOADER.GetMapImageN(2350 + carried_ware->type)
                    ->Draw(x + WARE_POS_DONKEY[dir * 16 + ani_step * 2], y + WARE_POS_DONKEY[dir * 16 + ani_step * 2 + 1]);
                }
            }



        } break;
        case CT_BOAT:
        {
            if(state == CARRS_FIGUREWORK)
            {
                // Beim normalen Laufen Träger mit Boot über den Schultern zeichnen
                DrawWalkingBobCarrier(x, y, GD_BOAT, fat);
//              DrawWalking(x,y,LOADER.GetBobN("carrier"),GD_BOAT,fat);
            }
            else if(state == CARRS_WAITFORWARE || (waiting_for_free_node && !pause_walked_gf && !carried_ware))
            {
                Loader::boat_cache[dir][0].draw(x, y, 0xFFFFFFFF, COLORS[gwg->GetPlayer(player)->color]);
            }
            else if(state == CARRS_WAITFORWARESPACE || (waiting_for_free_node && !pause_walked_gf && carried_ware))
            {
                Loader::boat_cache[dir][0].draw(x, y, 0xFFFFFFFF, COLORS[gwg->GetPlayer(player)->color]);

                // Ware im Boot zeichnen
                LOADER.GetMapImageN(2350 + carried_ware->type)
                ->Draw(x + WARE_POS_BOAT[(dir) * 2], y + WARE_POS_BOAT[(dir) * 2 + 1]);
            }
            else
            {
                // Wenn wir warten auf ein freies Plätzchen, müssen wir den (fest)stehend zeichnen!
                // Wenn event = 0, dann sind wir mittem auf dem Weg angehalten!
                unsigned ani_step = waiting_for_free_node ? 2 : GAMECLIENT.Interpolate(ASCENT_ANIMATION_STEPS[ascent], current_ev) % 8;

                CalcFigurRelative(x, y);

                // ruderndes Boot zeichnen
                Loader::boat_cache[dir][ani_step].draw(x, y, 0xFFFFFFFF, COLORS[gwg->GetPlayer(player)->color]);

                // Läuft normal mit oder ohne Ware
                if(carried_ware)
                    // Ware im Boot zeichnen
                    LOADER.GetMapImageN(2350 + carried_ware->type)
                    ->Draw(x + WARE_POS_BOAT[dir * 2], y + WARE_POS_BOAT[dir * 2 + 1]);

                // Sound ggf. abspielen
                if(ani_step == 2)
                    SoundManager::inst().PlayNOSound(84, this, 0);

                last_id = ani_step;
            }

        } break;
    }
}

/// Bestimmt neuen Animationszeitpunkt
void nofCarrier::SetNewAnimationMoment()
{
    next_animation = GameClient::inst().GetGFNumber() + NEXT_ANIMATION + rand() % NEXT_ANIMATION_RANDOM;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void nofCarrier::Walked()
{
    // Bootssounds ggf. löschen
    if(ct == CT_BOAT && state != CARRS_FIGUREWORK)
        SoundManager::inst().WorkingFinished(this);

    switch(state)
    {
        default:
            break;
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
            }
            else if(rs_pos == cur_rs->GetLength() / 2 || rs_pos == cur_rs->GetLength() / 2 + cur_rs->GetLength() % 2)
            {
                // Wir sind in der Mitte angekommen
                state = CARRS_WAITFORWARE;
                if(dir == 0 || dir == 1 || dir == 5)
                    dir = 5;
                else
                    dir = 4;

                current_ev = 0;

                // Jetzt wird wieder nur rumgegammelt, dann kriegen wir aber evtl keinen schönen IH-AH!
                StopWorking();

                // Animation auf später verschieben, damit die nicht mittendrin startet
                SetNewAnimationMoment();
            }
            else
            {
                // Eventuell laufen wir in die falsche Richtung?
                if(rs_pos > cur_rs->GetLength() / 2)
                {
                    rs_dir = !rs_dir;
                    rs_pos = cur_rs->GetLength() - rs_pos;
                }

                StartWalking(cur_rs->GetDir(rs_dir, rs_pos));
            }
        } break;
        case CARRS_FETCHWARE:
        {
            // Zur Flagge laufen, um die Ware zu holen

            // Sind wir schon da?
            if(rs_pos == cur_rs->GetLength())
                // Dann Ware aufnehmnen
                FetchWare(false);
            else
                StartWalking(cur_rs->GetDir(rs_dir, rs_pos));


        } break;
        case CARRS_CARRYWARE:
        {
            // Sind wir schon da?
            if(rs_pos == cur_rs->GetLength())
            {

                // Flagge, an der wir gerade stehen
                noFlag* this_flag  = static_cast<noFlag*>(((rs_dir) ? workplace->GetF1() : workplace->GetF2()));

                bool calculated = false;

                // Will die Waren jetzt gleich zur Baustelle neben der Flagge?
                if(WantInBuilding(&calculated))
                {
                    // Erst noch zur Baustelle bzw Gebäude laufen
                    state = CARRS_CARRYWARETOBUILDING;
                    StartWalking(1);
                    cur_rs = this_flag->routes[1];
                    // location wird immer auf nächste Flagge gesetzt --> in dem Fall aktualisieren
                    carried_ware->Carry((cur_rs->GetF1() == this_flag) ? cur_rs->GetF2() : cur_rs->GetF1());
                }
                else
                {
                    // Ist an der Flagge noch genügend Platz (wenn wir wieder eine Ware mitnehmen, kann sie auch voll sein)
                    if(this_flag->IsSpaceForWare())
                    {
                        carried_ware->LieAtFlag(this_flag);

                        // Ware soll ihren weiteren Weg berechnen
                        if (!calculated)
                        {
                            carried_ware->RecalcRoute();
                        }

                        // Ware ablegen
                        this_flag->AddWare(carried_ware);
                        // Wir tragen erstmal keine Ware mehr
                        carried_ware = 0;
                        // Gibts an den Flaggen etwas, was ich tragen muss, ansonsten wieder in die Mitte gehen und warten
                        LookForWares();
                    }
                    else if(workplace->AreWareJobs(!rs_dir, ct, true))
                    {
                        // die Flagge ist voll, aber wir können eine Ware mitnehmen, daher erst Ware nehmen und dann erst ablegen

                        // Ware "merken"
                        Ware* tmp_ware = carried_ware;
                        // neue Ware aufnehmen
                        FetchWare(true);

                        // alte Ware ablegen
                        tmp_ware->LieAtFlag(this_flag);

                        if (!calculated)
                        {
                            tmp_ware->RecalcRoute();
                        }
                        this_flag->AddWare(tmp_ware);
                    }
                    else
                    {
                        // wenn kein Platz mehr ist --> wieder umdrehen und zurückgehen
                        state = CARRS_GOBACKFROMFLAG;
                        rs_dir = !rs_dir;
                        rs_pos = cur_rs->GetLength() - rs_pos;
                        StartWalking((dir + 3) % 6);
                    }
                }
            }
            else if(rs_pos == cur_rs->GetLength() - 1)
            {
                // Wenn wir fast da sind, gucken, ob an der Flagge noch ein freier Platz ist
                noFlag* this_flag  = static_cast<noFlag*>(((rs_dir) ? workplace->GetF1() : workplace->GetF2()));

                if(this_flag->IsSpaceForWare() || WantInBuilding(NULL) || cur_rs->AreWareJobs(!rs_dir, ct, true))
                {
                    // Es ist Platz, dann zur Flagge laufen
                    StartWalking(cur_rs->GetDir(rs_dir, rs_pos));
                }
                else
                {
                    // Wenn kein Platz ist, stehenbleiben und warten!
                    state = CARRS_WAITFORWARESPACE;
                    dir = cur_rs->GetDir(rs_dir, rs_pos);
                }
            }
            else
            {
                StartWalking(cur_rs->GetDir(rs_dir, rs_pos));
            }

        } break;
        case CARRS_CARRYWARETOBUILDING:
        {
            // Ware ablegen
            gwg->GetSpecObj<noRoadNode>(x, y)->AddWare(carried_ware);
            // Ich trag' keine Ware mehr
            carried_ware = 0;
            // Wieder zurück zu meinem Weg laufen
            state = CARRS_LEAVEBUILDING;
            StartWalking(4);
        } break;
        case CARRS_LEAVEBUILDING:
        {
            // So tun, als ob der Träger gerade vom anderen Ende des Weges kommt, damit alles korrekt funktioniert
            cur_rs = workplace;
            dir = workplace->GetDir(rs_dir, workplace->GetLength() - 1);
            LookForWares();
        } break;
        case CARRS_GOBACKFROMFLAG:
        {
            // Wieder umdrehen und so tun, als wären wir gerade normal angekommen
            rs_dir = !rs_dir;
            rs_pos = cur_rs->GetLength() - rs_pos;
            state = CARRS_CARRYWARE;
            Walked();
        } break;
        case CARRS_BOATCARRIER_WANDERONWATER:
        {
            WanderOnWater();
        } break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void nofCarrier::LookForWares()
{
    // Gibts an dieser Flagge etwas, das ich tragen muss?
    if(workplace->AreWareJobs(!rs_dir, ct, true))
    {
        // Dann soll das CARRS_FETCHWARE übernehmen
        FetchWare(false);
    }
    else if(workplace->AreWareJobs(rs_dir, ct, false))
    {
        // Oder evtl auf der anderen Seite?
        state = CARRS_FETCHWARE;
        rs_dir = !rs_dir;
        rs_pos = 0;
        StartWalking(cur_rs->GetDir(rs_dir, rs_pos));
    }
    else
    {
        // Wieder zurück in die Mitte gehen
        state = CARRS_GOTOMIDDLEOFROAD;
        rs_dir = !rs_dir;
        rs_pos = 0;
        StartWalking(cur_rs->GetDir(rs_dir, rs_pos));
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void nofCarrier::GoalReached()
{
    // Erstes Produktivitätsevent anmelden
    productivity_ev = em->AddEvent(this, PRODUCTIVITY_GF, 1);
    // Wir arbeiten schonmal
    StartWorking();

    noRoadNode* rn = gwg->GetSpecObj<noRoadNode>(x, y);
    for(unsigned char i = 0; i < 6; ++i)
    {
        //noRoadNode * rn = gwg->GetSpecObj<noRoadNode>(x,y);
        if(rn->routes[i] == workplace)
        {
            // Am neuen Arbeitsplatz angekommen
            StartWalking(i);
            cur_rs = workplace;
            rs_pos = 0;
            rs_dir = (rn == cur_rs->GetF1()) ? false : true;

            state = CARRS_GOTOMIDDLEOFROAD;

            // Wenn hier schon Waren liegen, diese gleich transportieren
            if(workplace->AreWareJobs(rs_dir, ct, true))
            {
                // Ware aufnehmen
                carried_ware = static_cast<noFlag*>(rn)->SelectWare(dir, false, this);

                carried_ware->Carry( (rs_dir ? workplace->GetF1() : workplace->GetF2()) );

                if(carried_ware != NULL)
                    state = CARRS_CARRYWARE;
            }
            // wenn was an der gegenüberliegenden Flaggge liegt, ebenfalls holen
            else if(workplace->AreWareJobs(!rs_dir, ct, false))
                state = CARRS_FETCHWARE;
            return;
        }
    }

    LOG.lprintf("nofCarrier::GoalReached: ERROR: Road of carrier (id: %u) not found!\n", obj_id);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void nofCarrier::AbrogateWorkplace()
{
    if(workplace)
    {
        em->RemoveEvent(productivity_ev);
        productivity_ev = 0;

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
        workplace = 0;
        // Wenn ich noch ne Ware in der Hand habe, muss die gelöscht werden
        if(carried_ware)
        {
            carried_ware->WareLost(player);
            delete carried_ware;
            carried_ware = 0;
        }

        state = CARRS_FIGUREWORK;
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void nofCarrier::LostWork()
{
    workplace = 0;
    em->RemoveEvent(productivity_ev);
    productivity_ev = 0;

    if(state == CARRS_FIGUREWORK)
        GoHome();
    else
    {
        // Wenn ich noch ne Ware in der Hand habe, muss die gelöscht werden
        if(carried_ware)
        {
            carried_ware->WareLost(player);
            delete carried_ware;
            carried_ware = 0;
        }

        // Is this a boat carrier (i.e. he is on the water)
        if(ct == CT_BOAT)
        {
            Point<MapCoord> tmp_pos(x, y);
            if(state != CARRS_WAITFORWARE && state != CARRS_WAITFORWARESPACE)
            {
                // If we are walking choose the destination point as start point
                // for the pathfinding!
                gwg->GetPointA(tmp_pos.x, tmp_pos.y, dir);
            }



            // Look for the shore
            for(MapCoord tx = gwg->GetXA(tmp_pos.x, tmp_pos.y, 0), r = 1; r <= 5; tx = gwg->GetXA(tx, tmp_pos.y, 0), ++r)
            {

                MapCoord tx2 = tx, ty2 = tmp_pos.y;
                for(unsigned i = 2; i < 8; ++i)
                {
                    for(MapCoord r2 = 0; r2 < r; gwg->GetPointA(tx2, ty2, i % 6), ++r2)
                    {
                        if(gwg->IsCoastalPoint(tx2, ty2) && gwg->IsNodeForFigures(tx2, ty2))
                        {
                            shore_path = new std::vector<unsigned char>;
                            if(gwg->FindShipPath(tmp_pos.x, tmp_pos.y, tx2, ty2, shore_path, NULL))
                            {
                                // Ok let's paddle to the coast
                                rs_pos = 0;
                                cur_rs = NULL;
                                if(state == CARRS_WAITFORWARE || state == CARRS_WAITFORWARESPACE)
                                    WanderOnWater();
                                state = CARRS_BOATCARRIER_WANDERONWATER;
                                return;
                            }
                        }
                    }
                }
            }
        }

        StartWandering();
        if(state == CARRS_WAITFORWARE || state == CARRS_WAITFORWARESPACE)
            Wander();
    }
    state = CARRS_FIGUREWORK;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
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
    }
    else if(state == CARRS_CARRYWARETOBUILDING || state == CARRS_LEAVEBUILDING)
    {
        // Wenn ich in ein Gebäude gehen oder rauskomme, auf den Weg gehen, der an dieses Gebäude grenzt
        if(cur_rs->GetF1() == rs1->GetF1() || cur_rs->GetF1() == rs1->GetF2())
            workplace = rs1;
        else
            workplace = rs2;
    }
    else
    {
        // sonst wurde es ja schon entschieden
        workplace = (cur_rs == rs1) ? rs1 : rs2;
    }

    // Sonstige Sachen für jeweilige States unternehmen
    switch(state)
    {
        default:
            break;
        case CARRS_WAITFORWARE:
        {
            // Wenn wir stehen, müssen wir in die Mitte laufen
            state = CARRS_GOTOMIDDLEOFROAD;
            Walked();
        } break;
        case CARRS_FETCHWARE:
        {
            // Wenn wir zur 2. Flagge vom 1. Wegstück gelaufen sind, können wir das nun vergessen
            if(!workplace->AreWareJobs(!rs_dir, ct, false))
                state = CARRS_GOTOMIDDLEOFROAD;
        } break;
    }

    // Mich als Träger für meinen neuen Arbeitsplatz zuweisen
    workplace->setCarrier(ct == CT_DONKEY ? 1 : 0, this);

    // Für andere Straße neuen Träger/Esel rufen
    RoadSegment* uc_road = ((rs1 == workplace) ? (rs2) : (rs1));
    uc_road->setCarrier(ct == CT_DONKEY ? 1 : 0, NULL);

    if(ct == CT_NORMAL)
        gwg->GetPlayer(player)->FindCarrierForRoad(uc_road);
    else if(ct == CT_DONKEY)
        uc_road->setCarrier(1, gwg->GetPlayer(player)->OrderDonkey(uc_road));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void nofCarrier::HandleDerivedEvent(const unsigned int id)
{
    switch(id)
    {
            // Produktivitätsevent
        case 1:
        {
            productivity_ev = 0;

            // Gucken, ob bis jetzt gearbeitet wurde/wird oder nicht, je nachdem noch was dazuzählen
            if(since_working_gf != 0xFFFFFFFF)
            {
                // Es wurde bis jetzt nicht mehr gearbeitet, das also noch dazuzählen
                worked_gf += static_cast<unsigned short>(GAMECLIENT.GetGFNumber() - since_working_gf);
                // Zähler zurücksetzen
                since_working_gf = GAMECLIENT.GetGFNumber();
            }

            // Produktivität ausrechnen
            productivity = worked_gf * 100 / PRODUCTIVITY_GF;

            // Zähler zurücksetzen
            worked_gf = 0;

            // Nächstes Event anmelden
            productivity_ev = em->AddEvent(this, PRODUCTIVITY_GF, 1);

            // Reif für einen Esel?
            if(productivity >= DONKEY_PRODUCTIVITY && ct == CT_NORMAL)
                workplace->UpgradeDonkeyRoad();

        } break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
bool nofCarrier::AddWareJob(const noRoadNode* rn)
{
    // Wenn wir rumstehen, sollten wir mal loslaufen! ^^und ggf umdrehen, genauso wie beim Laufen in die Mitte
    if(state == CARRS_WAITFORWARE || state == CARRS_GOTOMIDDLEOFROAD)
    {
        // Stimmt die Richtung nicht? Dann umdrehen (geht aber nur, wenn wir stehen!)
        if(rs_dir == workplace->GetNodeID(rn) && state == CARRS_WAITFORWARE)
        {
            rs_dir = !rs_dir;
            // wenn wir zur Mitte laufen, müssen noch 2 von der pos abgezogen werden wegen dem Laufen
            rs_pos = cur_rs->GetLength() - rs_pos - ((state == CARRS_GOTOMIDDLEOFROAD) ? 2 : 0);
        }
        // beim Gehen in die Mitte nicht sofort umdrehen!
        else if(rs_dir == workplace->GetNodeID(rn))
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
    }
    else if(state == CARRS_WAITFORWARESPACE && rs_dir == !workplace->GetNodeID(rn))
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

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
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
            //if(workplace->AreWareJobs(rs_dir))
            //{
            //  rs_pos = cur_rs->GetLength()-rs_pos-2;
            //  rs_dir = !rs_dir;
            //}
            //else
            //{
            // Gibt garnix mehr zu tragen --> wieder in die Mitte gehen
            state = CARRS_GOTOMIDDLEOFROAD;
        /*}*/
        /*}*/
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void nofCarrier::FetchWare(const bool swap_wares)
{
    // Ware aufnehmnen
    carried_ware = gwg->GetSpecObj<noFlag>(x, y)->SelectWare((dir + 3) % 6, swap_wares, this);

    if(carried_ware)
    {
        carried_ware->Carry((rs_dir) ? workplace->GetF2() : workplace->GetF1());
        // Und zum anderen Ende laufen
        state = CARRS_CARRYWARE;
        rs_dir = !rs_dir;
        rs_pos = 0;

        StartWalking(cur_rs->GetDir(rs_dir, rs_pos));
    }
    else // zurücklaufen lassen
        state = CARRS_GOTOMIDDLEOFROAD;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
bool nofCarrier::SpaceAtFlag(const bool flag)
{
    // Interessiert uns nur, wenn wir auf einen freien Platz warten
    if(state == CARRS_WAITFORWARESPACE && rs_dir == !flag)
    {
        // In Richtung Flagge laufen, um Ware dort abzulegen
        StartWalking(cur_rs->GetDir(rs_dir, rs_pos));
        state = CARRS_CARRYWARE;
        return true;
    }
    else
        return false;

}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
bool nofCarrier::WantInBuilding(bool* calculated)
{
    RoadSegment* rs = static_cast<noFlag*>((rs_dir ? cur_rs->GetF1() : cur_rs->GetF2()))->routes[1];
    if(!rs)
        return false;

    if(rs->GetLength() != 1)
        return false;

    if (calculated != NULL)
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
        since_working_gf = GAMECLIENT.GetGFNumber();
}

/// Für Produktivitätsmessungen: hört auf zu arbeiten
void nofCarrier::StopWorking()
{
    // Falls wir vorher nicht gearbeitet haben, diese Zeit merken für die Produktivität
    if(since_working_gf != 0xFFFFFFFF)
    {
        worked_gf += static_cast<unsigned short>(GAMECLIENT.GetGFNumber() - since_working_gf);
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
        if(!rs_dir) carried_ware->Carry(cur_rs->GetF2());
        else carried_ware->Carry(cur_rs->GetF1());
    }
}


noRoadNode* nofCarrier::GetFirstFlag() const
{ return workplace ? workplace->GetF1() : 0; }
noRoadNode* nofCarrier::GetSecondFlag() const
{ return workplace ? workplace->GetF2() : 0; }



/// Boat carrier paddles to the coast after his road was destroyed
void nofCarrier::WanderOnWater()
{
    // Are we already there?
    if(rs_pos == shore_path->size())
    {
        // Start normal wandering at the land
        state = CARRS_FIGUREWORK;
        StartWandering();
        Wander();
        delete shore_path;
        shore_path = NULL;
    }
    else
    {
        // Continue paddling to the coast
        StartWalking(shore_path->at(rs_pos));
        ++rs_pos;
    }
}
