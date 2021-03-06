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
#include "SoundManager.h"
#include "drivers/VideoDriverWrapper.h"
#include "figures/nofHunter.h"
#include "helpers/random.h"
#include "network/GameClient.h"
#include "ogl/SoundEffectItem.h"
#include "ogl/glSmartBitmap.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "gameData/GameConsts.h"
#include "gameData/TerrainDesc.h"
#include "s25util/colors.h"

namespace {
auto& getAnimalRng()
{
    static auto soundRng = helpers::getRandomGenerator();
    return soundRng;
}
} // namespace

noAnimal::noAnimal(const Species species, const MapPoint pos)
    : noMovable(NodalObjectType::Animal, pos), species(species), state(State::Walking), pause_way(5 + RANDOM_RAND(15)),
      hunter(nullptr), sound_moment(0)
{}

void noAnimal::Serialize(SerializedGameData& sgd) const
{
    noMovable::Serialize(sgd);

    sgd.PushEnum<uint8_t>(species);
    sgd.PushEnum<uint8_t>(state);
    sgd.PushUnsignedShort(pause_way);
    sgd.PushObject(hunter, true);
}

noAnimal::noAnimal(SerializedGameData& sgd, const unsigned obj_id)
    : noMovable(sgd, obj_id), species(sgd.Pop<Species>()), state(sgd.Pop<State>()), pause_way(sgd.PopUnsignedShort()),
      hunter(sgd.PopObject<nofHunter>(GO_Type::NofHunter)), sound_moment(0)
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
        case State::WalkingUntilWaitingForHunter:
        case State::Walking:
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
                const unsigned now = VIDEODRIVER.GetTickCount();
                // Wurde der Soundzeitpunkt schon überschritten?
                if(now > sound_moment)
                {
                    // Play the sound if we unless we missed the timeframe by at least 1s
                    if((now < sound_moment + 1000) && !GAMECLIENT.IsPaused())
                        world->GetSoundMgr().playAnimalSound((species == Species::Sheep) ? 94 : 95);

                    // Calc new sound time
                    sound_moment = now + helpers::randomValue(getAnimalRng(), 8000, 13000);
                }
            }
        }
        break;
        case State::WaitingForHunter:
        case State::Paused:
        {
            // Stehend zeichnen
            LOADER.getAnimalSprite(species, GetCurMoveDir(), 0).draw(drawPt);
        }
        break;
        case State::Dead:
        {
            if(!LOADER.getDeadAnimalSprite(species).empty())
            {
                LOADER.getDeadAnimalSprite(species).draw(drawPt);
            }
        }
        break;
        case State::Disappearing:
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
            if(state != State::Dead)
                state = State::Walking;
        }
        break;
        // Sterbe-Event
        case 2:
        {
            // nun verschwinden
            current_ev = GetEvMgr().AddEvent(this, 30, 3);
            state = State::Disappearing;

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
            GetEvMgr().AddToKillList(world->RemoveFigure(pos, *this));
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
        case State::WalkingUntilWaitingForHunter:
        {
            // stehenbleiben und auf den Jäger warten
            state = State::WaitingForHunter;
        }
        break;
        case State::Walking:
        {
            // ein weiteres Stück gelaufen
            --pause_way;

            // Ist es so lange gelaufen, dass es mal wieder eine Pause braucht?
            if(!pause_way)
            {
                // dann stellt es sich hier hin und wartet erstmal eine Weile
                state = State::Paused;
                pause_way = 5 + RANDOM_RAND(15);
                current_ev = GetEvMgr().AddEvent(this, 50 + RANDOM_RAND(50), 1);
            } else
            {
                StandardWalking();
            }

            // Bei Enten und Schafen: Soundzeitpunkt ggf. setzen
            if(species == Species::Duck || species == Species::Sheep)
            {
                const unsigned now = VIDEODRIVER.GetTickCount();
                // Wurde der Soundzeitpunkt schon überschritten?
                if(now > sound_moment)
                    sound_moment = now + helpers::randomValue(getAnimalRng(), 8000, 13000);
            }
        }
        break;
    }
}

