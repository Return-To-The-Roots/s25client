// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AIStatsReporter.h"

#include "ai/aijh/planning/BuildingPlanner.h"
#include "BuildingRegister.h"
#include "GamePlayer.h"
#include "ai/AIResource.h"
#include "ai/aijh/debug/StatsConfig.h"
#include "ai/aijh/planning/Jobs.h"
#include "dataextractor/DataExtractor.h"
#include "buildings/noBaseBuilding.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobUsual.h"
#include "helpers/EnumRange.h"
#include "gameData/BuildingConsts.h"
#include "gameData/BuildingProperties.h"
#include "gameData/GameConsts.h"
#include "gameData/JobConsts.h"
#include "gameData/MilitaryConsts.h"
#include "gameData/ToolConsts.h"
#include "gameTypes/VisualSettings.h"
#include <boost/format.hpp>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <utility>

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

} // namespace

namespace AIJH {

AIStatsReporter::AIStatsReporter(const AIStatsSource& owner) : owner_(owner) {}

void AIStatsReporter::TrackCombatStart(const nobBaseMilitary& /*target*/) {}

void AIStatsReporter::InitializeCombatsLogFile() const {}

void AIStatsReporter::LogFinishedCombats(const unsigned /*gf*/) const {}

void AIStatsReporter::SaveStats(unsigned gf) const
{
    const GamePlayer& player = owner_.GetPlayer();
    if(player.ps == PlayerState::Locked)
        return;

    std::ofstream statsFile = createCsvFile("stats");
    if(!statsFile)
        return;

    DataExtractor extractor;
    extractor.ProcessSnapshot(player, gf, &owner_);
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
    const GamePlayer& player = owner_.GetPlayer();
    if(player.ps == PlayerState::Locked)
        return;

    InitializeStatsCsvFiles();

    std::ofstream buildingCountFile = createCsvFile("buildings_count");
    std::ofstream buildingSitesFile = createCsvFile("buildings_sites");
    std::ofstream productivityFile = createCsvFile("productivity");
    std::ofstream otherFile = createCsvFile("other");

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
    outfile << " Wood: " << owner_.GetAvailableResources(AISurfaceResource::Wood);
    outfile << " Stone: " << owner_.GetAvailableResources(AISurfaceResource::Stones);

    outfile << " Tools: " << std::endl;
    for(Tool tool : helpers::EnumRange<Tool>{})
        outfile << TOOL_NAMES_1.at(tool) << ":" << player.GetToolPriority(tool) << std::endl;
    outfile << std::endl;

    VisualSettings visual_settings{};
    player.FillVisualSettings(visual_settings);
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
