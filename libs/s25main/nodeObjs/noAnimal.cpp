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

#include "noAnimal.h"
#include "EventManager.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "drivers/VideoDriverWrapper.h"
#include "figures/nofHunter.h"
#include "network/GameClient.h"
#include "ogl/SoundEffectItem.h"
#include "random/Random.h"
#include "world/GameWorldGame.h"
#include "gameData/GameConsts.h"
#include "gameData/TerrainDesc.h"

#include "ogl/glSmartBitmap.h"
#include "s25util/colors.h"

noAnimal::noAnimal(const Species species, const MapPoint pos)
    : noMovable(NodalObjectType::Animal, pos), species(species), state(STATE_WALKING),
      pause_way(5 + RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 15)), hunter(nullptr), sound_moment(0)
{}

void noAnimal::Serialize_noAnimal(SerializedGameData& sgd) const
{
    noMovable::Serialize(sgd);

    sgd.PushEnum<uint8_t>(species);
    sgd.PushUnsignedChar(static_cast<unsigned char>(state));
    sgd.PushUnsignedShort(pause_way);
    sgd.PushObject(hunter, true);
}

noAnimal::noAnimal(SerializedGameData& sgd, const unsigned obj_id)
    : noMovable(sgd, obj_id), species(sgd.Pop<Species>()), state(State(sgd.PopUnsignedChar())),
      pause_way(sgd.PopUnsignedShort()), hunter(sgd.PopObject<nofHunter>(GOT_NOF_HUNTER)), sound_moment(0)
{}

void noAnimal::StartLiving()
{
    // anfangen zu laufen
    StandardWalking();
}

void noAnimal::Draw(DrawPoint drawPt)
{
    // Tier zeichnen

    switch(state)
    {
        default: break;
        case STATE_WALKINGUNTILWAITINGFORHUNTER:
        case STATE_WALKING:
        {
            // Laufend (bzw. Ente schwimmend zeichnen)

            // Interpolieren zwischen beiden Knotenpunkten
            drawPt += CalcWalkingRelative();

            unsigned ani_step = GAMECLIENT.Interpolate(ASCENT_ANIMATION_STEPS[GetAscent()], current_ev)
                                % ANIMALCONSTS[species].animation_steps;

            // Zeichnen
            LOADER.getAnimalSprite(species, GetCurMoveDir(), ani_step).draw(drawPt);

            // Bei Enten und Schafen: Soll ein Sound gespielt werden?
            if(species == Species::Duck || species == Species::Sheep)
            {
                unsigned now = VIDEODRIVER.GetTickCount();
                // Wurde der Soundzeitpunkt schon überschritten?
                if(now > sound_moment)
                {
                    // Wenns in dem jeweiligen Rahmen liegt, Sound abspielen
                    if((now < sound_moment + 1000) && !GAMECLIENT.IsPaused())
                        LOADER.GetSoundN("sound", (species == Species::Sheep) ? 94 : 95)->Play(50 + rand() % 70, false);

                    // Neuen Zeitpunkt errechnen
                    sound_moment = now + 8000 + rand() % 5000;
                }
            }
        }
        break;
        case STATE_WAITINGFORHUNTER:
        case STATE_PAUSED:
        {
            // Stehend zeichnen
            LOADER.getAnimalSprite(species, GetCurMoveDir(), 0).draw(drawPt);
        }
        break;
        case STATE_DEAD:
        {
            if(!LOADER.getDeadAnimalSprite(species).empty())
            {
                LOADER.getDeadAnimalSprite(species).draw(drawPt);
            }
        }
        break;
        case STATE_DISAPPEARING:
        {
            // Alpha-Wert ausrechnen
            unsigned char alpha = 0xFF - GAMECLIENT.Interpolate(0xFF, current_ev);

            // Gibts ein Leichenbild?
            if(!LOADER.getDeadAnimalSprite(species).empty())
            {
                LOADER.getDeadAnimalSprite(species).draw(drawPt, SetAlpha(COLOR_WHITE, alpha));
            } else
            {
                // Stehend zeichnen
                LOADER.getAnimalSprite(species, GetCurMoveDir(), 0).draw(drawPt, SetAlpha(COLOR_WHITE, alpha));
            }
        }
        break;
    }
}