helpers::OptionalEnum<Direction> noAnimal::FindDir()
{
    // mit zufälliger Richtung anfangen
    const Direction doffset = RANDOM_ENUM(Direction);
    const WorldDescription& worldDesc = world->GetDescription();

    for(const auto dir : helpers::enumRange(doffset))
    {
        const auto terrains = world->GetTerrain(pos, dir);
        const DescIdx<TerrainDesc> tLeft = terrains.left;
        const DescIdx<TerrainDesc> tRight = terrains.right;

        if(species == Species::Duck)
        {
            // Enten schwimmen nur auf dem Wasser --> muss daher Wasser sein
            if(worldDesc.get(tLeft).kind == TerrainKind::Water && worldDesc.get(tRight).kind == TerrainKind::Water)
                return dir;
        } else if(species == Species::PolarBear)
        {
            // Polarbären laufen nur auf Schnee rum
            if(worldDesc.get(tLeft).kind == TerrainKind::Snow && worldDesc.get(tRight).kind == TerrainKind::Snow)
                return dir;
        } else
        {
            // Die anderen Tiere dürfen nur auf Wiesen,Savannen usw. laufen, nicht auf Bergen oder in der Wüste!
            if(!worldDesc.get(tLeft).IsUsableByAnimals() || !worldDesc.get(tRight).IsUsableByAnimals())
                continue;

            // Außerdem dürfen keine Hindernisse im Weg sein
            const MapPoint dst = world->GetNeighbour(pos, dir);
            const noBase* no = world->GetNO(dst);

            if(no->GetType() != NodalObjectType::Nothing && no->GetType() != NodalObjectType::Environment
               && no->GetType() != NodalObjectType::Tree)
                continue;

            // Schließlich auch möglichst keine anderen Figuren bzw. Tiere
            if(!world->GetFigures(dst).empty())
                continue;

            // Und möglichst auch keine Straßen
            for(const auto dir2 : helpers::EnumRange<Direction>{})
            {
                if(world->GetPointRoad(dst, dir2) != PointRoad::None)
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
    return (species != Species::Duck && state != State::Dead && state != State::Disappearing && !hunter);
}

void noAnimal::BeginHunting(nofHunter* hunter)
{
    this->hunter = hunter;
}

MapPoint noAnimal::HunterIsNear()
{
    // Steht es gerade?
    if(state == State::Paused)
    {
        // dann bleibt es einfach stehen und gibt seine jetzigen Koordinaten zurück
        state = State::WaitingForHunter;
        // Warteevent abmelden
        GetEvMgr().RemoveEvent(current_ev);
        return pos;
    } else
    {
        // ansonsten nach dem Laufen stehenbleiben und die Koordinaten zurückgeben von dem Punkt, der erreicht wird
        state = State::WalkingUntilWaitingForHunter;
        return world->GetNeighbour(pos, GetCurMoveDir());
    }
}

void noAnimal::StopHunting()
{
    // keiner jagt uns mehr
    hunter = nullptr;

    switch(state)
    {
        default: return;

        case State::WaitingForHunter:
        {
            // wenn wir stehen, zusätzlich loslaufen
            state = State::Walking;
            StandardWalking();
        }
        break;
        case State::WalkingUntilWaitingForHunter:
        {
            // wir können wieder normal weiterlaufen
            state = State::Walking;
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
        state = State::Dead;
    } else
    {
        // Falls keine Verwesungsgrafik --> sofort verschwinden
        current_ev = GetEvMgr().AddEvent(this, 30, 3);
        state = State::Disappearing;
    }
}

void noAnimal::Eviscerated()
{
    // Event abmelden
    GetEvMgr().RemoveEvent(current_ev);
    // Reset hunter
    hunter = nullptr;
}
