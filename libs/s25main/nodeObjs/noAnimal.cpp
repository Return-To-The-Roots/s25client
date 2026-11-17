// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "noAnimal.h"
#include "EventManager.h"
#include "LeatherLoader.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "drivers/VideoDriverWrapper.h"
#include "figures/nofHunter.h"
#include "figures/nofSkinner.h"
#include "helpers/random.h"
#include "network/GameClient.h"
#include "ogl/SoundEffectItem.h"
#include "ogl/glSmartBitmap.h"
#include "random/Random.h"
#include "world/GameWorld.h"
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
      hunter(nullptr), skinner(nullptr), sound_moment(0)
{}

void noAnimal::serialize(SerializedGameData& sgd) const
{
    noMovable::serialize(sgd);

    sgd.pushEnum<uint8_t>(species);
    sgd.pushEnum<uint8_t>(state);
    sgd.pushUnsignedShort(pause_way);
    sgd.pushObject(hunter, true);
    sgd.pushObject(skinner, true);
}

noAnimal::noAnimal(SerializedGameData& sgd, const unsigned obj_id)
    : noMovable(sgd, obj_id), species(sgd.pop<Species>()), state(sgd.pop<State>()), pause_way(sgd.popUnsignedShort()),
      hunter(sgd.popObject<nofHunter>(GO_Type::NofHunter)), skinner(nullptr), sound_moment(0)
{
    if(sgd.getGameDataVersion() >= 12)
        skinner = sgd.popObject<nofSkinner>(GO_Type::NofSkinner);
}

void noAnimal::startLiving()
{
    // anfangen zu laufen
    standardWalking();
}

void noAnimal::draw(DrawPoint drawPt)
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
            drawPt += calcWalkingRelative();

            unsigned aniStep = GAMECLIENT.interpolate(ASCENT_ANIMATION_STEPS[getAscent()], current_ev)
                               % ANIMALCONSTS[species].animation_steps;

            // Zeichnen
            LOADER.getAnimalSprite(species, getCurMoveDir(), aniStep).draw(drawPt);

            // Bei Enten und Schafen: Soll ein Sound gespielt werden?
            if(species == Species::Duck || species == Species::Sheep)
            {
                const unsigned now = VIDEODRIVER.getTickCount();
                // Wurde der Soundzeitpunkt schon überschritten?
                if(now > sound_moment)
                {
                    // Play the sound if we unless we missed the timeframe by at least 1s
                    if((now < sound_moment + 1000) && !GAMECLIENT.isPaused())
                        world->getSoundMgr().playAnimalSound((species == Species::Sheep) ? 94 : 95);

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
            LOADER.getAnimalSprite(species, getCurMoveDir(), 0).draw(drawPt);
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
            unsigned char alpha = 0xFF - GAMECLIENT.interpolate(0xFF, current_ev);

            // Gibts ein Leichenbild?
            if(!LOADER.getDeadAnimalSprite(species).empty())
            {
                LOADER.getDeadAnimalSprite(species).draw(drawPt, setAlpha(COLOR_WHITE, alpha));
            } else
            {
                // Stehend zeichnen
                LOADER.getAnimalSprite(species, getCurMoveDir(), 0).draw(drawPt, setAlpha(COLOR_WHITE, alpha));
            }
        }
        break;
    }
}

/*
    An animal can be hunted by the hunter and skinned by the skinner at the same time.
    We have the following cases:
    - Both do not reach the dead animal in time. It disappears and the noAnimal
      class is responsible for removing and deleting itself afterwards.
    - Both reach the dead animal in time. However finishes last is responsible for removing
      and deleting the dead animal (see nofSkinner::handleStateSkinningCarcass and
      nofHunter::handleStateEviscerating). No disappearing of the animal.
    - Either doesn't arrive soon enough. If the hunter was first the skin remains there
      and if the skinner was first the meat remains there. The animal disappears and the noAnimal
      class is responsible for removing and deleting itself afterwards.
*/
void noAnimal::handleEvent(const unsigned id)
{
    current_ev = nullptr;

    switch(id)
    {
        // Laufevent
        case 0:
        {
            // neue Position einnehmen
            walk();

            // entscheiden, was als nächstes zu tun ist
            walked();
        }
        break;
        // Warte-Event
        case 1:
        {
            // wieder weiterlaufen
            standardWalking();
            // state entsprechen setzen, wenn es nich gestorben ist
            if(state != State::Dead)
                state = State::Walking;
        }
        break;
        // Sterbe-Event
        case 2:
        {
            // nun verschwinden
            current_ev = getEvMgr().addEvent(this, 30, 3);
            state = State::Disappearing;

            // Jäger ggf. Bescheid sagen (falls der es nicht mehr rechtzeitig schafft, bis ich verwest bin)
            if(hunter)
            {
                hunter->animalLost();
                hunter = nullptr;
            }
            if(skinner)
            {
                skinner->animalLost();
                skinner = nullptr;
            }
        }
        break;
        // Verschwind-Event
        case 3:
        {
            // von der Karte tilgen
            getEvMgr().addToKillList(world->removeFigure(pos, *this));
        }
        break;
    }
}