void noAnimal::HandleEvent(const unsigned id)
{
    current_ev = nullptr;

    switch(id)
    {
        // Laufevent
        case 0:
        {
            // neue Position einnehmen
            Walk();

            // entscheiden, was als nächstes zu tun ist
            Walked();
        }
        break;
        // Warte-Event
        case 1:
        {
            // wieder weiterlaufen
            StandardWalking();
            // state entsprechen setzen, wenn es nich gestorben ist
            if(state != STATE_DEAD)
                state = STATE_WALKING;
        }
        break;
        // Sterbe-Event
        case 2:
        {
            // nun verschwinden
            current_ev = GetEvMgr().AddEvent(this, 30, 3);
            state = STATE_DISAPPEARING;

            // Jäger ggf. Bescheid sagen (falls der es nicht mehr rechtzeitig schafft, bis ich verwest bin)
            if(hunter)
            {
                hunter->AnimalLost();
                hunter = nullptr;
            }
        }
        break;
        // Verschwind-Event
        case 3:
        {
            // von der Karte tilgen
            gwg->RemoveFigure(pos, this);
            GetEvMgr().AddToKillList(this);
        }
        break;
    }
}

void noAnimal::StartWalking(const Direction dir)
{
    StartMoving(dir, ANIMALCONSTS[species].speed);
}

void noAnimal::StandardWalking()
{
    // neuen Weg suchen
    const helpers::OptionalEnum<Direction> dir = FindDir();
    if(!dir)
    {
        // Sterben, weil kein Weg mehr gefunden wurde
        Die();
        // Jäger ggf. Bescheid sagen (falls der es nicht mehr rechtzeitig schafft, bis ich verwest bin)
        if(hunter) //-V779
        {
            hunter->AnimalLost();
            hunter = nullptr;
        }
    } else
    {
        // weiterlaufen
        StartWalking(*dir);
    }
}

void noAnimal::Walked()
{
    switch(state)
    {
        default: break;
        case STATE_WALKINGUNTILWAITINGFORHUNTER:
        {
            // stehenbleiben und auf den Jäger warten
            state = STATE_WAITINGFORHUNTER;
        }
        break;
        case STATE_WALKING:
        {
            // ein weiteres Stück gelaufen
            --pause_way;

            // Ist es so lange gelaufen, dass es mal wieder eine Pause braucht?
            if(!pause_way)
            {
                // dann stellt es sich hier hin und wartet erstmal eine Weile
                state = STATE_PAUSED;
                pause_way = 5 + RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 15);
                current_ev = GetEvMgr().AddEvent(this, 50 + RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 50), 1);
            } else
            {
                StandardWalking();
            }

            // Bei Enten und Schafen: Soundzeitpunkt ggf. setzen
            if(species == Species::Duck || species == Species::Sheep)
            {
                unsigned now = VIDEODRIVER.GetTickCount();
                // Wurde der Soundzeitpunkt schon überschritten?
                if(now > sound_moment)
                    // Neuen Zeitpunkt errechnen
                    sound_moment = now + 8000 + rand() % 5000;
            }
        }
        break;
    }
}

