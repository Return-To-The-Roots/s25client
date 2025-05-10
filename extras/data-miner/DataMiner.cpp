#include "DataMiner.h"

#include "GamePlayer.h"
#include "helpers/EnumRange.h"

#include "gameTypes/BuildingType.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/StatisticTypes.h"
#include <boost/filesystem/directory.hpp>

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>

using json = nlohmann::json;
namespace fs = boost::filesystem;

void DataMiner::ProcessSnapshot(GamePlayer& player, uint32_t gameframe)
{
    json snapshot;

    snapshot["run_id"] = run_id_;
    snapshot["gameframe"] = gameframe;

    // Macro statistics
    for (StatisticType type : helpers::EnumRange<StatisticType>{})
    {
        uint32_t value = player.GetStatisticCurrentValue(type);
        snapshot["statistics.type"].push_back(StatisticTypeName(type));
        snapshot["statistics.value"].push_back(value);
    }

    // Buildings
    const auto& building_nums = player.GetBuildingRegister().GetBuildingNums();
    for (BuildingType type : helpers::EnumRange<BuildingType>{})
    {
        snapshot["buildings.type"].push_back(BUILDING_NAMES_1.at(type));
        snapshot["buildings.count"].push_back(building_nums.buildings[type]);  // Replace with actual count
        snapshot["buildings.sites"].push_back(building_nums.buildings[type]);  // Replace with actual sites
    }

    // Merchandise
    Inventory inventory = player.GetInventory();
    json merchandise;
    for (GoodType type : helpers::EnumRange<GoodType>{})
    {
        merchandise[GOOD_NAMES_1.at(type)] = inventory.goods[type];
    }
    snapshot["merchandise"] = merchandise;

    std::cout << "Successfully processed gameframe " << gameframe << std::endl;

    snapshots_.push_back(snapshot);
}

void DataMiner::flush(const std::string& directory)
{
    // Construct the output filename based on run_id only (no gameframe)
    std::ostringstream filename;
    filename << "stats_" << run_id_ << ".ndjson";

    // Combine directory path with filename to get full file path
    fs::path filePath = fs::path(directory) / filename.str();

    // Write all snapshots to the file
    std::ofstream out(filePath, std::ios::app); // append mode
    if (out.is_open())
    {
        for (const auto& snapshot : snapshots_)
        {
            out << snapshot.dump() << std::endl; // Write each snapshot as a JSON line
        }
    }
    else
    {
        std::cerr << "Failed to open file: " << filePath << std::endl;
    }
}
