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
#include "random/Random.h"
#include "world/GameWorld.h"
#include "world/GameWorldGame.h"
#include "gameTypes/JobTypes.h"
#include "gameData/JobConsts.h"
#include <boost/optional.hpp>

auto getPlayerMask = [](unsigned playerId) { return 1u << playerId; };

GoodType types_less[]{
  /*  1 */ GD_TONGS,      // Zange
  /*  2 */ GD_HAMMER,     // Hammer
  /*  3 */ GD_AXE,        // Axt
  /*  4 */ GD_SAW,        // Saege
  /*  5 */ GD_PICKAXE,    // Spitzhacke
  /*  6 */ GD_SHOVEL,     // Schaufel
  /*  7 */ GD_CRUCIBLE,   // Schmelztiegel
  /*  8 */ GD_RODANDLINE, // Angel
  /*  9 */ GD_SCYTHE,     // Sense
  /* 12 */ GD_CLEAVER,    // Beil
  /* 13 */ GD_ROLLINGPIN, // Nudelholz
  /* 14 */ GD_BOW,        // Bogen
};

GoodType types_more[]{
  /*  0 */ GD_BEER,    // Bier
  /* 11 */ GD_WATER,   // Wasser
  /* 15 */ GD_BOAT,    // Boot
  /* 16 */ GD_SWORD,   // Schwert
  /* 17 */ GD_IRON,    // Eisen
  /* 18 */ GD_FLOUR,   // Mehl
  /* 19 */ GD_FISH,    // Fisch
  /* 20 */ GD_BREAD,   // Brot
  /* 22 */ GD_WOOD,    // Holz
  /* 23 */ GD_BOARDS,  // Bretter
  /* 24 */ GD_STONES,  // Steine
  /* 27 */ GD_GRAIN,   // Getreide
  /* 28 */ GD_COINS,   // Mnzen
  /* 29 */ GD_GOLD,    // Gold
  /* 30 */ GD_IRONORE, // Eisenerz
  /* 31 */ GD_COAL,    // Kohle
  /* 32 */ GD_MEAT,    // Fleisch
  /* 33 */ GD_HAM,     // Schinken ( Schwein )
};

EconomyModeHandler::EconomyModeHandler(unsigned end_frame) : end_frame(end_frame), last_updated(0)
{
    constexpr unsigned num_types_less = sizeof(types_less) / sizeof(types_less[0]);
    constexpr unsigned num_types_more = sizeof(types_more) / sizeof(types_more[0]);

    // Randomly determine *numGoodTypesToCollect* many good types, one of which is a tool
    unsigned int types_found = 0;

    while(types_found < numGoodTypesToCollect - 1)
    {
        GoodType next_type = types_more[RANDOM.Rand(__FILE__, __LINE__, GetObjId(), num_types_more)];
        unsigned i = 0;
        for(; i < types_found; i++)
        {
            if(types[i] == next_type)
            {
                break;
            }
        }
        if(i == types_found)
        {
            types[types_found] = next_type;
            types_found++;
        }
    }
    types[numGoodTypesToCollect - 1] = types_less[RANDOM.Rand(__FILE__, __LINE__, GetObjId(), num_types_less)];
    types_found++;

    if(end_frame > 0)
        event = GetEvMgr().AddEvent(this, end_frame);
    else
        event = nullptr;

    // Send Mission Goal
    for(unsigned p = 0; p < gwg->GetNumPlayers(); ++p)
    {
        std::string goaltext = _("Economy Mode: Collect as much as you can of the following good types: ");
        for(unsigned i = 0; i < numGoodTypesToCollect; i++)
        {
            if(i > 0)
                goaltext += ", ";
            goaltext += _(WARE_NAMES[types[i]]);
        }
        goaltext += ". ";
        goaltext += _("Tools in the hands of workers are also counted. So are weapons, and beer, that soldiers have in "
                      "use. For an updating tally of the collected goods see the economic progress window.");

        gwg->GetPostMgr().SetMissionGoal(p, goaltext);
    }
}

EconomyModeHandler::EconomyModeHandler(SerializedGameData& sgd, unsigned objId)
    : GameObject(sgd, objId), end_frame(sgd.PopUnsignedInt()), last_updated(0)
{
    if(!isOver())
    {
        event = sgd.PopEvent();
    } else
    {
        event = nullptr;
    }
    for(auto& type : types)
    {
        type = (GoodType)sgd.PopUnsignedChar();
    }
}

void EconomyModeHandler::Destroy() {}

/// Serialisierungsfunktion
void EconomyModeHandler::Serialize(SerializedGameData& sgd) const
{
    sgd.PushUnsignedInt(end_frame);
    if(!isOver())
    {
        sgd.PushEvent(event);
    }
    for(auto type : types)
    {
        sgd.PushUnsignedChar(type);
    }
}

