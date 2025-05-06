//
// Created by pavel on 01.05.25.
//

#include "DataMiner.h"
#include "Game.h"
#include "GamePlayer.h"
#include "PlayerInfo.h"
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

