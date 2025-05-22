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

#define CSV_IO_NO_THREAD
#include "csv.h" // Note: csv.h seems to be included for the macro, not active use in writing.

namespace fs = boost::filesystem;

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

    std::cout << "Successfully processed gameframe " << gameframe << std::endl;

    snapshots_.push_back(snapshot);

    // Keep track of all field names for headers
    for (const auto& pair : snapshot) // Changed from structured binding for clarity if needed
    {
        fieldNames_.insert(pair.first);
    }
}

void DataExtractor::flush(const std::string& filePath)
{
    // Skip if no snapshots to write
    if (snapshots_.empty()) {
        return;
    }

    fs::path outputFilePath(filePath);

    try {
        // Make sure the parent directory exists
        fs::path parentDir = outputFilePath.parent_path();
        if (!parentDir.empty() && !fs::exists(parentDir)) {
            fs::create_directories(parentDir);
        }

        bool writeHeaders = true;
        if (fs::exists(outputFilePath) && !fs::is_empty(outputFilePath)) {
            writeHeaders = false; // File exists and has content, so don't write headers
        }

        std::ofstream outFile;
        if (writeHeaders) {
            // Create/overwrite file (if empty or new) and prepare to write headers
            outFile.open(outputFilePath.string(), std::ios_base::out | std::ios_base::trunc);
        } else {
            // Append to existing file
            outFile.open(outputFilePath.string(), std::ios_base::app);
        }

        if (!outFile.is_open()) {
            throw std::runtime_error("Failed to open file: " + outputFilePath.string());
        }

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

        if (writeHeaders) {
            bool firstHeader = true;
            for (const auto& header : headers) {
                if (!firstHeader) outFile << ",";
                firstHeader = false;

                // Handle special characters in CSV headers (quote if necessary, escape internal quotes)
                bool needsQuotes = header.find_first_of(",\"\n") != std::string::npos;
                if (needsQuotes) {
                    outFile << "\"";
                    for (char c : header) {
                        if (c == '"') outFile << "\"\""; // Double up internal quotes
                        else outFile << c;
                    }
                    outFile << "\"";
                } else {
                    outFile << header;
                }
            }
            outFile << "\n";
        }

        // Write data rows
        for (const auto& snapshot : snapshots_) {
            bool firstCell = true;
            for (const auto& header : headers) {
                if (!firstCell) outFile << ",";
                firstCell = false;

                auto it = snapshot.find(header);
                if (it != snapshot.end()) {
                    outFile << it->second; // Data is int, no special quoting needed for int
                } else {
                    outFile << "0"; // Default value for missing fields
                }
            }
            outFile << "\n";
        }

        outFile.close();

        std::cout << "Successfully wrote " << snapshots_.size() << " snapshot(s) to " << outputFilePath << std::endl;

        snapshots_.clear();
        // fieldNames_ is not cleared. For single-file processing per run, it will reflect
        // the fields of that single snapshot. If appending, this means the set of headers
        // used for ordering data comes from the current snapshot. This assumes consistency
        // with pre-existing headers if appending.
    }
    catch (const std::exception& e) {
        std::cerr << "Error writing CSV: " << e.what() << std::endl;
    }
}
