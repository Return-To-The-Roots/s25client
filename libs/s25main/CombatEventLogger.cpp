// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CombatEventLogger.h"

#include "ai/aijh/StatsConfig.h"
#include "gameData/BuildingConsts.h"
#include <boost/filesystem/path.hpp>
#include <fstream>
#include <map>
#include <unordered_map>

namespace {

constexpr std::array<char, NUM_SOLDIER_RANKS> kRankLabels = {'P', 'F', 'S', 'O', 'G'};

std::ofstream OpenCombatLog()
{
    if(STATS_CONFIG.disableEventLogging || STATS_CONFIG.statsPath.empty())
        return {};
    const boost::filesystem::path path = boost::filesystem::path(STATS_CONFIG.statsPath) / "combat_log.txt";
    return std::ofstream(path.string(), std::ios::app);
}

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

std::string FormatDestroyedBuildings(const std::map<BuildingType, unsigned>& destroyed)
{
    if(destroyed.empty())
        return "none";

    std::string result;
    bool first = true;
    for(const auto& entry : destroyed)
    {
        if(!first)
            result += ",";
        first = false;
        result += BUILDING_NAMES_1.at(entry.first);
        result += ": ";
        result += std::to_string(entry.second);
    }
    return result;
}

struct CaptureDestroyedData
{
    std::map<BuildingType, unsigned> destroyed;
};

std::unordered_map<unsigned, CaptureDestroyedData> gCaptureDestroyed;

} // namespace

namespace CombatEventLogger {

void LogAttackOrder(unsigned gf, unsigned char attackerPlayer, unsigned char defenderPlayer, BuildingType targetType,
                    unsigned targetObjId, bool strongSoldiers, unsigned desiredCount, unsigned actualCount,
                    const std::array<unsigned, NUM_SOLDIER_RANKS>& actualByRank)
{
    std::ofstream log = OpenCombatLog(  );
    if(!log)
        return;
    log << gf << " ATTACK_ORDER attacker=" << static_cast<unsigned>(attackerPlayer + 1)
        << " defender=" << static_cast<unsigned>(defenderPlayer + 1) << " target=" << BUILDING_NAMES_1.at(targetType)
        << " " << targetObjId << " strong=" << (strongSoldiers ? "true" : "false") << " desired=" << desiredCount
        << " actual=" << actualCount << " actual_by_rank=" << FormatRankCounts(actualByRank) << std::endl;
}

void LogAggressiveDefenderOrder(unsigned gf, unsigned char attackerPlayer, BuildingType targetType,
                                unsigned targetObjId, unsigned char defenderPlayer, BuildingType defenderHomeType,
                                unsigned defenderHomeObjId, unsigned char defenderRank)
{
    std::ofstream log = OpenCombatLog();
    if(!log)
        return;
    log << gf << " AGG_DEFENDER_ORDER attacker=" << static_cast<unsigned>(attackerPlayer + 1)
        << " defender=" << static_cast<unsigned>(defenderPlayer + 1) << " target=" << BUILDING_NAMES_1.at(targetType)
        << " " << targetObjId
        << " defender_home=" << BUILDING_NAMES_1.at(defenderHomeType) << " " << defenderHomeObjId
        << " defender_rank=" << FormatRank(defenderRank) << std::endl;
}

void LogFightResult(unsigned gf, unsigned char attackerPlayer, BuildingType targetType, unsigned targetObjId,
                    unsigned char defenderPlayer, unsigned attackerRank, unsigned attackerStartHp, unsigned defenderRank,
                    unsigned defenderStartHp, const char* winnerRole, unsigned winnerRemainingHp)
{
    std::ofstream log = OpenCombatLog();
    if(!log)
        return;
    log << gf << " FIGHT attacker=" << static_cast<unsigned>(attackerPlayer + 1)
        << " defender=" << static_cast<unsigned>(defenderPlayer + 1) << " target=";
    if(targetObjId == 0)
        log << "Unknown";
    else
        log << BUILDING_NAMES_1.at(targetType) << " " << targetObjId;
    log << " attacker_rank=" << FormatRank(attackerRank) << " attacker_hp=" << attackerStartHp
        << " defender_rank=" << FormatRank(defenderRank) << " defender_hp=" << defenderStartHp
        << " winner=" << winnerRole << " winner_hp=" << winnerRemainingHp << std::endl;
}

void RecordCaptureDestroyed(const unsigned capturingObjId, const BuildingType type)
{
    if(!capturingObjId)
        return;
    gCaptureDestroyed[capturingObjId].destroyed[type]++;
}

void LogCapture(unsigned gf, unsigned char attackerPlayer, unsigned char defenderPlayer, BuildingType buildingType,
                unsigned buildingObjId)
{
    std::ofstream log = OpenCombatLog();
    if(!log)
        return;
    auto it = gCaptureDestroyed.find(buildingObjId);
    std::map<BuildingType, unsigned> destroyed;
    if(it != gCaptureDestroyed.end())
    {
        destroyed = std::move(it->second.destroyed);
        gCaptureDestroyed.erase(it);
    }
    log << gf << " CAPTURE attacker=" << static_cast<unsigned>(attackerPlayer + 1)
        << " defender=" << static_cast<unsigned>(defenderPlayer + 1)
        << " target=" << BUILDING_NAMES_1.at(buildingType) << " " << buildingObjId
        << " destroyed=" << FormatDestroyedBuildings(destroyed) << std::endl;
}

} // namespace CombatEventLogger
