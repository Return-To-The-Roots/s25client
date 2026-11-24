// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofSkinner.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "LeatherLoader.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "buildings/nobUsual.h"
#include "figures/nofHunter.h"
#include "network/GameClient.h"
#include "notifications/BuildingNote.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "nodeObjs/noAnimal.h"
#include "gameData/GameConsts.h"
#include "gameData/JobConsts.h"

using namespace leatheraddon;

const MapCoord MAX_SKINNING_DISTANCE = 50;

nofSkinner::nofSkinner(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(Job::Skinner, pos, player, workplace)
{}

nofSkinner::nofSkinner(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id)
{
    if(state != State::FigureWork && state != State::Waiting1)
        animal = sgd.popObject<noAnimal>(GO_Type::Animal);
    else
        animal = nullptr;
}

void nofSkinner::serialize(SerializedGameData& sgd) const
{
    nofBuildingWorker::serialize(sgd);

    if(state != State::FigureWork && state != State::Waiting1)
        sgd.pushObject(animal, true);
}

void nofSkinner::workAborted()
{
    if(animal)
    {
        animal->stopSkinning();
        animal = nullptr;
    }
}

void nofSkinner::animalLost()
{
    animal = nullptr;

    switch(state)
    {
        default: return;
        case State::SkinnerWalkingToCarcass: startWalkingHome(); break;
        case State::SkinnerSkinningCarcass:
            // Remove Working-Event
            getEvMgr().removeEvent(current_ev);
            startWalkingHome();
            break;
    }
}

#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4646) // function declared with [[noreturn]] has non-void return type
#endif

unsigned short nofSkinner::getCarryID() const
{
    throw std::logic_error("Must not be called. Handled by custom DrawWalkingWithWare");
}

#ifdef _MSC_VER
#    pragma warning(pop)
#endif

void nofSkinner::drawWalkingWithWare(DrawPoint drawPt)
{
    drawWalking(drawPt, "leather_bobs", bobIndex[BobType::SkinnerCarryingSkins]);
}

void nofSkinner::handleDerivedEvent(unsigned)
{
    switch(state)
    {
        case State::Waiting1: tryStartSkinning(); break;
        case State::Work: handleStateWork(); break;
        case State::Waiting2: handleStateWaiting2(); break;
        case State::SkinnerSkinningCarcass: handleStateSkinningCarcass(); break;
        default: break;
    }
}

void nofSkinner::drawWorking(DrawPoint drawPt)
{
    const GamePlayer& owner = world->getPlayer(player);
    switch(state)
    {
        default: break;
        case State::SkinnerSkinningCarcass:
        {
            unsigned short id = GAMECLIENT.interpolate(45, current_ev);
            unsigned short drawId =
              leatheraddon::bobIndex[leatheraddon::BobType::SkinnerSkinningAnimalCarcassAnimation];

            if(id < 4)
                drawId = drawId + id;
            else if(id < 36)
                drawId = (drawId + 4) + (id - 4) % 8;
            else
                drawId = (drawId + 12) + id - 36;

            LOADER.getPlayerImage("leather_bobs", drawId)->drawForPlayer(drawPt, owner.color);
        }
        break;
    }
}

void nofSkinner::handleStateSkinningCarcass()
{
    // see comment at noAnimal::handleEvent for explanation of ownership of animal
    animal->skinned();
    if(!animal->isHunted())
    {
        // Tier verschwinden lassen
        auto ownedAnimal = world->removeFigure(pos, *animal);
        // Tier vernichten
        ownedAnimal->destroy();
    }
    animal = nullptr;

    ware = produceWare();
    startWalkingHome();
}

void nofSkinner::startWalkingHome()
{
    workAborted();
    state = State::WalkingHome;
    // We may be still walking in which case we delay finding a path home until we reached the next node
    if(!isMoving())
        handleStateWalkingHome();
}

void nofSkinner::handleStateWalkingHome()
{
    // We reached our home? (exactly at the flag !!)
    MapPoint homeFlagPos = world->getNeighbour(workplace->getPos(), Direction::SouthEast);
    if(pos == homeFlagPos)
    {
        // Redirect to nofBuildingWorker
        workingReady();
        return;
    }

    // Search for way if we can go home (add tolerance to avoid dying, if he walked some fields to far away)
    const auto dir = world->findHumanPath(pos, homeFlagPos, MAX_SKINNING_DISTANCE + MAX_SKINNING_DISTANCE / 4);
    if(dir)
        startWalking(*dir);
    else
    {
        // no way to home
        abrogateWorkplace();
        startWandering();
    }
}

void nofSkinner::tryStartSkinning()
{
    if(workplace->isProductionDisabled())
    {
        current_ev = getEvMgr().addEvent(this, JOB_CONSTS[job_].wait1_length, 1);
        workplace->startNotWorking();
        return;
    }

    // pigs have higher priority then going skinning dead animals
    if(areWaresAvailable())
        handleStateWaiting1();
    else
    {
        const auto availableAnimals = nofHunter::getAnimalsInRange(*world, pos, ANIMAL_RADIUS, MAX_SKINNING_DISTANCE,
                                                                   [](const noAnimal* a) { return a->canBeSkinned(); });

        if(!availableAnimals.empty())
        {
            animal = RANDOM_ELEMENT(availableAnimals);
            state = State::SkinnerWalkingToCarcass;
            workplace->is_working = true;
            animal->beginSkinning(this);
            startWalking(Direction::SouthEast);
            workplace->stopNotWorking();
        } else
        {
            current_ev = getEvMgr().addEvent(this, JOB_CONSTS[job_].wait1_length, 1);
            workplace->startNotWorking();
        }
    }
}

void nofSkinner::tryToWork()
{
    if(areWaresAvailable())
        nofBuildingWorker::tryToWork();
    else
    {
        state = State::Waiting1;
        current_ev = getEvMgr().addEvent(this, JOB_CONSTS[job_].wait1_length, 1);
        workplace->startNotWorking();
    }
}

void nofSkinner::walkedDerived()
{
    switch(state)
    {
        default: break;
        case State::SkinnerWalkingToCarcass:
        {
            if(pos == animal->getPos())
            {
                state = State::SkinnerSkinningCarcass;
                current_ev = getEvMgr().addEvent(this, 80, 1);
            } else
            {
                const auto dir = world->findHumanPath(pos, animal->getPos(), MAX_SKINNING_DISTANCE);
                if(dir)
                    startWalking(*dir);
                else
                {
                    // no way --> go home
                    startWalkingHome();
                }
            }
            break;
        }
        case State::WalkingHome: handleStateWalkingHome(); break;
    }
}

helpers::OptionalEnum<GoodType> nofSkinner::produceWare()
{
    return GoodType::Skins;
}