void noAnimal::startWalking(const Direction dir)
{
    startMoving(dir, ANIMALCONSTS[species].speed);
}

void noAnimal::standardWalking()
{
    // neuen Weg suchen
    const helpers::OptionalEnum<Direction> dir = findDir();
    if(!dir)
    {
        // Sterben, weil kein Weg mehr gefunden wurde
        die();
        // Jäger ggf. Bescheid sagen (falls der es nicht mehr rechtzeitig schafft, bis ich verwest bin)
        if(hunter) //-V779
        {
            hunter->animalLost();
            hunter = nullptr;
        }
    } else
    {
        // weiterlaufen
        startWalking(*dir);
    }
}

void noAnimal::walked()
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
                current_ev = getEvMgr().addEvent(this, 50 + RANDOM_RAND(50), 1);
            } else
            {
                standardWalking();
            }

            // Bei Enten und Schafen: Soundzeitpunkt ggf. setzen
            if(species == Species::Duck || species == Species::Sheep)
            {
                const unsigned now = VIDEODRIVER.getTickCount();
                // Wurde der Soundzeitpunkt schon überschritten?
                if(now > sound_moment)
                    sound_moment = now + helpers::randomValue(getAnimalRng(), 8000, 13000);
            }
        }
        break;
    }
}

helpers::OptionalEnum<Direction> noAnimal::findDir()
{
    // mit zufälliger Richtung anfangen
    const Direction doffset = RANDOM_ENUM(Direction);
    const WorldDescription& worldDesc = world->getDescription();

    for(const auto dir : helpers::enumRange(doffset))
    {
        const auto terrains = world->getTerrain(pos, dir);
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
            if(!worldDesc.get(tLeft).isUsableByAnimals() || !worldDesc.get(tRight).isUsableByAnimals())
                continue;

            // Außerdem dürfen keine Hindernisse im Weg sein
            const MapPoint dst = world->getNeighbour(pos, dir);
            const noBase* no = world->getNO(dst);

            if(no->getType() != NodalObjectType::Nothing && no->getType() != NodalObjectType::Environment
               && no->getType() != NodalObjectType::Tree)
                continue;

            // Schließlich auch möglichst keine anderen Figuren bzw. Tiere
            if(!world->getFigures(dst).empty())
                continue;

            // Und möglichst auch keine Straßen
            for(const auto dir2 : helpers::EnumRange<Direction>{})
            {
                if(world->getPointRoad(dst, dir2) != PointRoad::None)
                    return std::nullopt;
            }
            return dir;
        }
    }

    // kein Weg mehr gefunden
    return std::nullopt;
}

bool noAnimal::canBeSkinned() const
{
    return (species != Species::Duck && state == State::Dead && !skinner);
}

bool noAnimal::isGettingSkinned() const
{
    return skinner != nullptr;
}

void noAnimal::skinned()
{
    if(!hunter)
    {
        // Remove decay event for animal because skinner has taken it
        getEvMgr().removeEvent(current_ev);
    }
    // Reset skinner
    skinner = nullptr;
}

void noAnimal::beginSkinning(nofSkinner* skinner)
{
    this->skinner = skinner;
}

void noAnimal::stopSkinning()
{
    skinner = nullptr;
}

bool noAnimal::canHunted() const
{
    // Enten sowie Tiere, die bereits gejagt werden, oder schon tot daliegen, können nicht gejagt werden
    return (species != Species::Duck && state != State::Dead && state != State::Disappearing && !hunter);
}

bool noAnimal::isHunted() const
{
    return hunter != nullptr;
}

void noAnimal::beginHunting(nofHunter* hunter)
{
    this->hunter = hunter;
}

MapPoint noAnimal::hunterIsNear()
{
    // Steht es gerade?
    if(state == State::Paused)
    {
        // dann bleibt es einfach stehen und gibt seine jetzigen Koordinaten zurück
        state = State::WaitingForHunter;
        // Warteevent abmelden
        getEvMgr().removeEvent(current_ev);
        return pos;
    } else
    {
        // ansonsten nach dem Laufen stehenbleiben und die Koordinaten zurückgeben von dem Punkt, der erreicht wird
        state = State::WalkingUntilWaitingForHunter;
        return world->getNeighbour(pos, getCurMoveDir());
    }
}

void noAnimal::stopHunting()
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
            standardWalking();
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

void noAnimal::die()
{
    // nun bin ich tot
    if(ANIMALCONSTS[species].dead_id)
    {
        // Verwesungsevent
        unsigned gf_length = leatheraddon::isAddonActive(*world) ? 600 : 300;
        current_ev = getEvMgr().addEvent(this, gf_length, 2);
        state = State::Dead;
    } else
    {
        // Falls keine Verwesungsgrafik --> sofort verschwinden
        current_ev = getEvMgr().addEvent(this, 30, 3);
        state = State::Disappearing;
    }
}

void noAnimal::eviscerated()
{
    // Event abmelden
    if(!isGettingSkinned())
        getEvMgr().removeEvent(current_ev);
    // Reset hunter
    hunter = nullptr;
}
