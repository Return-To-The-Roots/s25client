//
// Created by pavel on 01.05.25.
//

#include "DataMiner.h"
#include "Game.h"
#include "GamePlayer.h"
#include "PlayerInfo.h"d
#include "ClickhouseWriter.h"
#include "ai/aijh/StatsConfig.h"
#include "helpers/format.hpp"
#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/writer.h>

#include <fstream>

namespace fs = std::filesystem;
namespace {
std::ofstream SUPPRESS_UNUSED createCsvFile(std::string name)
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
}

DataMiner::DataMiner(unsigned gf, Game& game): gf_(gf), game_(game), world_(game.world_)
{
}

void DataMiner::Run()
{
    for (unsigned i = 0; i < world_.GetNumPlayers(); ++i) {
        GamePlayer& player = world_.GetPlayer(i);
        if(player.isUsed())
        {
            mineStats(player);
            mineBuildings(player);
        }
    }
}

void DataMiner::WriteToClickhouse(GamePlayer& player) {
    // Initialize ClickHouse writer
    // Replace these with your actual ClickHouse connection details
    ClickhouseWriter writer("localhost", 9000, "game_ai", "raw_metrics");

    // Collect statistics data
    std::vector<std::pair<std::string, uint32_t>> statistics;
    for (StatisticType type : helpers::EnumRange<StatisticType>{}) {
        uint32_t value = player.GetStatisticCurrentValue(type);
        statistics.emplace_back(StatisticTypeName(type), value);
    }

    // Collect buildings data
    std::vector<std::tuple<std::string, uint16_t, uint16_t>> buildings;
    const auto& building_nums = player.GetBuildingRegister().GetBuildingNums();
    for (BuildingType type : helpers::EnumRange<BuildingType>{}) {
        unsigned count = building_nums.buildings[type];
        unsigned sites = building_nums.buildings[type];
        buildings.emplace_back(BUILDING_NAMES_1.at(type), count, sites);
    }

    // Collect merchandise data
    std::map<std::string, uint32_t> merchandise;
    Inventory inventory = player.GetInventory();
    for (GoodType type : helpers::EnumRange<GoodType>{}) {
        unsigned count = inventory.goods[type];
        merchandise[GOOD_NAMES_1.at(type)] = count;
    }

    // Write data to ClickHouse
    writer.WriteToClickhouse(run_id_, gf_, statistics, buildings, merchandise);
}

void DataMiner::mineBuildings(GamePlayer& player)
{
    std::cout << "GameFrame";
    for(BuildingType type : helpers::EnumRange<BuildingType>{})
    {
        std::cout << "," << BUILDING_NAMES_1.at(type) << "Count";
    }
    std::cout << std::endl;
    std::cout << gf_;
    for(BuildingType type : helpers::EnumRange<BuildingType>{})
    {
        int count = player.GetBuildingRegister().GetBuildingNums().buildings[type];
        std::cout << "," << count;
    }
    std::cout << std::endl;
}

void DataMiner::mineBuildingsSites(GamePlayer& player)
{
    std::cout << "GameFrame";
    for(BuildingType type : helpers::EnumRange<BuildingType>{})
    {
        std::cout << "," << BUILDING_NAMES_1.at(type) << "Sites";
    }
    std::cout << std::endl;
    std::cout << gf_;
    for(BuildingType type : helpers::EnumRange<BuildingType>{})
    {
        int count = player.GetBuildingRegister().GetBuildingNums().buildingSites[type];
        std::cout << "," << count;
    }
    std::cout << std::endl;
}

void DataMiner::mineStats(GamePlayer& player)
{
        std::cout << "GameFrame,Country,Buildings,Military,GoldCoins,Productivity,Kills" << std::endl;
        std::cout << gf_;
        for(StatisticType type : helpers::EnumRange<StatisticType>{})
        {
            int count =  player.GetStatisticCurrentValue(type);
            std::cout << "," << count;
        }
}

