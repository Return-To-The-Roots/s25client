// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AIStatsReporter.h"

#include "ai/aijh/runtime/AIPlayerJH.h"

#include "BasePlayerInfo.h"
#include "ai/aijh/planning/BuildingPlanner.h"
#include "BuildingRegister.h"
#include "CombatLossTracker.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "ai/aijh/debug/StatsConfig.h"
#include "ai/aijh/planning/Jobs.h"
#include "dataextractor/DataExtractor.h"
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
#include "gameTypes/VisualSettings.h"
#include "s25util/colors.h"

#include <boost/filesystem/path.hpp>
#include <boost/format.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <utility>

namespace fs = std::filesystem;

namespace {

bool statsCsvHeadersInitialized = false;
bool statsSnapshotHeaderWritten = false;

std::ofstream createCsvFile(const std::string& name)
{
    boost::format fmt("%s/%s.csv");
    fmt % STATS_CONFIG.statsPath % name;
    std::ofstream file(fmt.str(), std::ios::app);

    if(!file)
        std::cerr << "Unable open " << name << " buildingFile for appending!" << std::endl;
    return file;
}

void InitializeStatsCsvFiles()
{
    if(statsCsvHeadersInitialized)
        return;

    std::ofstream buildingCountFile = createCsvFile("buildings_count");
    std::ofstream buildingSitesFile = createCsvFile("buildings_sites");
    std::ofstream productivityFile = createCsvFile("productivity");
    std::ofstream otherFile = createCsvFile("other");

    if(buildingCountFile)
    {
        buildingCountFile << "GameFrame";
        for(BuildingType type : helpers::EnumRange<BuildingType>{})
            buildingCountFile << "," << BUILDING_NAMES_1.at(type) << "Count";
        buildingCountFile << std::endl;
    }

    if(buildingSitesFile)
    {
        buildingSitesFile << "GameFrame";
        for(BuildingType type : helpers::EnumRange<BuildingType>{})
            buildingSitesFile << "," << BUILDING_NAMES_1.at(type) << "Sites";
        buildingSitesFile << std::endl;
    }

    if(productivityFile)
    {
        productivityFile << "GameFrame";
        for(BuildingType type : helpers::EnumRange<BuildingType>{})
            productivityFile << "," << BUILDING_NAMES_1.at(type) << "Prod";
        productivityFile << std::endl;
    }

    if(otherFile)
    {
        otherFile << "GameFrame,MilBld,WoodAvailable,StoneAvailable,BoardsDemand,AverageBuildTime";
        for(const Job job : SOLDIER_JOBS)
            otherFile << "," << JOB_NAMES_1.at(job);
        otherFile << std::endl;
    }

    statsCsvHeadersInitialized = true;
}

std::map<BuildingType, int> GetBuildingsMap(const AIJH::BuildingPlanner& bldPlanner)
{
    std::map<BuildingType, int> bldMap;
    for(BuildingType type : helpers::EnumRange<BuildingType>{})
        bldMap[type] = bldPlanner.GetNumBuildings(type);
    return bldMap;
}

std::map<BuildingType, int> GetBuildingsSiteMap(const AIJH::BuildingPlanner& bldPlanner)
{
    std::map<BuildingType, int> bldMap;
    for(BuildingType type : helpers::EnumRange<BuildingType>{})
        bldMap[type] = bldPlanner.GetNumBuildingSites(type);
    return bldMap;
}

std::map<BuildingType, int> GetBuildingsWantedMap(const AIJH::BuildingPlanner& bldPlanner)
{
    std::map<BuildingType, int> bldMap;
    for(BuildingType type : helpers::EnumRange<BuildingType>{})
        bldMap[type] = bldPlanner.GetNumAdditionalBuildingsWanted(type);
    return bldMap;
}

std::string CurrentTimestamp()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    const std::tm* now_tm = std::localtime(&now_time_t);
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

AIStatsReporter::AIStatsReporter(AIPlayerJH& owner) : owner_(owner) {}

void AIStatsReporter::TrackCombatStart(const nobBaseMilitary& target)
{
    const unsigned startGf = owner_.gwb.GetEvMgr().GetCurrentGF();
    const unsigned objId = target.GetObjId();
    const auto it = std::find_if(activeCombats_.begin(), activeCombats_.end(),
                                 [objId](const ActiveCombat& combat) { return combat.targetObjId == objId; });
    if(it != activeCombats_.end())
        return;

    double captureRisk = 0.0;
    if(const auto* mil = dynamic_cast<const nobMilitary*>(&target))
        captureRisk = mil->GetCaptureRiskEstimate();
    CombatLossTracker::RegisterCombat(objId, captureRisk);
    activeCombats_.push_back({target.GetPos(), objId, target.GetPlayer(), target.GetBuildingType(), startGf, false});
}

std::string AIStatsReporter::GetCombatsLogPath() const
{
    const fs::path filePath = fs::path(STATS_CONFIG.statsPath) / "combats.txt";
    return filePath.string();
}

std::string AIStatsReporter::FormatPlayerLabel(unsigned playerIdx) const
{
    const GamePlayer& pl = owner_.gwb.GetPlayer(playerIdx);
    const unsigned color = pl.color;
    const int colorIdx = BasePlayerInfo::GetColorIdx(color);
    static constexpr std::array<const char*, PLAYER_COLORS.size()> colorNames = {
      "Blue", "Yellow", "Red", "Magenta", "Black", "Green", "Orange", "Cyan", "White", "Brown", "Purple"};
    const std::string colorName =
      (colorIdx >= 0 && static_cast<size_t>(colorIdx) < colorNames.size()) ? colorNames[colorIdx] : "Unknown";
    const std::string nationName = _(NationNames[pl.nation]);
    std::ostringstream ss;
    ss << "Player " << (playerIdx + 1) << " - " << colorName << " " << nationName;
    return ss.str();
}

void AIStatsReporter::LogPlayerMetadata(std::ofstream& combatsFile) const
{
    bool first = true;
    for(unsigned playerIdx = 0; playerIdx < owner_.gwb.GetNumPlayers(); ++playerIdx)
    {
        const GamePlayer& player = owner_.gwb.GetPlayer(playerIdx);
        if(player.ps != PlayerState::Occupied)
            continue;
        if(!first)
            combatsFile << ", ";
        combatsFile << FormatPlayerLabel(playerIdx);
        first = false;
    }
    combatsFile << std::endl;
}

void AIStatsReporter::InitializeCombatsLogFile() const
{
    if(combatsLogInitialized_)
        return;

    std::ofstream combatsFile(GetCombatsLogPath(), std::ios::trunc);
    if(!combatsFile)
    {
        std::cerr << "Unable to open combats log file for writing!" << std::endl;
        return;
    }

    LogPlayerMetadata(combatsFile);
    combatsLogInitialized_ = true;
}

bool AIStatsReporter::HasOwnAggressors(const nobBaseMilitary& building) const
{
    for(const nofAttacker* attacker : building.GetAggressors())
    {
        if(attacker && attacker->GetPlayer() == owner_.playerId)
            return true;
    }
    return false;
}

AIStatsReporter::CombatLogState AIStatsReporter::EvaluateCombatState(ActiveCombat& combat, const unsigned gf) const
{
    static constexpr unsigned kCombatGraceGf = 500;
    static constexpr unsigned kCombatStaleGf = 2500;

    const noBase* nodeObj = owner_.gwb.GetNO(combat.pos);
    const auto* building = dynamic_cast<const nobBaseMilitary*>(nodeObj);
    if(!building)
        return CombatLogState::Success;

    if(building->GetObjId() != combat.targetObjId)
        return CombatLogState::Success;

    if(HasOwnAggressors(*building))
    {
        combat.sawAggressor = true;
        return CombatLogState::Pending;
    }

    if(!combat.sawAggressor)
    {
        const unsigned age = gf - combat.startGf;
        if(age < kCombatGraceGf)
            return CombatLogState::Pending;
        if(age >= kCombatStaleGf)
            return CombatLogState::Failure;
        return CombatLogState::Pending;
    }

    const unsigned char owner = building->GetPlayer();
    if(owner == owner_.playerId)
        return CombatLogState::Success;
    if(owner != combat.defenderPlayer)
        return CombatLogState::Success;
    return CombatLogState::Failure;
}

void AIStatsReporter::LogFinishedCombats(const unsigned gf) const
{
    if(activeCombats_.empty())
        return;

    std::vector<std::pair<ActiveCombat, bool>> finishedCombats;
    finishedCombats.reserve(activeCombats_.size());
    auto it = activeCombats_.begin();
    while(it != activeCombats_.end())
    {
        const CombatLogState state = EvaluateCombatState(*it, gf);
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

    InitializeCombatsLogFile();
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
        std::ostringstream riskStream;
        riskStream << std::fixed << std::setprecision(2) << stats.captureRisk;
        combatsFile << "#" << gf << " Player #" << static_cast<unsigned>(owner_.playerId + 1) << " attacks Player #"
                    << static_cast<unsigned>(combat.defenderPlayer + 1) << " "
                    << BUILDING_NAMES_1.at(combat.buildingType) << " " << combat.targetObjId << ". Attack #"
                    << (success ? "succed" : "failed") << " Forces: Attacker "
                    << FormatRankCounts(stats.attackerForces) << " . Defender " << FormatRankCounts(stats.defenderForces)
                    << " Losses: Attacker " << FormatRankCounts(stats.attackerLosses) << " . Defender "
                    << FormatRankCounts(stats.defenderLosses) << " CaptureRisk=" << riskStream.str()
                    << " StartGF=" << combat.startGf << " EndGF=" << gf;
        if(success)
            combatsFile << " Destroyed: " << FormatDestroyedBuildings(stats.destroyedBuildings);
        combatsFile << std::endl;
    }
}

void AIStatsReporter::SaveStats(unsigned gf) const
{
    if(owner_.player.ps == PlayerState::Locked)
        return;

    std::ofstream statsFile = createCsvFile("stats");
    if(!statsFile)
        return;

    DataExtractor extractor;
    extractor.ProcessSnapshot(owner_.player, gf, &owner_);
    if(const SnapshotData* snapshot = extractor.GetCurrentSnapshot())
    {
        const bool writeHeader = !statsSnapshotHeaderWritten;
        extractor.SerializeCsv(*snapshot, statsFile, writeHeader);
        if(writeHeader)
            statsSnapshotHeaderWritten = true;
        extractor.ClearSnapshot();
    }
}

void AIStatsReporter::SaveDebugStats(unsigned gf) const
{
    if(owner_.player.ps == PlayerState::Locked)
        return;

    InitializeStatsCsvFiles();

    std::ofstream buildingCountFile = createCsvFile("buildings_count");
    std::ofstream buildingSitesFile = createCsvFile("buildings_sites");
    std::ofstream productivityFile = createCsvFile("productivity");
    std::ofstream otherFile = createCsvFile("other");

    const unsigned previousStatsFrame = lastStatsFrame_;
    const BuildingRegister& buildingRegister = owner_.player.GetBuildingRegister();
    const Inventory& playerInventory = owner_.player.GetInventory();
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
        {
            for(const nobUsual* bld : buildingRegister.GetBuildings(type))
                accumulateBuild(*bld);
        }
    }
    for(const nobMilitary* bld : buildingRegister.GetMilitaryBuildings())
        accumulateBuild(*bld);
    for(const nobHarborBuilding* bld : buildingRegister.GetHarbors())
        accumulateBuild(*bld);
    for(const nobBaseWarehouse* bld : buildingRegister.GetStorehouses())
        accumulateBuild(*bld);

