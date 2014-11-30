// $Id: noAnimal.cpp 9521 2014-11-30 23:31:02Z marcus $
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
#include "noAnimal.h"
#include "Swap.h"
#include "Loader.h"
#include "macros.h"
#include "GameClient.h"
#include "Random.h"
#include "EventManager.h"
#include "GameWorld.h"
#include "nofHunter.h"
#include "VideoDriverWrapper.h"
#include "SerializedGameData.h"

#include "glSmartBitmap.h"

/// Konstruktor
noAnimal::noAnimal(const Species species, const unsigned short x, const unsigned short y) : noMovable(NOP_ANIMAL, x, y)
    , species(species), state(STATE_WALKING), pause_way(5 + RANDOM.Rand(__FILE__, __LINE__, obj_id, 15)), hunter(0), sound_moment(0)
{
    if(hunter)
        hunter->AnimalLost();
}

void noAnimal::Serialize_noAnimal(SerializedGameData* sgd) const
{
    Serialize_noMovable(sgd);

    sgd->PushUnsignedChar(static_cast<unsigned char>(species));
    sgd->PushUnsignedChar(static_cast<unsigned char>(state));
    sgd->PushUnsignedShort(pause_way);
    sgd->PushObject(hunter, true);
}

noAnimal::noAnimal(SerializedGameData* sgd, const unsigned obj_id) : noMovable(sgd, obj_id),
    species(Species(sgd->PopUnsignedChar())),
    state(State(sgd->PopUnsignedChar())),
    pause_way(sgd->PopUnsignedShort()),
    hunter(sgd->PopObject<nofHunter>(GOT_NOF_HUNTER)),
    sound_moment(0)
{
}

void noAnimal::StartLiving()
{
    // anfangen zu laufen
    StandardWalking();
}

void noAnimal::Draw(int x, int y)
{
    // Tier zeichnen

    /*char str[255];
    sprintf(str,"%u",obj_id);
    LOADER.GetFontN("resource",0)->Draw(x,y,str,0,0xFFFF0000);*/

    switch(state)
    {
        default:
            break;
        case STATE_WALKINGUNTILWAITINGFORHUNTER:
        case STATE_WALKING:
        {
            // Laufend (bzw. Ente schwimmend zeichnen)

            // Interpolieren zwischen beiden Knotenpunkten
            CalcWalkingRelative(x, y);

            unsigned ani_step = GAMECLIENT.Interpolate(ASCENT_ANIMATION_STEPS[ascent], current_ev) % ANIMALCONSTS[species].animation_steps;

            // Zeichnen
            Loader::animal_cache[species][dir][ani_step].draw(x, y);

            // Bei Enten und Schafen: Soll ein Sound gespielt werden?
            if(species == SPEC_DUCK || species == SPEC_SHEEP)
            {
                unsigned int now = VideoDriverWrapper::inst().GetTickCount();
                // Wurde der Soundzeitpunkt schon überschritten?
                if(now > sound_moment)
                {
                    // Wenns in dem jeweiligen Rahmen liegt, Sound abspielen
                    if((now < sound_moment + 1000) && !GAMECLIENT.IsPaused())
                        LOADER.GetSoundN("sound", (species == SPEC_SHEEP) ? 94 : 95)->Play(50 + rand() % 70, false);

                    // Neuen Zeitpunkt errechnen
                    sound_moment = now + 8000 + rand() % 5000;
                }
            }

        } break;
        case STATE_WAITINGFORHUNTER:
        case STATE_PAUSED:
        {
            // Stehend zeichnen
            Loader::animal_cache[species][dir][0].draw(x, y);
        } break;
        case STATE_DEAD:
        {
            if (Loader::animal_cache[species][0][ANIMAL_MAX_ANIMATION_STEPS].isGenerated())
            {
                Loader::animal_cache[species][0][ANIMAL_MAX_ANIMATION_STEPS].draw(x, y);
            }
        } break;
        case STATE_DISAPPEARING:
        {
            // Alpha-Wert ausrechnen
            unsigned char alpha = 0xFF - GAMECLIENT.Interpolate(0xFF, current_ev);

            // Gibts ein Leichenbild?
            if (Loader::animal_cache[species][0][ANIMAL_MAX_ANIMATION_STEPS].isGenerated())
            {
                Loader::animal_cache[species][0][ANIMAL_MAX_ANIMATION_STEPS].draw(x, y, SetAlpha(COLOR_WHITE, alpha));
            }
            else if (dir < 6)
            {
                // Stehend zeichnen
                Loader::animal_cache[species][dir][0].draw(x, y, SetAlpha(COLOR_WHITE, alpha));
            }

        } break;
    }

}


