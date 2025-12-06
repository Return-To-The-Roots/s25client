// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AIPlayerJH.h"

#include "BasePlayerInfo.h"
#include "BuildingPlanner.h"
#include "BuildingRegister.h"
#include "CombatLossTracker.h"
#include "GamePlayer.h"
#include "Jobs.h"
#include "StatsConfig.h"
#include "buildings/noBaseBuilding.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobUsual.h"
#include "figures/nofAttacker.h"
#include "helpers/EnumRange.h"
#include "gameData/BuildingConsts.h"
#include "gameData/BuildingProperties.h"
#include "gameData/GameConsts.h"
#include "gameData/JobConsts.h"
#include "gameData/MilitaryConsts.h"
#include "gameData/ToolConsts.h"

#include "s25util/colors.h"

#include <boost/filesystem/path.hpp>
#include <boost/format.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::map<BuildingType, int> GetBuildingsMap(AIJH::BuildingPlanner bldPlanner)
{
    std::map<BuildingType, int> bldMap;
    for(auto type : helpers::EnumRange<BuildingType>{})
    {
        bldMap[type] = bldPlanner.GetNumBuildings(type);
    }
    return bldMap;
}

std::map<BuildingType, int> GetBuildingsSiteMap(AIJH::BuildingPlanner bldPlanner)
{
    std::map<BuildingType, int> bldMap;
    for(auto type : helpers::EnumRange<BuildingType>{})
    {
        bldMap[type] = bldPlanner.GetNumBuildingSites(type);
    }
    return bldMap;
}

std::map<BuildingType, int> GetBuildingsWantedMap(AIJH::BuildingPlanner bldPlanner)
{
    std::map<BuildingType, int> bldMap;
    for(auto type : helpers::EnumRange<BuildingType>{})
    {
        bldMap[type] = bldPlanner.GetNumAdditionalBuildingsWanted(type);
    }
    return bldMap;
}

std::ofstream createCsvFile(std::string name)
{
    boost::format fmt("%s/%s.csv");
    fmt % STATS_CONFIG.statsPath % name;
    std::ofstream file(fmt.str(), std::ios::app);

    if(!file)
    {
        std::cerr << "Unable open " << name << " buildingFile for appending!" << std::endl;
    }
    return file;
}

std::string CurrentTimestamp()
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&now_time_t);
    std::stringstream ss;
    ss << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S");

    return ss.str();
}

std::string FormatRankCounts(const std::array<unsigned, NUM_SOLDIER_RANKS>& counts)
{
    static constexpr std::array<char, NUM_SOLDIER_RANKS> rankLabels = {'P', 'F', 'S', 'O', 'G'};
    std::string result;
    for(int idx = static_cast<int>(NUM_SOLDIER_RANKS) - 1; idx >= 0; --idx)
    {
        const unsigned count = counts[idx];
        if(count == 0)
            continue;
        if(!result.empty())
            result += ",";
        result.push_back(rankLabels[idx]);
        result.push_back('-');
        result += std::to_string(count);
    }
    if(result.empty())
        result = "none";
    return result;
}

std::string FormatDestroyedBuildings(const std::map<BuildingType, unsigned>& destroyed)
{
    if(destroyed.empty())
        return "none";

    std::string result;
    bool first = true;
    for(const auto& entry : destroyed)
    {
        if(entry.second == 0)
            continue;
        if(!first)
            result += ",";
        first = false;
        result += BUILDING_NAMES_1.at(entry.first);
        result += ": ";
        result += std::to_string(entry.second);
    }

    if(result.empty())
        result = "none";
    return result;
}
} // namespace