    const double averageBuildTime =
      completedBuildings != 0 ? static_cast<double>(totalBuildTime) / static_cast<double>(completedBuildings) : 0.0;
    const BuildingPlanner& bldPlanner = owner_.GetBldPlanner();
    otherFile << gf;
    otherFile << "," << bldPlanner.GetNumMilitaryBlds();
    otherFile << "," << owner_.GetAvailableResources(AISurfaceResource::Wood);
    otherFile << "," << owner_.GetAvailableResources(AISurfaceResource::Stones);
    otherFile << "," << buildingRegister.CalcBoardsDemand();
    otherFile << "," << averageBuildTime;
    for(const Job job : SOLDIER_JOBS)
        otherFile << "," << playerInventory[job];
    otherFile << std::endl;
    lastStatsFrame_ = gf;

    const auto bldMap = GetBuildingsMap(bldPlanner);
    const auto sitesMap = GetBuildingsSiteMap(bldPlanner);
    const auto wantedMap = GetBuildingsWantedMap(bldPlanner);

    buildingCountFile << gf;
    for(BuildingType type : helpers::EnumRange<BuildingType>{})
        buildingCountFile << "," << bldMap.at(type);
    buildingCountFile << std::endl;

    buildingSitesFile << gf;
    for(BuildingType type : helpers::EnumRange<BuildingType>{})
        buildingSitesFile << "," << sitesMap.at(type);
    buildingSitesFile << std::endl;

