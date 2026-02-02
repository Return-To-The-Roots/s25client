#include "dataextractor/DataExtractor.h"

#include "GamePlayer.h"
#include "ai/aijh/AIPlayerJH.h"
#include "helpers/EnumRange.h"

#include "gameTypes/BuildingType.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"
#include "gameTypes/StatisticTypes.h"
#include "gameTypes/VisualSettings.h"
#include <boost/filesystem/directory.hpp> 
#include <boost/filesystem/operations.hpp> 

#include <fstream> 
#include <iostream>
#include <vector>
#include <unordered_map>
#include <set>
#include <string> 
#include <optional> // Ensure it's included, though DataExtractor.h already does

#include <nlohmann/json.hpp> // For JSON output

namespace fs = boost::filesystem; 

void DataExtractor::ProcessSnapshot(const GamePlayer& player, uint32_t gameframe, const AIPlayer* aiPlayer)
{
    // Create a snapshot object that will hold all data for this gameframe
    SnapshotData snapshot_data_map; // Renamed to avoid conflict with a type if any

    // Add metadata fields
    snapshot_data_map["GameFrame"] = gameframe;
    snapshot_data_map["PlayerId"] = player.GetPlayerId();

    // Process statistics
    for (StatisticType type : helpers::EnumRange<StatisticType>{})
    {
        uint32_t value = player.GetStatisticCurrentValue(type);
        snapshot_data_map[StatisticTypeName(type)] = value;
    }

    // Process buildings
    const auto& building_nums = player.GetBuildingRegister().GetBuildingNums();
    for (BuildingType type : helpers::EnumRange<BuildingType>{})
    {
        std::string buildingName = BUILDING_NAMES_1.at(type);
        snapshot_data_map[buildingName + "Count"] = building_nums.buildings[type];
        snapshot_data_map[buildingName + "Sites"] = building_nums.buildingSites[type];
        // Add production data
        snapshot_data_map[buildingName + "Prod"] = player.GetBuildingRegister().CalcAverageProductivity(type);
    }

    // Process merchandise (inventory)
    Inventory inventory = player.GetInventory();
    const helpers::EnumArray<unsigned, GoodType>* producedGoods = nullptr;
    if(aiPlayer)
    {
        const auto* aiJH = dynamic_cast<const AIJH::AIPlayerJH*>(aiPlayer);
        if(aiJH)
            producedGoods = &aiJH->GetProducedGoods();
    }
    for (GoodType type : helpers::EnumRange<GoodType>{})
    {
        std::string goodName = GOOD_NAMES_1.at(type);
        snapshot_data_map[goodName] = inventory.goods[type];
        if(producedGoods)
            snapshot_data_map[goodName + "Produced"] = (*producedGoods)[type];
    }
    for(Job job : helpers::EnumRange<Job>{})
        snapshot_data_map["Job" + JOB_NAMES_1.at(job)] = inventory.people[job];
    for(Tool tool : helpers::EnumRange<Tool>{})
        snapshot_data_map["ToolPriority" + TOOL_NAMES_1.at(tool)] = player.GetToolPriority(tool);
    VisualSettings visualSettings{};
    player.FillVisualSettings(visualSettings);
    std::size_t distIdx = 0;
    for(const auto& mapping : distributionMap)
    {
        const GoodType good = std::get<0>(mapping);
        const BuildingType building = std::get<1>(mapping);
        snapshot_data_map["Distr" + GOOD_NAMES_1.at(good) + "To" + BUILDING_NAMES_1.at(building)] =
          visualSettings.distribution[distIdx++];
    }

    std::cerr << "Successfully processed gameframe " << gameframe << std::endl; 

    currentSnapshot_ = snapshot_data_map; // Assign to the single optional snapshot

    // Keep track of all field names for headers
    fieldNames_.clear(); // Clear previous field names if any (for potential reuse)
    for (const auto& pair : snapshot_data_map)
    {
        fieldNames_.insert(pair.first);
    }
}

const SnapshotData* DataExtractor::GetCurrentSnapshot() const
{
    return currentSnapshot_ ? &*currentSnapshot_ : nullptr;
}

void DataExtractor::ClearSnapshot()
{
    currentSnapshot_.reset();
    fieldNames_.clear();
}

void DataExtractor::SerializeCsv(const SnapshotData& snapshot, std::ostream& output, bool write_header) const
{
    // Prepare headers (GameFrame first, then others sorted)
    std::vector<std::string> headers;
    headers.reserve(fieldNames_.size());
    if (fieldNames_.count("GameFrame"))
        headers.push_back("GameFrame");
    if (fieldNames_.count("PlayerId"))
        headers.push_back("PlayerId");
    for (const auto& name : fieldNames_) { // std::set iterates in sorted order
        if (name != "GameFrame" && name != "PlayerId") {
            headers.push_back(name);
        }
    }

    if(write_header)
    {
        // Write header row
        bool firstHeader = true;
        for (const auto& header : headers) {
            if (!firstHeader) output << ",";
            firstHeader = false;

            bool needsQuotes = header.find_first_of(",\"\n") != std::string::npos;
            if (needsQuotes) {
                output << '"';
                for (char c : header) {
                    if (c == '"') output << "\"\""; 
                    else output << c;
                }
                output << '"';
            } else {
                output << header;
            }
        }
        output << '\n';
    }

    // Write data row
    bool firstCell = true;
    for (const auto& header : headers) {
        if (!firstCell) output << ",";
        firstCell = false;

        auto it = snapshot.find(header);
        if (it != snapshot.end()) {
            output << it->second;
        } else {
            output << '0'; // Default value for missing fields
        }
    }
    output << '\n';

}

void DataExtractor::SerializeJson(const SnapshotData& snapshot, std::ostream& output) const
{
    nlohmann::json j_object = snapshot;
    output << j_object.dump() << '\n';
}
