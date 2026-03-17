// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CombatEventLogger.h"

#include "EventLogBatchWriter.h"
#include "gameData/BuildingConsts.h"
#include <map>
#include <sstream>
#include <unordered_map>

namespace {

constexpr std::array<char, NUM_SOLDIER_RANKS> kRankLabels = {'P', 'F', 'S', 'O', 'G'};
EventLogBatchWriter gCombatLog("combat_log.txt");

std::string FormatRankCounts(const std::array<unsigned, NUM_SOLDIER_RANKS>& counts)
{
    std::string result;
    for(int idx = static_cast<int>(NUM_SOLDIER_RANKS) - 1; idx >= 0; --idx)
    {
        const unsigned count = counts[idx];
        if(count == 0)
            continue;
        if(!result.empty())
            result += ",";
        result.push_back(kRankLabels[idx]);
        result.push_back('-');
        result += std::to_string(count);
    }
    if(result.empty())
        result = "none";
    return result;
}

std::string FormatRank(const unsigned rank)
{
    if(rank < kRankLabels.size())
        return std::string(1, kRankLabels[rank]);
    return "?";
}

struct DestroyedBuilding
{
    BuildingType type;
    unsigned buildingObjId;
};

std::string FormatDestroyedBuildings(const std::vector<DestroyedBuilding>& destroyed)
{
    if(destroyed.empty())
        return "none";

    std::string result;
    for(std::size_t i = 0; i < destroyed.size(); ++i)
    {
        if(i != 0)
            result += ",";
        result += BUILDING_NAMES_1.at(destroyed[i].type);
        result += " ";
        result += std::to_string(destroyed[i].buildingObjId);
    }
    return result;
}

std::string FormatSources(const std::vector<CombatEventLogger::AttackSource>& sources)
{
    if(sources.empty())
        return "none";

    std::string result;
    bool first = true;
    for(const CombatEventLogger::AttackSource& source : sources)
    {
        if(!first)
            result += ",";
        first = false;
        result += BUILDING_NAMES_1.at(source.buildingType);
        result += " ";
        result += std::to_string(source.buildingObjId);
        result += ":";
        result += std::to_string(source.count);
        result += "(";
        result += FormatRankCounts(source.byRank);
        result += ")";
    }
    return result;
}

struct CaptureDestroyedData
{
    std::vector<DestroyedBuilding> destroyed;
};

std::unordered_map<unsigned, CaptureDestroyedData> gCaptureDestroyed;

} // namespace

namespace CombatEventLogger {

void LogAttackOrder(unsigned gf, unsigned char attackerPlayer, unsigned char defenderPlayer, BuildingType targetType,
                    unsigned targetObjId, bool strongSoldiers, unsigned desiredCount, unsigned actualCount,
                    const std::array<unsigned, NUM_SOLDIER_RANKS>& actualByRank,
                    const std::vector<AttackSource>& sources)
{
    std::ostringstream line;
    line << gf << " ATTACK_ORDER attacker=" << static_cast<unsigned>(attackerPlayer + 1)
         << " defender=" << static_cast<unsigned>(defenderPlayer + 1) << " target=" << BUILDING_NAMES_1.at(targetType)
         << " " << targetObjId << " strong=" << (strongSoldiers ? "true" : "false") << " desired=" << desiredCount
         << " actual=" << actualCount << " actual_by_rank=" << FormatRankCounts(actualByRank)
         << " sources=" << FormatSources(sources);
    gCombatLog.Append(gf, line.str());
}

void LogAggressiveDefenderOrder(unsigned gf, unsigned char attackerPlayer, BuildingType targetType,
                                unsigned targetObjId, unsigned char defenderPlayer,
                                const std::vector<AttackSource>& sources, unsigned char defenderRank)
{
    std::ostringstream line;
    line << gf << " AGG_DEFENDER_ORDER attacker=" << static_cast<unsigned>(attackerPlayer + 1)
         << " defender=" << static_cast<unsigned>(defenderPlayer + 1) << " target=" << BUILDING_NAMES_1.at(targetType)
         << " " << targetObjId << " defender_rank=" << FormatRank(defenderRank)
         << " sources=" << FormatSources(sources);
    gCombatLog.Append(gf, line.str());
}

void LogFightResult(unsigned gf, unsigned char attackerPlayer, BuildingType targetType, unsigned targetObjId,
                    unsigned char defenderPlayer, unsigned attackerRank, unsigned attackerStartHp, unsigned defenderRank,
                    unsigned defenderStartHp, const char* winnerRole, unsigned winnerRemainingHp)
{
    std::ostringstream line;
    line << gf << " FIGHT attacker=" << static_cast<unsigned>(attackerPlayer + 1)
         << " defender=" << static_cast<unsigned>(defenderPlayer + 1) << " target=";
    if(targetObjId == 0)
        line << "Unknown";
    else
        line << BUILDING_NAMES_1.at(targetType) << " " << targetObjId;
    line << " attacker_rank=" << FormatRank(attackerRank) << " attacker_hp=" << attackerStartHp
         << " defender_rank=" << FormatRank(defenderRank) << " defender_hp=" << defenderStartHp
         << " winner=" << winnerRole << " winner_hp=" << winnerRemainingHp;
    gCombatLog.Append(gf, line.str());
}

void RecordCaptureDestroyed(const unsigned capturingObjId, const BuildingType type, const unsigned destroyedObjId)
{
    if(!capturingObjId)
        return;
    gCaptureDestroyed[capturingObjId].destroyed.push_back(DestroyedBuilding{type, destroyedObjId});
}

void LogCapture(unsigned gf, unsigned char attackerPlayer, unsigned char defenderPlayer, BuildingType buildingType,
                unsigned buildingObjId)
{
    auto it = gCaptureDestroyed.find(buildingObjId);
    std::vector<DestroyedBuilding> destroyed;
    if(it != gCaptureDestroyed.end())
    {
        destroyed = std::move(it->second.destroyed);
        gCaptureDestroyed.erase(it);
    }
    std::ostringstream line;
    line << gf << " CAPTURE attacker=" << static_cast<unsigned>(attackerPlayer + 1)
         << " defender=" << static_cast<unsigned>(defenderPlayer + 1)
         << " target=" << BUILDING_NAMES_1.at(buildingType) << " " << buildingObjId
         << " destroyed=" << FormatDestroyedBuildings(destroyed);
    gCombatLog.Append(gf, line.str());
}

} // namespace CombatEventLogger
