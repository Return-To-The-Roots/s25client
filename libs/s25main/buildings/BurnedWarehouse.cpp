// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "BurnedWarehouse.h"

#include "EventManager.h"
#include "GamePlayer.h"
#include "SerializedGameData.h"
#include "figures/nofPassiveWorker.h"
#include "helpers/Range.h"
#include "pathfinding/PathConditionHuman.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include <boost/container/static_vector.hpp>

/// Number of "waves" of workers leaving
constexpr unsigned GO_OUT_PHASES = 10;
/// Time between those phases
constexpr unsigned PHASE_LENGTH = 2;

BurnedWarehouse::BurnedWarehouse(const MapPoint pos, const unsigned char player, const PeopleArray& people)
    : noCoordBase(NodalObjectType::BurnedWarehouse, pos), player(player), go_out_phase(0), people(people)
{
    // First event
    GetEvMgr().AddEvent(this, PHASE_LENGTH, 0);
}

BurnedWarehouse::BurnedWarehouse(SerializedGameData& sgd, const unsigned obj_id)
    : noCoordBase(sgd, obj_id), player(sgd.PopUnsignedChar()), go_out_phase(sgd.PopUnsignedInt())
{
    helpers::popContainer(sgd, people.people);
    if(sgd.GetGameDataVersion() >= 12)
        helpers::popContainer(sgd, people.armoredSoldiers);
    else
        std::fill(people.armoredSoldiers.begin(), people.armoredSoldiers.end(), 0);
}

BurnedWarehouse::~BurnedWarehouse() = default;

void BurnedWarehouse::Destroy()
{
    noCoordBase::Destroy();
}

void BurnedWarehouse::Serialize(SerializedGameData& sgd) const
{
    noCoordBase::Serialize(sgd);

    sgd.PushUnsignedChar(player);
    sgd.PushUnsignedInt(go_out_phase);
    helpers::pushContainer(sgd, people.people);
    helpers::pushContainer(sgd, people.armoredSoldiers);
}

void BurnedWarehouse::HandleEvent(const unsigned /*id*/)
{
    RTTR_Assert(go_out_phase != GO_OUT_PHASES);

    // Determine valid directions for people
    boost::container::static_vector<Direction, helpers::NumEnumValues_v<Direction>> possibleDirs;
    PathConditionHuman pathChecker(*world);
    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        if(pathChecker.IsNodeOk(world->GetNeighbour(pos, dir)))
            possibleDirs.push_back(dir);
    }

    if(possibleDirs.empty())
    {
        // No way out for figures -> all die and we can remove this object
        GetEvMgr().AddToKillList(world->RemoveFigure(pos, *this));
        for(const auto i : helpers::enumRange<Job>())
            world->GetPlayer(player).DecreaseInventoryJob(i, people.people[i]);
        for(const auto i : helpers::enumRange<ArmoredSoldier>())
            world->GetPlayer(player).DecreaseInventoryJob(i, people.armoredSoldiers[i]);

        return;
    }

    for(const auto job : helpers::enumRange<Job>())
    {
        // In the last phase all remaining ones leave, else only some
        unsigned count;
        if(go_out_phase + 1 >= GO_OUT_PHASES)
            count = people.people[job];
        else
            count = people.people[job] / (GO_OUT_PHASES - go_out_phase);
        if(count == 0)
            continue;

        // Remove from inventory
        people.people[job] -= count;

        // Distribute in all directions starting at a random one of the possible ones
        const unsigned startIdx = (possibleDirs.size() <= 1u) ? 0 : RANDOM_RAND(possibleDirs.size());
        const unsigned numPeoplePerDir = count / possibleDirs.size();

        for(const unsigned j : helpers::range(possibleDirs.size()))
        {
            // Get current direction accounting for startIdx and hence possible wrap around
            const unsigned idx = j + startIdx;
            const Direction curDir = possibleDirs[idx < possibleDirs.size() ? idx : idx - possibleDirs.size()];
            // Take all in last direction
            const auto curNumPeople = (j + 1u < possibleDirs.size()) ? numPeoplePerDir : count;
            count -= curNumPeople;
            for([[maybe_unused]] const auto z : helpers::range(curNumPeople))
            {
                // Create job and send moving into the current direction
                auto& figure = world->AddFigure(pos, std::make_unique<nofPassiveWorker>(job, pos, player, nullptr));
                if(isSoldier(job))
                {
                    auto const armoredSoldier = figureToAmoredSoldierEnum(&figure);
                    if(people.armoredSoldiers[armoredSoldier] > 0)
                    {
                        figure.SetArmor(true);
                        people.Remove(armoredSoldier);
                    }
                }
                figure.StartWandering(GetObjId());
                figure.StartWalking(curDir);
            }
        }
    }

    // Prepare next phase if any
    ++go_out_phase;
    if(go_out_phase == GO_OUT_PHASES)
    {
        // All done
        GetEvMgr().AddToKillList(world->RemoveFigure(pos, *this));
        // There shouldn't be any more
        for(unsigned int it : people.people)
            RTTR_Assert(it == 0);
    } else // Not done yet
        GetEvMgr().AddEvent(this, PHASE_LENGTH, 0);
}
