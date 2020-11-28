// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "EconomyModeHandler.h"

#include "EventManager.h"
#include "GameInterface.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "SerializedGameData.h"
#include "helpers/containerUtils.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "world/GameWorldGame.h"
#include "gameTypes/JobTypes.h"
#include "gameData/JobConsts.h"
#include <boost/optional.hpp>
#include <array>

EconomyModeHandler::EconomyModeHandler(unsigned endFrame) : endFrame(endFrame), gfLastUpdated(0)
{
    const std::vector<GoodType> specialGoodPool({
      GD_TONGS,
      GD_HAMMER,
      GD_AXE,
      GD_SAW,
      GD_PICKAXE,
      GD_SHOVEL,
      GD_CRUCIBLE,
      GD_RODANDLINE,
      GD_SCYTHE,
      GD_CLEAVER,
      GD_ROLLINGPIN,
      GD_BOW,
    });

    const std::vector<GoodType> commonGoodPool({
      GD_BEER,
      GD_WATER,
      GD_BOAT,
      GD_SWORD,
      GD_IRON,
      GD_FLOUR,
      GD_FISH,
      GD_BREAD,
      GD_WOOD,
      GD_BOARDS,
      GD_STONES,
      GD_GRAIN,
      GD_COINS,
      GD_GOLD,
      GD_IRONORE,
      GD_COAL,
      GD_MEAT,
      GD_HAM,
    });

    constexpr unsigned numGoodTypesToCollect = 7;

    // Randomly determine *numGoodTypesToCollect* many good types, one of which is a special good (=tool)

    static_assert(numGoodTypesToCollect > 0, "There have to be goods to be collected");
    RTTR_Assert(commonGoodPool.size() >= numGoodTypesToCollect - 1); // "There have to be enough commond goods"
    RTTR_Assert(!specialGoodPool.empty());                        // There have to be enough special goods
    goodsToCollect.clear();
    goodsToCollect.resize(numGoodTypesToCollect);
    auto nextSlot = begin(goodsToCollect);

    while(nextSlot != end(goodsToCollect) - 1)
    {
        GoodType nextGoodType = commonGoodPool[RANDOM.Rand(__FILE__, __LINE__, GetObjId(), commonGoodPool.size())];
        if(std::find(begin(goodsToCollect), nextSlot, nextGoodType) == nextSlot)
        {
            *nextSlot = nextGoodType;
            nextSlot++;
        }
    }
    *nextSlot = specialGoodPool[RANDOM.Rand(__FILE__, __LINE__, GetObjId(), specialGoodPool.size())];

    // Schedule end game event and trust the event manager to keep track of it
    if(!isInfinite())
        GetEvMgr().AddEvent(this, endFrame);

    // Find and set up Teams and trackers
    DetermineTeams();
    amountsThePlayersCollected.resize(goodsToCollect.size());
    maxAmountsATeamCollected.resize(goodsToCollect.size());

    // Send Mission Goal
    for(unsigned p = 0; p < gwg->GetNumPlayers(); ++p)
    {
        std::string goalText = _("Economy Mode: Collect as much as you can of the following good types: ");
        for(unsigned i = 0; i < numGoodTypesToCollect; i++)
        {
            if(i > 0)
                goalText += ", ";
            goalText += _(WARE_NAMES[goodsToCollect[i]]);
        }
        goalText += ". ";
        goalText += _("Tools in the hands of workers are also counted. So are weapons, and beer, that soldiers have in "
                      "use. For an updating tally of the collected goods see the economic progress window.");

        gwg->GetPostMgr().SetMissionGoal(p, goalText);
    }
}

EconomyModeHandler::EconomyModeHandler(SerializedGameData& sgd, unsigned objId)
    : GameObject(sgd, objId), endFrame(sgd.PopUnsignedInt()), gfLastUpdated(0)
{
    sgd.PopContainer(goodsToCollect);

    std::vector<unsigned> teamBitMasks;
    sgd.PopContainer(teamBitMasks);
    for(auto& teamMask : teamBitMasks)
    {
        economyModeTeams.emplace_back(teamMask, goodsToCollect.size());
    }

    amountsThePlayersCollected.resize(goodsToCollect.size());
    maxAmountsATeamCollected.resize(goodsToCollect.size());
}

void EconomyModeHandler::Destroy() {}

/// Serialisierungsfunktion
void EconomyModeHandler::Serialize(SerializedGameData& sgd) const
{
    sgd.PushUnsignedInt(endFrame);
    sgd.PushContainer(goodsToCollect);
    std::vector<unsigned> teamBitMasks;
    for(const EconomyModeHandler::EconTeam& curTeam : economyModeTeams)
    {
        teamBitMasks.push_back(curTeam.playersInTeam.to_ulong());
    }
    sgd.PushContainer(teamBitMasks);
}