    productivityFile << gf;
    for(BuildingType type : helpers::EnumRange<BuildingType>{})
        productivityFile << "," << owner_.GetProductivity(type);
    productivityFile << std::endl;

    if(gf % 2500 != 0)
        return;

    boost::format fmt("%s/ai_test_%010d.txt");
    fmt % STATS_CONFIG.statsPath % gf;
    std::ofstream outfile(fmt.str(), std::ios::app);
    if(!outfile)
    {
        std::cerr << "Unable to open file for appending!" << std::endl;
        return;
    }

    outfile << CurrentTimestamp() << std::endl;
    outfile << owner_.player.name << std::endl;

    outfile << " Score: ";
    outfile << " Cou: " << owner_.player.GetStatisticCurrentValue(StatisticType::Country);
    outfile << " Bui: " << owner_.player.GetStatisticCurrentValue(StatisticType::Buildings);
    outfile << " Mer: " << owner_.player.GetStatisticCurrentValue(StatisticType::Merchandise);
    outfile << " Mil: " << owner_.player.GetStatisticCurrentValue(StatisticType::Military);
    outfile << " Gol: " << owner_.player.GetStatisticCurrentValue(StatisticType::Gold);
    outfile << " Pro: " << owner_.player.GetStatisticCurrentValue(StatisticType::Productivity);
    outfile << std::endl;