helpers::OptionalEnum<Direction> noAnimal::FindDir()
{
    // mit zufälliger Richtung anfangen
    unsigned doffset = RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 6);

    for(auto dir : helpers::EnumRange<Direction>{})
    {
        dir += doffset;

        DescIdx<TerrainDesc> tLeft = gwg->GetLeftTerrain(pos, dir);
        DescIdx<TerrainDesc> tRight = gwg->GetRightTerrain(pos, dir);

        if(species == Species::Duck)
        {
            // Enten schwimmen nur auf dem Wasser --> muss daher Wasser sein
            if(gwg->GetDescription().get(tLeft).kind == TerrainKind::WATER
               && gwg->GetDescription().get(tRight).kind == TerrainKind::WATER)
                return dir;
        } else if(species == Species::PolarBear)
        {
            // Polarbären laufen nur auf Schnee rum
            if(gwg->GetDescription().get(tLeft).kind == TerrainKind::SNOW
               && gwg->GetDescription().get(tRight).kind == TerrainKind::SNOW)
                return dir;
        } else
        {
            // Die anderen Tiere dürfen nur auf Wiesen,Savannen usw. laufen, nicht auf Bergen oder in der Wüste!
            if(!gwg->GetDescription().get(tLeft).IsUsableByAnimals()
               || !gwg->GetDescription().get(tRight).IsUsableByAnimals())
                continue;

            // Außerdem dürfen keine Hindernisse im Weg sein
            MapPoint dst = gwg->GetNeighbour(pos, dir);
            noBase* no = gwg->GetNO(dst);

            if(no->GetType() != NodalObjectType::Nothing && no->GetType() != NodalObjectType::Environment
               && no->GetType() != NodalObjectType::Tree)
                continue;

            // Schließlich auch möglichst keine anderen Figuren bzw. Tiere
            if(!gwg->GetFigures(dst).empty())
                continue;

            // Und möglichst auch keine Straßen
            for(const auto dir2 : helpers::EnumRange<Direction>{})
            {
                if(gwg->GetPointRoad(dst, dir2) != PointRoad::None)
                    return boost::none;
            }
            return dir;
        }
    }

    // kein Weg mehr gefunden
    return boost::none;
}

bool noAnimal::CanHunted() const
{
    // Enten sowie Tiere, die bereits gejagt werden, oder schon tot daliegen, können nicht gejagt werden
    return (species != Species::Duck && state != STATE_DEAD && state != STATE_DISAPPEARING && !hunter);
}

void noAnimal::BeginHunting(nofHunter* hunter)
{
    this->hunter = hunter;
}

MapPoint noAnimal::HunterIsNear()
{
    // Steht es gerade?
    if(state == STATE_PAUSED)
    {
        // dann bleibt es einfach stehen und gibt seine jetzigen Koordinaten zurück
        state = STATE_WAITINGFORHUNTER;
        // Warteevent abmelden
        GetEvMgr().RemoveEvent(current_ev);
        return pos;
    } else
    {
        // ansonsten nach dem Laufen stehenbleiben und die Koordinaten zurückgeben von dem Punkt, der erreicht wird
        state = STATE_WALKINGUNTILWAITINGFORHUNTER;
        return gwg->GetNeighbour(pos, GetCurMoveDir());
    }
}

void noAnimal::StopHunting()
{
    // keiner jagt uns mehr
    hunter = nullptr;

    switch(state)
    {
        default: return;

        case STATE_WAITINGFORHUNTER:
        {
            // wenn wir stehen, zusätzlich loslaufen
            state = STATE_WALKING;
            StandardWalking();
        }
        break;
        case STATE_WALKINGUNTILWAITINGFORHUNTER:
        {
            // wir können wieder normal weiterlaufen
            state = STATE_WALKING;
        }
        break;
    }
}

void noAnimal::Die()
{
    // nun bin ich tot
    if(ANIMALCONSTS[species].dead_id)
    {
        // Verwesungsevent
        current_ev = GetEvMgr().AddEvent(this, 300, 2);
        state = STATE_DEAD;
    } else
    {
        // Falls keine Verwesungsgrafik --> sofort verschwinden
        current_ev = GetEvMgr().AddEvent(this, 30, 3);
        state = STATE_DISAPPEARING;
    }
}

void noAnimal::Eviscerated()
{
    // Event abmelden
    GetEvMgr().RemoveEvent(current_ev);
    // Reset hunter
    hunter = nullptr;
}
