#include "DataExtractor.h"

#include "GamePlayer.h"
#include "helpers/EnumRange.h"

#include "gameTypes/BuildingType.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/StatisticTypes.h"
#include <boost/filesystem/directory.hpp> // Not strictly needed anymore for flush
#include <boost/filesystem/operations.hpp> // Not strictly needed anymore for flush

#include <fstream> // Not strictly needed anymore for flush
#include <iostream>
#include <vector>
#include <unordered_map>
#include <set>

#define CSV_IO_NO_THREAD
#include "csv.h" // Note: csv.h seems to be included for the macro, not active use in writing.

namespace fs = boost::filesystem; // Not strictly needed anymore for flush

void DataExtractor::ProcessSnapshot(GamePlayer& player, uint32_t gameframe)
{
    // Create a snapshot object that will hold all data for this gameframe
    SnapshotData snapshot;

    // Add gameframe info
    snapshot["GameFrame"] = gameframe;

    // Process statistics
    for (StatisticType type : helpers::EnumRange<StatisticType>{})
    {
        uint32_t value = player.GetStatisticCurrentValue(type);
        snapshot[StatisticTypeName(type)] = value;
    }

    // Process buildings
    const auto& building_nums = player.GetBuildingRegister().GetBuildingNums();
    for (BuildingType type : helpers::EnumRange<BuildingType>{})
    {
        std::string buildingName = BUILDING_NAMES_1.at(type);
        snapshot[buildingName + "Count"] = building_nums.buildings[type];
        snapshot[buildingName + "Sites"] = building_nums.buildingSites[type];
        // Add production data
        snapshot[buildingName + "Prod"] = player.GetBuildingRegister().CalcAverageProductivity(type);
    }

    // Process merchandise (inventory)
    Inventory inventory = player.GetInventory();
    for (GoodType type : helpers::EnumRange<GoodType>{})
    {
        std::string goodName = GOOD_NAMES_1.at(type);
        snapshot[goodName] = inventory.goods[type];
    }

    std::cerr << "Successfully processed gameframe " << gameframe << std::endl; // Using cerr for progress messages

    snapshots_.push_back(snapshot);

    // Keep track of all field names for headers
    for (const auto& pair : snapshot)
    {
        fieldNames_.insert(pair.first);
    }
}

void DataExtractor::flush() // Changed signature
{
    // Skip if no snapshots to write
    if (snapshots_.empty()) {
        return;
    }

    try {
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

            // Handle special characters in CSV headers (quote if necessary, escape internal quotes)
            bool needsQuotes = header.find_first_of(",\"\n") != std::string::npos;
            if (needsQuotes) {
                std::cout << "\"";
                for (char c : header) {
                    if (c == '"') std::cout << "\"\""; // Double up internal quotes
                    else std::cout << c;
                }
                std::cout << "\"";
            } else {
                std::cout << header;
            }
        }
        std::cout << "\n";

        // Write data rows to std::cout
        for (const auto& snapshot : snapshots_) {
            bool firstCell = true;
            for (const auto& header : headers) {
                if (!firstCell) std::cout << ",";
                firstCell = false;

                auto it = snapshot.find(header);
                if (it != snapshot.end()) {
                    std::cout << it->second; // Data is int, no special quoting needed for int
                } else {
                    std::cout << "0"; // Default value for missing fields
                }
            }
            std::cout << "\n";
        }

        std::cerr << "Successfully wrote " << snapshots_.size() << " snapshot(s) to stdout." << std::endl; // Using cerr for status

        snapshots_.clear();
        // fieldNames_ is not cleared here. If DataExtractor instance is reused for multiple
        // ProcessSnapshot calls before a flush, fieldNames_ will accumulate.
        // For the current main.cpp logic (one snapshot per run), this is fine.
        // If DataExtractor were to process multiple files internally before one flush,
        // fieldNames_ should ideally be cleared along with snapshots_ or at the start of a new "batch".
    }
    catch (const std::exception& e) {
        std::cerr << "Error writing CSV to stdout: " << e.what() << std::endl;
    }
}