void EconomyModeHandler::DetermineTeams()
{
    // If we already determined who is in a team with whom skip this. For the economy mode we only count teams at game
    // start
    if(!economyModeTeams.empty())
        return;
    for(unsigned i = 0; i < gwg->GetNumPlayers(); ++i)
    {
        if(gwg->GetPlayer(i).isUsed())
        {
            bool foundTeam = false;
            for(const auto& team : economyModeTeams)
            {
                if(team.containsPlayer(i))
                {
                    foundTeam = true;
                    break;
                }
            }
            if(!foundTeam)
            {
                const GamePlayer& player = gwg->GetPlayer(i);
                std::bitset<MAX_PLAYERS> newTeam;
                newTeam.set(i);
                for(unsigned j = i + 1; j < gwg->GetNumPlayers(); ++j)
                {
                    if(gwg->GetPlayer(j).isUsed() && player.IsAlly(j))
                    {
                        newTeam.set(j);
                    }
                }
                economyModeTeams.emplace_back(newTeam, goodsToCollect.size());
            }
        }
    }
}

unsigned EconomyModeHandler::SumUpGood(GoodType good, const Inventory& Inventory)
{
    unsigned retVal = Inventory.goods[good];

    // Add the tools used by workers to the good totals
    for(unsigned j = 0; j < NUM_JOB_TYPES; j++)
    {
        boost::optional<GoodType> tool = JOB_CONSTS[(Job)j].tool;
        if(tool == good)
        {
            retVal += Inventory.people[j];
        }
    }
    // Add the weapons and beer used by soldiers to the good totals
    if(good == GD_BEER || good == GD_SWORD || good == GD_SHIELDROMANS)
    {
        for(const auto& it : SOLDIER_JOBS)
        {
            retVal += Inventory.people[it];
        }
    }

    return retVal;
}

void EconomyModeHandler::UpdateAmounts()
{
    // Return if the game is over or we already updated the amounts this game frame
    if(isOver() || gfLastUpdated == GetEvMgr().GetCurrentGF())
    {
        return;
    }

    // Sum up goods
    for(unsigned i = 0; i < gwg->GetNumPlayers(); ++i)
    {
        const GamePlayer& player = gwg->GetPlayer(i);
        Inventory playerInventory = player.GetInventory();
        for(unsigned g = 0; g < goodsToCollect.size(); g++)
        {
            amountsThePlayersCollected[g][i] = SumUpGood(goodsToCollect[g], playerInventory);
        }
    }

    // Compute Teams
    DetermineTeams();

    // Compute the amounts for the teams
    std::fill(maxAmountsATeamCollected.begin(), maxAmountsATeamCollected.end(), 0);
    for(auto& team : economyModeTeams)
    {
        std::fill(team.amountsTheTeamCollected.begin(), team.amountsTheTeamCollected.end(), 0);
        for(unsigned i = 0; i < gwg->GetNumPlayers(); ++i)
        {
            if(team.containsPlayer(i))
            {
                for(unsigned g = 0; g < goodsToCollect.size(); g++)
                {
                    team.amountsTheTeamCollected[g] += GetAmount(g, i);
                    if(team.amountsTheTeamCollected[g] > maxAmountsATeamCollected[g])
                    {
                        maxAmountsATeamCollected[g] = team.amountsTheTeamCollected[g];
                    }
                }
            }
        }
    }
    // Determine the leading teams for each good type and determine how many good type wins is the maximum.
    mostGoodTypeWins = 0;
    for(auto& team : economyModeTeams)
    {
        team.goodTypeWins = 0;
        for(unsigned g = 0; g < goodsToCollect.size(); g++)
        {
            if(team.amountsTheTeamCollected[g] >= maxAmountsATeamCollected[g])
            {
                team.goodTypeWins++;
                if(team.goodTypeWins > mostGoodTypeWins)
                {
                    mostGoodTypeWins = team.goodTypeWins;
                }
            }
        }
    }

    gfLastUpdated = GetEvMgr().GetCurrentGF();
}

void EconomyModeHandler::HandleEvent(const unsigned)
{
    if(isOver())
    {
        return;
    }

    // Handle game end event

    // Update one last time
    UpdateAmounts();

    // Determine bitmask of all players in teams with the most good type wins
    std::bitset<MAX_PLAYERS> bestMask;
    for(auto& team : economyModeTeams)
    {
        if(team.goodTypeWins == mostGoodTypeWins)
        {
            bestMask |= team.playersInTeam;
        }
    }

    // Let players know who won
    if(bestMask.count() > 1)
        gwg->GetGameInterface()->GI_TeamWinner(bestMask.to_ulong());
    else
        for(unsigned i = 0; i < gwg->GetNumPlayers(); ++i)
        {
            if(bestMask[i])
            {
                gwg->GetGameInterface()->GI_Winner(i);
            }
        }

    gwg->MakeWholeMapVisibleForAllPlayers();
    gwg->GetGameInterface()->GI_UpdateMapVisibility();
}

bool EconomyModeHandler::isOver() const
{
    return gwg->GetGGS().objective == GO_ECONOMYMODE && !isInfinite() && endFrame < GetEvMgr().GetCurrentGF();
}

EconomyModeHandler::EconTeam::EconTeam(SerializedGameData& sgd, unsigned numGoodTypesToCollect)
    : playersInTeam(sgd.PopSignedInt()), amountsTheTeamCollected(numGoodTypesToCollect, 0), goodTypeWins(0)
{}

void EconomyModeHandler::EconTeam::Serialize(SerializedGameData& sgd) const
{
    sgd.PushUnsignedInt(playersInTeam.to_ulong());
}

bool EconomyModeHandler::EconTeam::containsPlayer(unsigned playerId) const
{
    return playersInTeam[playerId];
}