void EconomyModeHandler::FindTeams()
{
    // If we already determined who is in a team with whom skip this. For the economy mode we only count teams at game
    // start
    if(!teams.empty())
        return;
    for(unsigned i = 0; i < gwg->GetNumPlayers(); ++i)
    {
        if(gwg->GetPlayer(i).isUsed())
        {
            bool found_team = false;
            for(unsigned t = 0; t < teams.size(); ++t)
            {
                if(teams[t].mask & getPlayerMask(i))
                {
                    found_team = true;
                    break;
                }
            }
            if(!found_team)
            {
                const GamePlayer& player = gwg->GetPlayer(i);
                unsigned new_team = getPlayerMask(i);
                unsigned num_players_in_team = 1;
                for(unsigned j = i + 1; j < gwg->GetNumPlayers(); ++j)
                {
                    if(gwg->GetPlayer(j).isUsed() && player.IsAlly(j))
                    {
                        num_players_in_team++;
                        new_team = new_team | getPlayerMask(j);
                    }
                }
                teams.emplace_back(new_team, num_players_in_team);
            }
        }
    }
}

unsigned int EconomyModeHandler::SumGood(GoodType good, const Inventory& Inventory)
{
    unsigned int retVal = Inventory.goods[good];

    // Add the tools used by workers to the good totals
    for(unsigned int j = 0; j < NUM_JOB_TYPES; j++)
    {
        boost::optional<GoodType> tool = JOB_CONSTS[(Job)j].tool;
        if(tool && tool == good)
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
    if((isOver() && end_frame != 0) || last_updated == GetEvMgr().GetCurrentGF())
    {
        return;
    }

    // Sum up goods
    for(unsigned i = 0; i < gwg->GetNumPlayers(); ++i)
    {
        const GamePlayer& player = gwg->GetPlayer(i);
        Inventory playerInventory = player.GetInventory();
        for(unsigned int g = 0; g < numGoodTypesToCollect; g++)
        {
            amounts[g][i] = SumGood(types[g], playerInventory);
        }
    }

    // Compute Teams
    FindTeams();

    // Compute the amounts for the teams
    for(unsigned int& maxTeamAmount : maxTeamAmounts)
    {
        maxTeamAmount = 0;
    }

    for(unsigned t = 0; t < teams.size(); ++t)
    {
        for(unsigned int g = 0; g < numGoodTypesToCollect; g++)
        {
            teams[t].teamAmounts[g] = 0;
        }
        for(unsigned i = 0; i < gwg->GetNumPlayers(); ++i)
        {
            if(teams[t].mask & getPlayerMask(i))
            {
                for(unsigned int g = 0; g < numGoodTypesToCollect; g++)
                {
                    teams[t].teamAmounts[g] += GetAmount(g, i);
                    if(teams[t].teamAmounts[g] > maxTeamAmounts[g])
                    {
                        maxTeamAmounts[g] = teams[t].teamAmounts[g];
                    }
                }
            }
        }
    }
    // Determine the leading teams for each good type and determine how many good type wins is the maximum.
    mostWins = 0;
    for(auto& team : teams)
    {
        team.teamWins = 0;
        for(unsigned int g = 0; g < numGoodTypesToCollect; g++)
        {
            if(team.teamAmounts[g] >= maxTeamAmounts[g])
            {
                team.teamWins++;
                if(team.teamWins > mostWins)
                {
                    mostWins = team.teamWins;
                }
            }
        }
    }

    last_updated = GetEvMgr().GetCurrentGF();
}

bool EconomyModeHandler::globalVisibility()
{
    return gwg->GetGGS().objective == GO_ECONOMYMODE && gwg->GetEvMgr().GetCurrentGF() >= end_frame
           && GetEndFrame() > 0;
}

bool EconomyModeHandler::isOver() const
{
    return gwg->GetGGS().objective == GO_ECONOMYMODE && end_frame < GetEvMgr().GetCurrentGF();
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

    // Determine mask of all players in teams with the most good type wins
    unsigned bestMask = 0;
    unsigned int numWinners = 0;
    for(auto& team : teams)
    {
        if(team.teamWins == mostWins)
        {
            bestMask = bestMask | team.mask;
            numWinners += team.num_players_in_team;
        }
    }

    // Let players know who won
    if(bestMask && numWinners != 1)
        gwg->GetGameInterface()->GI_TeamWinner(bestMask);
    else
        for(unsigned i = 0; i < gwg->GetNumPlayers(); ++i)
        {
            if(bestMask & getPlayerMask(i))
            {
                gwg->GetGameInterface()->GI_Winner(i);
            }
        }

    // Call function to recalculcate visibilities
    for(unsigned i = 0; i < gwg->GetNumPlayers(); ++i)
    {
        gwg->GetGameInterface()->GI_TreatyOfAllianceChanged(i); // TODO: Is this abuse? Should we rename that function?
    }
    event = nullptr;
}

bool EconomyModeHandler::econTeam::inTeam(unsigned int playerId) const
{
    return mask & getPlayerMask(playerId);
}
