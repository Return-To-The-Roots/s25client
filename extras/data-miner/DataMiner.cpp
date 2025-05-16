#include "DataMiner.h"

#include "GamePlayer.h"
#include "helpers/EnumRange.h"

#include "gameTypes/BuildingType.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/StatisticTypes.h"
#include <boost/filesystem/directory.hpp>
#include <boost/filesystem/operations.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <set>

// Fast-CSV is a header-only library
#define CSV_IO_NO_THREAD
#include "csv.h"

namespace fs = boost::filesystem;

void DataMiner::ProcessSnapshot(GamePlayer& player, uint32_t gameframe)
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
        snapshot[buildingName + "Sites"] = building_nums.buildings[type]; // Replace with actual sites
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
    for (const auto& [field, _] : snapshot)
    {
        fieldNames_.insert(field);
    }
}

void DataMiner::flush(const std::string& filePath)
{
    // Skip if no snapshots to write
    if (snapshots_.empty()) {
        return;
    }

    // Create a boost::filesystem::path from the provided path string
    fs::path outputFilePath(filePath);

    // Delete file if it already exists
    if (fs::exists(outputFilePath)) {
        fs::remove(outputFilePath);
    }

    try {
        // Make sure the directory exists
        fs::path parentDir = outputFilePath.parent_path();
        if (!parentDir.empty() && !fs::exists(parentDir)) {
            fs::create_directories(parentDir);
        }

        // Convert to vector and sort for consistent column order
        // std::vector<std::string> headers(fieldNames_.begin(), fieldNames_.end());
        std::vector<std::string> headers;
        headers.reserve(fieldNames_.size());  // Pre-allocate for efficiency

        // First, add the two special fields if they exist
        headers.push_back("GameFrame");

        // Then add all other fields except those two
        for (const auto& name : fieldNames_) {
            if (name != "GameFrame") {
                headers.push_back(name);
            }
        }

        // Open the output file
        std::ofstream outFile(outputFilePath.string());
        if (!outFile.is_open()) {
            throw std::runtime_error("Failed to open file: " + outputFilePath.string());
        }

        // Write header row
        bool first = true;
        for (const auto& header : headers) {
            if (!first) outFile << ",";
            first = false;

            // Handle special characters in CSV
            if (header.find(',') != std::string::npos ||
                header.find('\"') != std::string::npos ||
                header.find('\n') != std::string::npos) {
                outFile << "\"" << header << "\"";
            } else {
                outFile << header;
            }
        }
        outFile << "\n";

        // Write data rows
        for (const auto& snapshot : snapshots_) {
            first = true;
            for (const auto& header : headers) {
                if (!first) outFile << ",";
                first = false;

                auto it = snapshot.find(header);
                if (it != snapshot.end()) {
                    outFile << it->second;
                } else {
                    outFile << "0"; // Default value for missing fields
                }
            }
            outFile << "\n";
        }

        outFile.close();

        std::cout << "Successfully wrote " << snapshots_.size() << " snapshots to " << outputFilePath << std::endl;

        // Clear snapshots after successful write
        snapshots_.clear();
        // Don't clear fieldNames_ as we want to keep the same headers structure for future flushes
    }
    catch (const std::exception& e) {
        std::cerr << "Error writing CSV: " << e.what() << std::endl;
    }
}