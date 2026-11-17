// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofHunter.h"

#include "EventManager.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "RTTR_Assert.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "buildings/nobUsual.h"
#include "enum_cast.hpp"
#include "network/GameClient.h"
#include "notifications/BuildingNote.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "pathfinding/PathConditionHuman.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "nodeObjs/noAnimal.h"
#include "gameData/JobConsts.h"
#include <stdexcept>

/// Maximale Distanz, die ein Jäger läuft, um ein Tier zu jagen
const MapCoord MAX_HUNTING_DISTANCE = 50;

nofHunter::nofHunter(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofBuildingWorker(Job::Hunter, pos, player, workplace), animal(nullptr), shootingPos(MapPoint::invalid()),
      shooting_dir(Direction::West)
{}

void nofHunter::serialize(SerializedGameData& sgd) const
{
    nofBuildingWorker::serialize(sgd);

    if(state != State::FigureWork && state != State::Waiting1)
    {
        sgd.pushObject(animal, true);
        helpers::pushPoint(sgd, shootingPos);
        sgd.pushEnum<uint8_t>(shooting_dir);
    }
}

nofHunter::nofHunter(SerializedGameData& sgd, const unsigned obj_id) : nofBuildingWorker(sgd, obj_id)
{
    if(state != State::FigureWork && state != State::Waiting1)
    {
        animal = sgd.popObject<noAnimal>(GO_Type::Animal);
        shootingPos = sgd.popMapPoint();
        shooting_dir = sgd.pop<Direction>();
        // https://github.com/Return-To-The-Roots/s25client/issues/1126
        if(sgd.getGameDataVersion() < 4 && state == State::HunterFindingShootingpoint && pos == shootingPos)
            state = State::HunterWaitingForAnimalReady;
    } else
    {
        animal = nullptr;
        shootingPos = MapPoint::invalid();
        shooting_dir = Direction::West;
    }
}

void nofHunter::drawWorking(DrawPoint drawPt)
{
    const GamePlayer& owner = world->getPlayer(player);
    switch(state)
    {
        default: break;
        case State::HunterWaitingForAnimalReady:
            LOADER.getBobSprite(owner.nation, Job::Hunter, shooting_dir, 0).drawForPlayer(drawPt, owner.color);
            break;
        case State::HunterShooting:
        {
            if(shooting_dir == Direction::East)
            {
                // die Animation in dieser Richtung ist etwas anders als die in den restlichen
                unsigned short id = GAMECLIENT.interpolate(13, current_ev);
                LOADER.getPlayerImage("rom_bobs", 219 + id)->drawForPlayer(drawPt, owner.color);

                if(id == 12)
                {
                    world->getSoundMgr().playNOSound(74, *this, 0);
                    was_sounding = true;
                }
            } else
            {
                unsigned short id = GAMECLIENT.interpolate(8, current_ev);
                LOADER.getPlayerImage("rom_bobs", 1686 + rttr::enumCast(shooting_dir + 2u) * 8 + id)
                  ->drawForPlayer(drawPt, owner.color);

                if(id == 7)
                {
                    world->getSoundMgr().playNOSound(74, *this, 0);
                    was_sounding = true;
                }
            }

            drawShadow(drawPt, 0, shooting_dir);
        }
        break;
        case State::HunterEviscerating:
        {
            unsigned short id = GAMECLIENT.interpolate(45, current_ev);
            unsigned short draw_id;

            if(id < 4)
                draw_id = 232 + id;
            else if(id < 36)
                draw_id = 236 + (id - 4) % 8;
            else
                draw_id = 244 + id - 36;

            LOADER.getPlayerImage("rom_bobs", draw_id)->drawForPlayer(drawPt, owner.color);
        }
        break;
    }
}

void nofHunter::handleDerivedEvent(unsigned /*id*/)
{
    switch(state)
    {
        default: RTTR_Assert(false); break;
        case State::Waiting1: tryStartHunting(); break;
        case State::HunterWaitingForAnimalReady: handleStateWaitingForAnimalReady(); break;
        case State::HunterShooting: handleStateShooting(); break;
        case State::HunterEviscerating: handleStateEviscerating(); break;
    }
}

std::vector<noAnimal*> nofHunter::getAnimalsInRange(const GameWorldBase& world, const MapPoint pos, unsigned radius,
                                                    unsigned maxDistance, bool (*isValidAnimal)(const noAnimal*))
{
    const auto pointToAnimal = [&world](const MapPoint pt, unsigned) -> noAnimal* {
        for(auto& figure : world.getFigures(pt))
        {
            if(figure.getType() == NodalObjectType::Animal)
                return static_cast<noAnimal*>(&figure);
        }
        return nullptr;
    };

    const auto canAnimalBeUsed = [pos, maxDistance, &world, isValidAnimal](const noAnimal* const animal) {
        return animal && isValidAnimal(animal)
               && (pos == animal->getPos() || world.findHumanPath(pos, animal->getPos(), maxDistance));
    };

    return world.getPointsInRadius(pos, radius, pointToAnimal, canAnimalBeUsed, true);
}

std::vector<noAnimal*> nofHunter::getHuntableAnimalsInRange(const GameWorldBase& world, MapPoint pos)
{
    return getAnimalsInRange(world, pos, ANIMAL_RADIUS, MAX_HUNTING_DISTANCE,
                             [](const noAnimal* a) { return a->canHunted(); });
}

void nofHunter::tryStartHunting()
{
    std::vector<noAnimal*> available_animals;

    if(world->getReplayCompatVersion() >= 4)
        available_animals = getHuntableAnimalsInRange(*world, pos);
    else
    {
        // Legacy square search for replays recorded with old code
        constexpr int SQUARE_SIZE = 19;
        Position curPos;
        for(curPos.y = pos.y - SQUARE_SIZE; curPos.y <= pos.y + SQUARE_SIZE; ++curPos.y)
        {
            for(curPos.x = pos.x - SQUARE_SIZE; curPos.x <= pos.x + SQUARE_SIZE; ++curPos.x)
            {
                MapPoint curMapPos = world->makeMapPoint(curPos);
                for(auto& figure : world->getFigures(curMapPos))
                {
                    if(figure.getType() != NodalObjectType::Animal)
                        continue;
                    auto& animal = static_cast<noAnimal&>(figure);
                    if(!animal.canHunted())
                        continue;
                    if(pos == animal.getPos() || world->findHumanPath(pos, animal.getPos(), MAX_HUNTING_DISTANCE))
                    {
                        available_animals.push_back(&animal);
                    }
                }
            }
        }
    }

    // Gibt es überhaupt ein Tier, das ich jagen kann?
    if(!available_animals.empty())
    {
        // Ein Tier zufällig heraussuchen
        animal = RANDOM_ELEMENT(available_animals);

        // Wir jagen es jetzt
        state = State::HunterChasing;

        // Wir arbeiten jetzt
        workplace->is_working = true;

        // Tier Bescheid sagen
        animal->beginHunting(this);

        // Anfangen zu laufen (erstmal aus dem Haus raus!)
        startWalking(Direction::SouthEast);

        workplace->stopNotWorking();
    } else
    {
        // Weiter warten, vielleicht gibts ja später wieder mal was
        current_ev = getEvMgr().addEvent(this, JOB_CONSTS[job_].wait1_length, 1);
        world->getNotifications().publish(
          BuildingNote(BuildingNote::NoRessources, player, workplace->getPos(), workplace->getBuildingType()));
        workplace->startNotWorking();
    }

    was_sounding = false;
}

void nofHunter::walkedDerived()
{
    switch(state)
    {
        default: break;
        case State::HunterChasing: handleStateChasing(); break;
        case State::HunterFindingShootingpoint: handleStateFindingShootingPoint(); break;
        case State::HunterWalkingToCadaver: handleStateWalkingToCadaver(); break;
        case State::WalkingHome: handleStateWalkingHome(); break;
    }
}

bool nofHunter::isShootingPointGood(const MapPoint pt)
{
    // Punkt muss betretbar sein und man muss ihn erreichen können
    return PathConditionHuman(*world).isNodeOk(pt) && world->findHumanPath(this->pos, pt, 6).hasValue();
}

void nofHunter::handleStateChasing()
{
    // Sind wir in der Nähe des Tieres?
    if(world->calcDistance(pos, animal->getPos()) < 7)
    {
        // Dann bitten wir es mal, schonmal anzuhalten und bekommen seine Koordinaten, wo es dann steht
        Position animalPos(animal->hunterIsNear());

        // Nun müssen wir drumherum einen Punkt suchen, von dem wir schießen, der natürlich direkt dem Standort
        // des Tieres gegenüberliegen muss (mit zufälliger Richtung beginnen)
        shootingPos = MapPoint::invalid();
        for(const Direction d : helpers::enumRange(RANDOM_ENUM(Direction)))
        {
            Position delta;
            switch(d)
            {
                case Direction::West:
                    delta.x = -4;
                    delta.y = 0;
                    break;
                case Direction::NorthWest:
                    delta.x = -2;
                    delta.y = -4;
                    break;
                case Direction::NorthEast:
                    delta.x = 2;
                    delta.y = -4;
                    break;
                case Direction::East:
                    delta.x = 4;
                    delta.y = 0;
                    break;
                case Direction::SouthEast:
                    delta.x = 2;
                    delta.y = 4;
                    break;
                case Direction::SouthWest:
                    delta.x = -2;
                    delta.y = 4;
                    break;
                default: throw std::logic_error("Wrong value?");
            }

            MapPoint curShootingPos = world->makeMapPoint(animalPos + delta);
            if(curShootingPos == pos || world->findHumanPath(pos, curShootingPos, 6))
            {
                shootingPos = curShootingPos;
                // Richtung, in die geschossen wird, bestimmen (natürlich die entgegengesetzte nehmen)
                shooting_dir = d + 3u;
                break;
            }
        }

        // Wurde ein Punkt gefunden?
        if(shootingPos.isValid())
        {
            // dorthingehen
            state = State::HunterFindingShootingpoint;
            handleStateFindingShootingPoint();
        } else
        {
            // kein Punkt gefunden --> nach Hause gehen
            startWalkingHome();
        }

    } else
    {
        // Weg dorthin suchen
        const auto dir = world->findHumanPath(pos, animal->getPos(), MAX_HUNTING_DISTANCE);
        if(dir)
        {
            // Weg gefunden, dann hinlaufen
            startWalking(*dir);
        } else
        {
            // kein Weg gefunden --> nach Hause laufen
            startWalkingHome();
        }
    }
}

void nofHunter::handleStateFindingShootingPoint()
{
    // Are we there yet?
    if(shootingPos == pos)
    {
        state = State::HunterWaitingForAnimalReady;
        handleStateWaitingForAnimalReady();
    } else
    {
        // Weg dorthin suchen
        const auto dir = world->findHumanPath(pos, shootingPos, 6);
        if(dir)
        {
            // Weg gefunden, dann hinlaufen
            startWalking(*dir);
        } else
        {
            // kein Weg gefunden --> nach Hause laufen
            startWalkingHome();
        }
    }
}

void nofHunter::handleStateWaitingForAnimalReady()
{
    // Is the animal ready?
    if(animal->isReadyForShooting())
    {
        // fire!
        state = State::HunterShooting;
        current_ev = getEvMgr().addEvent(this, 16, 1);
    } else if(animal->isGettingReadyForShooting())
        current_ev = getEvMgr().addEvent(this, 15, 2); // Give the animal some time for getting ready
    else
    {
        // Something went wrong, animal is doing something else?
        startWalkingHome();
    }
}

void nofHunter::handleStateShooting()
{
    // Tier muss sterben
    animal->die();
    // zum Kadaver laufen, um ihn auszunehmen
    state = State::HunterWalkingToCadaver;
    handleStateWalkingToCadaver();
}

void nofHunter::handleStateWalkingToCadaver()
{
    // Sind wir schon da?
    if(animal->getPos() == pos)
    {
        // dann ausnehmen
        state = State::HunterEviscerating;
        current_ev = getEvMgr().addEvent(this, 80, 1);
    } else
    {
        // Weg dorthin suchen
        const auto dir = world->findHumanPath(pos, animal->getPos(), 6);
        if(dir)
        {
            // Weg gefunden, dann hinlaufen
            startWalking(*dir);
        } else
        {
            // kein Weg gefunden --> nach Hause laufen
            startWalkingHome();
        }
    }
}

void nofHunter::handleStateEviscerating()
{
    // Evtl. Sounds löschen
    if(was_sounding)
    {
        world->getSoundMgr().stopSounds(*this);
        was_sounding = false;
    }

    // see comment at noAnimal::HandleEvent for explanation of ownership of animal
    animal->eviscerated();
    if(!animal->isGettingSkinned())
    {
        // Tier verschwinden lassen
        auto ownedAnimal = world->removeFigure(pos, *animal);
        // Tier vernichten
        ownedAnimal->destroy();
    }
    animal = nullptr;

    // Fleisch in die Hand nehmen
    ware = GoodType::Meat;
    // und zurück zur Hütte
    startWalkingHome();
}

void nofHunter::startWalkingHome()
{
    workAborted();
    state = State::WalkingHome;
    // We may be still walking in which case we delay finding a path home until we reached the next node
    if(!isMoving())
        handleStateWalkingHome();
}

void nofHunter::handleStateWalkingHome()
{
    // Sind wir zu Hause angekommen? (genauer an der Flagge !!)
    MapPoint homeFlagPos = world->getNeighbour(workplace->getPos(), Direction::SouthEast);
    if(pos == homeFlagPos)
    {
        // Weiteres übernimmt nofBuildingWorker
        workingReady();
        return;
    }

    // Weg suchen und ob wir überhaupt noch nach Hause kommen (Toleranz bei dem Weg mit einberechnen,
    // damit er nicht einfach rumirrt und wegstirbt, wenn er einmal ein paar Felder zu weit gelaufen ist)
    const auto dir = world->findHumanPath(pos, homeFlagPos, MAX_HUNTING_DISTANCE + MAX_HUNTING_DISTANCE / 4);
    if(dir)
    {
        // All good, let's start walking there
        startWalking(*dir);
    } else
    {
        // Kein Weg führt mehr nach Hause--> Rumirren
        abrogateWorkplace();
        startWandering();
        wander();
    }
}

void nofHunter::animalLost()
{
    animal = nullptr;

    switch(state)
    {
        default: return;
        case State::HunterChasing:
        case State::HunterFindingShootingpoint:
        case State::HunterWalkingToCadaver: startWalkingHome(); break;
        case State::HunterShooting:
        case State::HunterEviscerating:
        case State::HunterWaitingForAnimalReady:
            // Arbeits-Event abmelden
            getEvMgr().removeEvent(current_ev);
            // Nach Hause laufen
            startWalkingHome();
            break;
    }
}

void nofHunter::workAborted()
{
    // Tier Bescheid sagen
    if(animal)
    {
        animal->stopHunting();
        animal = nullptr;
    }
}