namespace AIJH {

void AIPlayerJH::TrackCombatStart(const nobBaseMilitary& target)
{
    const unsigned objId = target.GetObjId();
    const auto it = std::find_if(activeCombats_.begin(), activeCombats_.end(),
                                 [objId](const ActiveCombat& combat) { return combat.targetObjId == objId; });
    if(it != activeCombats_.end())
        return;

    CombatLossTracker::RegisterCombat(objId);
    activeCombats_.push_back({target.GetPos(), objId, target.GetPlayer(), target.GetBuildingType()});
}

std::string AIPlayerJH::GetCombatsLogPath() const
{
    fs::path filePath = fs::path(STATS_CONFIG.statsPath) / "combats.txt";
    return filePath.string();
}

std::string AIPlayerJH::FormatPlayerLabel(unsigned playerIdx) const
{
    const GamePlayer& pl = gwb.GetPlayer(playerIdx);
    const unsigned color = pl.color;
    const int colorIdx = BasePlayerInfo::GetColorIdx(color);
    static constexpr std::array<const char*, PLAYER_COLORS.size()> colorNames =
      {"Blue", "Yellow", "Red", "Magenta", "Black", "Green", "Orange", "Cyan", "White", "Brown", "Purple"};
    const std::string colorName =
      (colorIdx >= 0 && static_cast<size_t>(colorIdx) < colorNames.size()) ? colorNames[colorIdx] : "Unknown";
    const std::string nationName = _(NationNames[pl.nation]);
    std::ostringstream ss;
    ss << "Player " << (playerIdx + 1) << " - " << colorName << " " << nationName;
    return ss.str();
}

void AIPlayerJH::LogPlayerMetadata(std::ofstream& combatsFile) const
{
    bool first = true;
    for(unsigned playerIdx = 0; playerIdx < gwb.GetNumPlayers(); ++playerIdx)
    {
        const GamePlayer& player = gwb.GetPlayer(playerIdx);
        if(player.ps != PlayerState::Occupied)
            continue;
        if(!first)
            combatsFile << ", ";
        combatsFile << FormatPlayerLabel(playerIdx);
        first = false;
    }
    combatsFile << std::endl;
}

bool AIPlayerJH::HasOwnAggressors(const nobBaseMilitary& building) const
{
    for(const nofAttacker* attacker : building.GetAggressors())
    {
        if(attacker && attacker->GetPlayer() == playerId)
            return true;
    }
    return false;
}

AIPlayerJH::CombatLogState AIPlayerJH::EvaluateCombatState(const ActiveCombat& combat) const
{
    const noBase* nodeObj = gwb.GetNO(combat.pos);
    const auto* building = dynamic_cast<const nobBaseMilitary*>(nodeObj);
    if(!building)
        return CombatLogState::Success;

    if(building->GetObjId() != combat.targetObjId)
        return CombatLogState::Success;

    if(HasOwnAggressors(*building))
        return CombatLogState::Pending;

    const unsigned char owner = building->GetPlayer();
    if(owner == playerId)
        return CombatLogState::Success;
    if(owner != combat.defenderPlayer)
        return CombatLogState::Success;
    return CombatLogState::Failure;
}

void AIPlayerJH::LogFinishedCombats(const unsigned gf) const
{
    if(activeCombats_.empty())
        return;

    std::vector<std::pair<ActiveCombat, bool>> finishedCombats;
    finishedCombats.reserve(activeCombats_.size());
    auto it = activeCombats_.begin();
    while(it != activeCombats_.end())
    {
        const CombatLogState state = EvaluateCombatState(*it);
        if(state == CombatLogState::Pending)
        {
            ++it;
            continue;
        }

        finishedCombats.emplace_back(*it, state == CombatLogState::Success);
        it = activeCombats_.erase(it);
    }

    if(finishedCombats.empty())
        return;

    std::ofstream combatsFile(GetCombatsLogPath(), std::ios::app);
    if(!combatsFile)
    {
        std::cerr << "Unable to open combats log file for appending!" << std::endl;
        return;
    }

    for(const auto& entry : finishedCombats)
    {
        const ActiveCombat& combat = entry.first;
        const bool success = entry.second;
        const CombatStats stats = CombatLossTracker::TakeStats(combat.targetObjId);
        combatsFile << "#" << gf << " Player #" << static_cast<unsigned>(playerId + 1) << " attacks Player #"
                    << static_cast<unsigned>(combat.defenderPlayer + 1) << " " << BUILDING_NAMES_1.at(combat.buildingType)
                    << ". Attack #" << (success ? "succed" : "failed") << " Forces: Attacker "
                    << FormatRankCounts(stats.attackerForces) << " . Defender " << FormatRankCounts(stats.defenderForces)
                    << " Losses: Attacker " << FormatRankCounts(stats.attackerLosses) << " . Defender "
                    << FormatRankCounts(stats.defenderLosses);
        if(success)
            combatsFile << " Destroyed: " << FormatDestroyedBuildings(stats.destroyedBuildings);
        combatsFile << std::endl;
    }
}

void AIPlayerJH::saveStats(unsigned int gf) const
{
    if(player.ps == PlayerState::Locked)
    {
        return;
    }

    if(gf == 0)
    {
        std::ofstream combatsFile(GetCombatsLogPath(), std::ios::trunc);
        if(!combatsFile)
            std::cerr << "Unable to open combats log file for writing!" << std::endl;
        else
        {
            LogPlayerMetadata(combatsFile);
        }
    }
    LogFinishedCombats(gf);

    std::ofstream buildingCountFile = createCsvFile("buildings_count");
    std::ofstream buildingSitesFile = createCsvFile("buildings_sites");
    std::ofstream productivityFile = createCsvFile("productivity");
    std::ofstream goodsFile = createCsvFile("goods");
    std::ofstream jobsFile = createCsvFile("jobs");
    std::ofstream scoreFile = createCsvFile("score");
    std::ofstream otherFile = createCsvFile("other");

    if(gf == 0)
    {
        buildingCountFile << "GameFrame";
        for(BuildingType type : helpers::EnumRange<BuildingType>{})
        {
            buildingCountFile << "," << BUILDING_NAMES_1.at(type) << "Count";
        }
        buildingCountFile << std::endl;

        buildingSitesFile << "GameFrame";
        for(BuildingType type : helpers::EnumRange<BuildingType>{})
        {
            buildingSitesFile << "," << BUILDING_NAMES_1.at(type) << "Sites";
        }
        buildingSitesFile << std::endl;

        productivityFile << "GameFrame";
        for(BuildingType type : helpers::EnumRange<BuildingType>{})
        {
            productivityFile << "," << BUILDING_NAMES_1.at(type) << "Prod";
        }
        productivityFile << std::endl;

        goodsFile << "GameFrame";
        for(GoodType type : helpers::EnumRange<GoodType>{})
        {
            goodsFile << "," << GOOD_NAMES_1.at(type);
        }
        goodsFile << std::endl;

        jobsFile << "GameFrame";
        for(Job job : helpers::EnumRange<Job>{})
        {
            jobsFile << "," << JOB_NAMES_1.at(job);
        }
        goodsFile << std::endl;

        scoreFile << "GameFrame,Country,Buildings,Military,GoldCoins,Productivity,Kills" << std::endl;

        otherFile << "GameFrame,MilBld,WoodAvailable,StoneAvailable,BoardsDemand,AverageBuildTime";
        for(const Job job : SOLDIER_JOBS)
            otherFile << "," << JOB_NAMES_1.at(job);
        otherFile << std::endl;
    }
    const unsigned previousStatsFrame = lastStatsFrame_;
    const BuildingRegister& buildingRegister = player.GetBuildingRegister();
    const Inventory& playerInventory = player.GetInventory();
    std::uint64_t totalBuildTime = 0;
    unsigned completedBuildings = 0;
    const auto accumulateBuild = [&](const noBaseBuilding& building) {
        const unsigned completionGF = building.GetBuildCompleteFrame();
        if(completionGF == 0 || completionGF <= previousStatsFrame || completionGF > gf)
            return;
        const unsigned startGF = building.GetBuildStartingFrame();
        if(startGF == 0 || completionGF < startGF)
            return;
        totalBuildTime += static_cast<std::uint64_t>(completionGF - startGF);
        ++completedBuildings;
    };

    for(BuildingType type : helpers::EnumRange<BuildingType>{})
    {
        if(BuildingProperties::IsUsual(type))
            for(const nobUsual* bld : buildingRegister.GetBuildings(type))
                accumulateBuild(*bld);
    }
    for(const nobMilitary* bld : buildingRegister.GetMilitaryBuildings())
        accumulateBuild(*bld);
    for(const nobHarborBuilding* bld : buildingRegister.GetHarbors())
        accumulateBuild(*bld);
    for(const nobBaseWarehouse* bld : buildingRegister.GetStorehouses())
        accumulateBuild(*bld);

    const double averageBuildTime = completedBuildings != 0
                                      ? static_cast<double>(totalBuildTime) / static_cast<double>(completedBuildings)
                                      : 0.0;
    scoreFile << gf;
    scoreFile << "," << player.GetStatisticCurrentValue(StatisticType::Country);
    scoreFile << "," << player.GetStatisticCurrentValue(StatisticType::Buildings);
    scoreFile << "," << player.GetStatisticCurrentValue(StatisticType::Military);
    scoreFile << "," << player.GetStatisticCurrentValue(StatisticType::Gold);
    scoreFile << "," << player.GetStatisticCurrentValue(StatisticType::Productivity);
    scoreFile << "," << player.GetStatisticCurrentValue(StatisticType::Vanquished);
    scoreFile << std::endl;

    otherFile << gf;
    otherFile << "," << bldPlanner->GetNumMilitaryBlds();
    otherFile << "," << GetAvailableResources(AISurfaceResource::Wood);
    otherFile << "," << GetAvailableResources(AISurfaceResource::Stones);
    otherFile << "," << buildingRegister.CalcBoardsDemand();
    otherFile << "," << averageBuildTime;
    for(const Job job : SOLDIER_JOBS)
        otherFile << "," << playerInventory[job];
    otherFile << std::endl;
    lastStatsFrame_ = gf;

    auto bldMap = GetBuildingsMap(*bldPlanner);
    auto sitesMap = GetBuildingsSiteMap(*bldPlanner);
    auto wantedMap = GetBuildingsWantedMap(*bldPlanner);

    buildingCountFile << gf;
    for(BuildingType type : helpers::EnumRange<BuildingType>{})
    {
        int count = bldMap[type];
        buildingCountFile << "," << count;
    }
    buildingCountFile << std::endl;
    buildingCountFile.close();

    buildingSitesFile << gf;
    for(BuildingType type : helpers::EnumRange<BuildingType>{})
    {
        int count = sitesMap[type];
        buildingSitesFile << "," << count;
    }
    buildingSitesFile << std::endl;
    buildingSitesFile.close();

    productivityFile << gf;
    for(BuildingType type : helpers::EnumRange<BuildingType>{})
    {
        productivityFile << "," << GetProductivity(type);
    }
    productivityFile << std::endl;
    productivityFile.close();

    goodsFile << gf;
    for(GoodType type : helpers::EnumRange<GoodType>{})
    {
        goodsFile << "," << AmountInStorage(type);
    }
    goodsFile << std::endl;
    goodsFile.close();

    jobsFile << gf;
    for(Job job : helpers::EnumRange<Job>{})
    {
        jobsFile << "," << AmountInStorage(job);
    }
    jobsFile << std::endl;
    jobsFile.close();

    if(gf % 2500 != 0)
    {
        return;
    }
    boost::format fmt("%s/ai_test_%010d.txt");
    fmt % STATS_CONFIG.statsPath % gf;
    std::ofstream outfile(fmt.str(), std::ios::app);
    if(!outfile)
    {
        std::cerr << "Unable to open file for appending!" << std::endl;
        return;
    }

    outfile << CurrentTimestamp() << std::endl;
    outfile << player.name << std::endl;

    outfile << " Score: ";
    outfile << " Cou: " << player.GetStatisticCurrentValue(StatisticType::Country);
    outfile << " Bui: " << player.GetStatisticCurrentValue(StatisticType::Buildings);
    outfile << " Mer: " << player.GetStatisticCurrentValue(StatisticType::Merchandise);
    outfile << " Mil: " << player.GetStatisticCurrentValue(StatisticType::Military);
    outfile << " Gol: " << player.GetStatisticCurrentValue(StatisticType::Gold);
    outfile << " Pro: " << player.GetStatisticCurrentValue(StatisticType::Productivity);
    outfile << std::endl;

    outfile << " Resources: ";
    outfile << " Wood: " << GetAvailableResources(AISurfaceResource::Wood);
    outfile << " Stone: " << GetAvailableResources(AISurfaceResource::Stones);

    outfile << " Tools: " << std::endl;
    for(Tool tool : helpers::EnumRange<Tool>{})
    {
        outfile << TOOL_NAMES_1.at(tool) << ":" << player.GetToolPriority(tool) << std::endl;
        ;
    }
    outfile << std::endl;

    outfile << " Goods: " << std::endl;
    for(GoodType type : helpers::EnumRange<GoodType>{})
    {
        outfile << GOOD_NAMES_1.at(type) << ":" << AmountInStorage(type) << std::endl;
    }
    outfile << std::endl;

    outfile << " Buildings: ";
    outfile << std::endl;

    for(BuildingType type : helpers::EnumRange<BuildingType>{})
    {
        outfile << BUILDING_NAMES_1.at(type) << "\t";
        outfile << " Count:" << bldMap[type] - sitesMap[type];
        outfile << " Sites:" << sitesMap[type];
        outfile << " Wanted:" << wantedMap[type];
        outfile << " Productivity: " << GetProductivity(type);
        outfile << std::endl;
    }
    outfile << std::endl;

    outfile << " Jobs: " << std::endl;
    for(Job job : helpers::EnumRange<Job>{})
    {
        outfile << JOB_NAMES_1.at(job) << ":" << AmountInStorage(job) << std::endl;
    }

    outfile << std::endl;

    outfile << " Other: " << std::endl;
    outfile << "Mil. Builngs: " << bldPlanner->GetNumMilitaryBlds() << std::endl;
    outfile << "Wood available: " << GetAvailableResources(AISurfaceResource::Wood) << std::endl;
    outfile << "Stone available: " << GetAvailableResources(AISurfaceResource::Stones) << std::endl;
    outfile << "Boards demand: " << buildingRegister.CalcBoardsDemand() << std::endl;
    outfile << "Avg build time: " << averageBuildTime << std::endl;
    outfile << "Soldiers by rank:";
    for(const Job job : SOLDIER_JOBS)
        outfile << " " << JOB_NAMES_1.at(job) << ": " << playerInventory[job];
    outfile << std::endl;
    outfile.close();
}

} // namespace AIJH
