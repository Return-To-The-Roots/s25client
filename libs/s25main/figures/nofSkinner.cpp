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
#include "network/GameClient.h"
#include "notifications/BuildingNote.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "nodeObjs/noAnimal.h"
#include "gameData/JobConsts.h"

using namespace leatheraddon;

const MapCoord MAX_SKINNING_DISTANCE = 50;

nofSkinner::nofSkinner(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(Job::Skinner, pos, player, workplace)
{}

nofSkinner::nofSkinner(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id)
{
    if(state != State::FigureWork && state != State::Waiting1)
        animal = sgd.PopObject<noAnimal>(GO_Type::Animal);
    else
        animal = nullptr;
}

void nofSkinner::Serialize(SerializedGameData& sgd) const
{
    nofBuildingWorker::Serialize(sgd);

    if(state != State::FigureWork && state != State::Waiting1)
    {
        sgd.PushObject(animal, true);
    }
}

void nofSkinner::WorkAborted()
{
    if(animal)
    {
        animal->StopSkinning();
        animal = nullptr;
    }
}

void nofSkinner::AnimalLost()
{
    animal = nullptr;

    switch(state)
    {
        default: return;
        case State::SkinnerWalkingToCarcass: StartWalkingHome(); break;
        case State::SkinnerSkinningCarcass:
            // Remove Working-Event
            GetEvMgr().RemoveEvent(current_ev);
            StartWalkingHome();
            break;
    }
}

unsigned short nofSkinner::GetCarryID() const
{
    throw std::logic_error("Must not be called. Handled by custom DrawWalkingWithWare");
}

void nofSkinner::DrawWalkingWithWare(DrawPoint drawPt)
{
    DrawWalking(drawPt, "leather_bobs", bobIndex[BobType::SkinnerCarryingSkins]);
}

void nofSkinner::HandleDerivedEvent(unsigned)
{
    switch(state)
    {
        case State::Waiting1: TryStartSkinning(); break;
        case State::Work: HandleStateWork(); break;
        case State::Waiting2: HandleStateWaiting2(); break;
        case State::SkinnerSkinningCarcass: HandleStateSkinningCarcass(); break;
        default: break;
    }
}

void nofSkinner::DrawWorking(DrawPoint drawPt)
{
    const GamePlayer& owner = world->GetPlayer(player);
    switch(state)
    {
        default: break;
        case State::SkinnerSkinningCarcass:
        {
            unsigned short id = GAMECLIENT.Interpolate(45, current_ev);
            unsigned short draw_id =
              leatheraddon::bobIndex[leatheraddon::BobType::SkinnerSkinningAnimalCarcassAnimation];

            if(id < 4)
                draw_id = draw_id + id;
            else if(id < 36)
                draw_id = (draw_id + 4) + (id - 4) % 8;
            else
                draw_id = (draw_id + 12) + id - 36;

            LOADER.GetPlayerImage("leather_bobs", draw_id)->drawForPlayer(drawPt, owner.color);
        }
        break;
    }
}

void nofSkinner::HandleStateSkinningCarcass()
{
    animal->Skinned();
    if(!animal->IsHunted())
    {
        // Tier verschwinden lassen
        auto ownedAnimal = world->RemoveFigure(pos, *animal);
        // Tier vernichten
        ownedAnimal->Destroy();
    }
    animal = nullptr;

    ware = ProduceWare();
    StartWalkingHome();
}

void nofSkinner::StartWalkingHome()
{
    WorkAborted();
    state = State::WalkingHome;
    // We may be still walking in which case we delay finding a path home until we reached the next node
    if(!IsMoving())
        HandleStateWalkingHome();
}

void nofSkinner::HandleStateWalkingHome()
{
    // We reached our home? (exactly at the flag !!)
    MapPoint homeFlagPos = world->GetNeighbour(workplace->GetPos(), Direction::SouthEast);
    if(pos == homeFlagPos)
    {
        // Redirect to nofBuildingWorker
        WorkingReady();
        return;
    }

    // Search for way if we can go home (add tolerance to avoid dying, if he walked some fields to far away)
    const auto dir = world->FindHumanPath(pos, homeFlagPos, MAX_SKINNING_DISTANCE + MAX_SKINNING_DISTANCE / 4);
    if(dir)
    {
        StartWalking(*dir);
    } else
    {
        // no way to home
        AbrogateWorkplace();
        StartWandering();
        Wander();
    }
}

void nofSkinner::TryStartSkinning()
{
    if(workplace->IsProductionDisabled())
    {
        current_ev = GetEvMgr().AddEvent(this, JOB_CONSTS[job_].wait1_length, 1);
        workplace->StartNotWorking();
        return;
    }

    // pigs have higher priority then going skinning dead animals
    if(AreWaresAvailable())
        HandleStateWaiting1();
    else
    {
        const int ANIMAL_RADIUS = 19;
        const auto pointToAnimal = [world = this->world](const MapPoint pt, unsigned) -> noAnimal* {
            for(auto& figure : world->GetFigures(pt))
            {
                if(figure.GetType() != NodalObjectType::Animal)
                    continue;
                return checkedCast<noAnimal*>(&figure);
            }
            return nullptr;
        };

        const auto canAnimalBeSkinned = [pos = this->pos](const noAnimal* const animal) {
            return animal && animal->CanBeSkinned()
                   && (pos == animal->GetPos() || world->FindHumanPath(pos, animal->GetPos(), MAX_SKINNING_DISTANCE));
        };

        const auto available_animals =
          world->GetPointsInRadius(pos, ANIMAL_RADIUS, pointToAnimal, canAnimalBeSkinned, true);

        if(!available_animals.empty())
        {
            animal = RANDOM_ELEMENT(available_animals);
            state = State::SkinnerWalkingToCarcass;
            workplace->is_working = true;
            animal->BeginSkinning(this);
            StartWalking(Direction::SouthEast);
            workplace->StopNotWorking();
        } else
        {
            current_ev = GetEvMgr().AddEvent(this, JOB_CONSTS[job_].wait1_length, 1);
            workplace->StartNotWorking();
        }
    }
}

void nofSkinner::TryToWork()
{
    if(AreWaresAvailable())
        nofBuildingWorker::TryToWork();
    else
    {
        state = State::Waiting1;
        current_ev = GetEvMgr().AddEvent(this, JOB_CONSTS[job_].wait1_length, 1);
        workplace->StartNotWorking();
    }
}

void nofSkinner::WalkedDerived()
{
    switch(state)
    {
        default: break;
        case State::SkinnerWalkingToCarcass:
        {
            if(pos == animal->GetPos())
            {
                state = State::SkinnerSkinningCarcass;
                current_ev = GetEvMgr().AddEvent(this, 80, 1);
            } else
            {
                const auto dir = world->FindHumanPath(pos, animal->GetPos(), MAX_SKINNING_DISTANCE);
                if(dir)
                {
                    StartWalking(*dir);
                } else
                {
                    // no way --> go home
                    StartWalkingHome();
                }
            }
            break;
        }
        case State::WalkingHome: HandleStateWalkingHome(); break;
    }
}

helpers::OptionalEnum<GoodType> nofSkinner::ProduceWare()
{
    return GoodType::Skins;
}