    outfile << " Resources: ";
    outfile << " Wood: " << owner_.GetAvailableResources(AISurfaceResource::Wood);
    outfile << " Stone: " << owner_.GetAvailableResources(AISurfaceResource::Stones);

    outfile << " Tools: " << std::endl;
    for(Tool tool : helpers::EnumRange<Tool>{})
        outfile << TOOL_NAMES_1.at(tool) << ":" << owner_.player.GetToolPriority(tool) << std::endl;
    outfile << std::endl;

    VisualSettings visual_settings{};
    owner_.player.FillVisualSettings(visual_settings);
    outfile << " Goods Distribution: " << std::endl;
    GoodType current_good = GoodType::Nothing;
    for(std::size_t idx = 0; idx < distributionMap.size(); ++idx)
    {
        const auto& mapping = distributionMap[idx];
        const GoodType good = std::get<0>(mapping);
        const BuildingType building = std::get<1>(mapping);
        const unsigned priority = visual_settings.distribution[idx];
        if(good != current_good)
        {
            if(idx != 0)
                outfile << std::endl;
            outfile << GOOD_NAMES_1.at(good) << ": ";
            current_good = good;
        }
        else
            outfile << ",";
        outfile << BUILDING_NAMES_1.at(building) << " " << priority;
    }
    outfile << std::endl << std::endl;

    outfile << " Goods: " << std::endl;
    for(GoodType type : helpers::EnumRange<GoodType>{})
    {
        outfile << GOOD_NAMES_1.at(type) << " Stocks:" << owner_.AmountInStorage(type)
                << ",Produced:" << owner_.GetProducedGoods()[type] << std::endl;
    }
    outfile << std::endl;

    outfile << " Buildings: " << std::endl;
    for(BuildingType type : helpers::EnumRange<BuildingType>{})
    {
        outfile << BUILDING_NAMES_1.at(type) << "\t";
        outfile << " Count:" << bldMap.at(type) - sitesMap.at(type);
        outfile << " Sites:" << sitesMap.at(type);
        outfile << " Wanted:" << wantedMap.at(type);
        outfile << " Productivity: " << owner_.GetProductivity(type);
        outfile << std::endl;
    }
    outfile << std::endl;

    outfile << " Jobs: " << std::endl;
    for(Job job : helpers::EnumRange<Job>{})
        outfile << JOB_NAMES_1.at(job) << ":" << owner_.AmountInStorage(job) << std::endl;

    outfile << std::endl;

    outfile << " Other: " << std::endl;
    outfile << "Mil. Builngs: " << bldPlanner.GetNumMilitaryBlds() << std::endl;
    outfile << "Wood available: " << owner_.GetAvailableResources(AISurfaceResource::Wood) << std::endl;
    outfile << "Stone available: " << owner_.GetAvailableResources(AISurfaceResource::Stones) << std::endl;
    outfile << "Boards demand: " << buildingRegister.CalcBoardsDemand() << std::endl;
    outfile << "Avg build time: " << averageBuildTime << std::endl;
    outfile << "Soldiers by rank:";
    for(const Job job : SOLDIER_JOBS)
        outfile << " " << JOB_NAMES_1.at(job) << ": " << playerInventory[job];
    outfile << std::endl;
}

} // namespace AIJH
