#include "DataExtractor.h"

#include "GamePlayer.h"
#include "helpers/EnumRange.h"

#include "gameTypes/BuildingType.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/StatisticTypes.h"
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

#define CSV_IO_NO_THREAD
#include "csv.h" 

namespace fs = boost::filesystem; 

void DataExtractor::ProcessSnapshot(GamePlayer& player, uint32_t gameframe)
{
    // Create a snapshot object that will hold all data for this gameframe
    SnapshotData snapshot_data_map; // Renamed to avoid conflict with a type if any

    // Add gameframe info
    snapshot_data_map["GameFrame"] = gameframe;

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
    for (GoodType type : helpers::EnumRange<GoodType>{})
    {
        std::string goodName = GOOD_NAMES_1.at(type);
        snapshot_data_map[goodName] = inventory.goods[type];
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

void DataExtractor::flush(OutputFormat format)
{
    // Skip if no snapshot data to write
    if (!currentSnapshot_) {
        std::cerr << "No snapshot data to flush." << std::endl;
        return;
    }

    const SnapshotData& snapshot_to_write = *currentSnapshot_;

    try {
        if (format == OutputFormat::CSV) {
            // Prepare headers (GameFrame first, then others sorted)
            std::vector<std::string> headers;
            headers.reserve(fieldNames_.size());
            if (fieldNames_.count("GameFrame")) {
                headers.push_back("GameFrame");
            }
            for (const auto& name : fieldNames_) { // std::set iterates in sorted order
                if (name != "GameFrame") {
                    headers.push_back(name);
                }
            }

            // Always write headers to std::cout
            bool firstHeader = true;
            for (const auto& header : headers) {
                if (!firstHeader) std::cout << ",";
                firstHeader = false;

                bool needsQuotes = header.find_first_of(",\"\n") != std::string::npos;
                if (needsQuotes) {
                    std::cout << "\"";
                    for (char c : header) {
                        if (c == '"') std::cout << "\"\""; 
                        else std::cout << c;
                    }
                    std::cout << "\"";
                } else {
                    std::cout << header;
                }
            }
            std::cout << "\n";

            // Write data row to std::cout
            bool firstCell = true;
            for (const auto& header : headers) {
                if (!firstCell) std::cout << ",";
                firstCell = false;

                auto it = snapshot_to_write.find(header);
                if (it != snapshot_to_write.end()) {
                    std::cout << it->second;
                } else {
                    std::cout << "0"; // Default value for missing fields
                }
            }
            std::cout << "\n";
            std::cerr << "Successfully wrote 1 snapshot as CSV to stdout." << std::endl;

        } else if (format == OutputFormat::JSON) {
            // Directly convert the map (SnapshotData) to a nlohmann::json object
            nlohmann::json j_object = snapshot_to_write; 
            
            // Output JSON object to std::cout
            std::cout << j_object.dump() << std::endl; 
            std::cerr << "Successfully wrote 1 snapshot as JSON object to stdout." << std::endl;
        }

        currentSnapshot_.reset(); // Clear the stored snapshot
        fieldNames_.clear();      // Clear field names for potential reuse
    }
    catch (const std::exception& e) {
        std::cerr << "Error writing data to stdout: " << e.what() << std::endl;
    }
}