void noAnimal::HandleEvent(const unsigned int id)
{
    current_ev = 0;

    switch(id)
    {
            // Laufevent
        case 0:
        {
            // neue Position einnehmen
            Walk();

            // entscheiden, was als nächstes zu tun ist
            Walked();
        } break;
        // Warte-Event
        case 1:
        {
            // wieder weiterlaufen
            StandardWalking();
            // state entsprechen setzen, wenn es nich gestorben ist
            if(state != STATE_DEAD)
                state = STATE_WALKING;

        } break;
        // Sterbe-Event
        case 2:
        {
            // nun verschwinden
            current_ev = em->AddEvent(this, 30, 3);
            state = STATE_DISAPPEARING;

            // Jäger ggf. Bescheid sagen (falls der es nicht mehr rechtzeitig schafft, bis ich verwest bin)
            if(hunter)
                hunter->AnimalLost();

        } break;
        // Verschwind-Event
        case 3:
        {
            // von der Karte tilgen
            current_ev = 0;
            gwg->RemoveFigure(this, x, y);
            em->AddToKillList(this);
        } break;

    }

}

void noAnimal::StartWalking(const unsigned char dir)
{
    StartMoving(dir, ANIMALCONSTS[species].speed);
}

void noAnimal::StandardWalking()
{
    // neuen Weg suchen
    if( (dir = FindDir()) == 0xFF)
    {
        // Sterben, weil kein Weg mehr gefunden wurde
        Die();
        // Jäger ggf. Bescheid sagen (falls der es nicht mehr rechtzeitig schafft, bis ich verwest bin)
        if(hunter)
        {
            hunter->AnimalLost();
            hunter = 0;
        }
    }
    else
    {
        // weiterlaufen
        StartWalking(dir);
    }
}

void noAnimal::Walked()
{
    switch(state)
    {
        default:
            break;
        case STATE_WALKINGUNTILWAITINGFORHUNTER:
        {
            // stehenbleiben und auf den Jäger warten
            state = STATE_WAITINGFORHUNTER;
        } break;
        case STATE_WALKING:
        {
            // ein weiteres Stück gelaufen
            --pause_way;

            // Ist es so lange gelaufen, dass es mal wieder eine Pause braucht?
            if(!pause_way)
            {
                // dann stellt es sich hier hin und wartet erstmal eine Weile
                state = STATE_PAUSED;
                pause_way = 5 + RANDOM.Rand(__FILE__, __LINE__, obj_id, 15);
                current_ev = em->AddEvent(this, 50 + RANDOM.Rand(__FILE__, __LINE__, obj_id, 50), 1);
            }
            else
            {
                StandardWalking();
            }

            // Bei Enten und Schafen: Soundzeitpunkt ggf. setzen
            if(species == SPEC_DUCK || species == SPEC_SHEEP)
            {
                unsigned int now = VideoDriverWrapper::inst().GetTickCount();
                // Wurde der Soundzeitpunkt schon überschritten?
                if(now > sound_moment)
                    // Neuen Zeitpunkt errechnen
                    sound_moment = now + 8000 + rand() % 5000;
            }
        } break;
    }
}

unsigned char noAnimal::FindDir()
{
    // mit zufälliger Richtung anfangen
    unsigned doffset = RANDOM.Rand(__FILE__, __LINE__, obj_id, 6);

    for(unsigned char dtmp = 0; dtmp < 6; ++dtmp)
    {
        unsigned char d = (dtmp + doffset) % 6;

        unsigned char t1 = gwg->GetWalkingTerrain1(x, y, d);

		/* Animals are people, too. They should be allowed to cross borders as well!
		
        if(x == 0)
        {
            if(d == 5 || d == 0 || d == 1)
                continue;
        }
        if(x == gwg->GetWidth() - 1)
        {
            if(d >= 2 && d <= 4)
                continue;
        }
        if(y == 0)
        {
            if(d == 1 || d == 2)
                continue;
        }
        if(y == gwg->GetHeight() - 1)
        {
            if(d == 4 || d == 5)
                continue;
        }
        */

        if(species == SPEC_DUCK)
        {
            // Enten schwimmen nur auf dem Wasser --> muss daher Wasser sein (ID 14 = Wasser)
            unsigned char t2 = gwg->GetWalkingTerrain2(x, y, d);
            
            if(t1 == 14 &&
                    t2 == 14)
                return d;
        }
        else if(species == SPEC_POLARBEAR)
        {
            // Polarbären laufen nur auf Schnee rum
            unsigned char t2 = gwg->GetWalkingTerrain2(x, y, d);

            if(t1 == 0 ||
                    t2 == 0)
                return d;
        }
        else
        {
            // Die anderen Tiere dürfen nur auf Wiesen,Savannen usw. laufen, nicht auf Bergen oder in der Wüste!
            if(!((t1 == 3 || (t1 >= 8 && t1 <= 13)) && (t1 == 3 || (t1 >= 8 && t1 <= 13))))
                continue;

            // Außerdem dürfen keine Hindernisse im Weg sein
            unsigned short dst_x = gwg->GetXA(x, y, d), dst_y = gwg->GetYA(x, y, d);
            noBase* no = gwg->GetNO(dst_x, dst_y);

            if(no->GetType() != NOP_NOTHING &&  no->GetType() != NOP_ENVIRONMENT &&  no->GetType() != NOP_TREE)
                continue;

            // Schließlich auch möglichst keine anderen Figuren bzw. Tiere
            if(gwg->GetFigures(dst_x, dst_y).size())
                continue;

            // Und möglichst auch keine Straßen
            bool roads = false;
            for(unsigned char d2 = 0; d2 < 6; ++d2)
            {
                if(gwg->GetPointRoad(dst_x, dst_y, d2))
                {
                    roads = true;
                    break;
                }
            }

            if(!roads)
                return d;
        }
    }

    // kein Weg mehr gefunden
    return 0xFF;
}



bool noAnimal::CanHunted() const
{
    // Enten sowie Tiere, die bereits gejagt werden, oder schon tot daliegen, können nicht gejagt werden
    return (species != SPEC_DUCK && state != STATE_DEAD && state != STATE_DISAPPEARING && !hunter);
}

void noAnimal::BeginHunting(nofHunter* hunter)
{
    this->hunter = hunter;
}


void noAnimal::HunterIsNear(unsigned short* location_x, unsigned short* location_y)
{
    // Steht es gerade?
    if(state == STATE_PAUSED)
    {
        // dann bleibt es einfach stehen und gibt seine jetzigen Koordinaten zurück
        state = STATE_WAITINGFORHUNTER;
        *location_x = x;
        *location_y = y;
        // Warteevent abmelden
        em->RemoveEvent(current_ev);
        current_ev = 0;
    }
    else
    {
        // ansonsten nach dem Laufen stehenbleiben und die Koordinaten zurückgeben von dem Punkt, der erreicht wird
        state = STATE_WALKINGUNTILWAITINGFORHUNTER;
        *location_x = gwg->GetXA(x, y, dir);
        *location_y = gwg->GetYA(x, y, dir);
    }
}

void noAnimal::StopHunting()
{
    // keiner jagt uns mehr
    hunter = 0;

    switch(state)
    {
        default: return;

        case STATE_WAITINGFORHUNTER:
        {
            // wenn wir stehen, zusätzlich loslaufen
            state = STATE_WALKING;
            StandardWalking();
        } break;
        case STATE_WALKINGUNTILWAITINGFORHUNTER:
        {
            // wir können wieder normal weiterlaufen
            state = STATE_WALKING;
        } break;
    }
}



void noAnimal::Die()
{
    // nun bin ich tot
    if(ANIMALCONSTS[species].dead_id)
    {
        // Verwesungsevent
        current_ev = em->AddEvent(this, 300, 2);
        state = STATE_DEAD;
    }
    else
    {
        // Falls keine Verwesungsgrafik --> sofort verschwinden
        current_ev = em->AddEvent(this, 30, 3);
        state = STATE_DISAPPEARING;
    }
}

void noAnimal::Eviscerated()
{
    // Event abmelden
    em->RemoveEvent(current_ev);
    current_ev = 0;
}

