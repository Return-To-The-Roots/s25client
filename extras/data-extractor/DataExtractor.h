#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <cstdint>

class GamePlayer;

// Define the snapshot data type
using SnapshotData = std::unordered_map<std::string, uint32_t>; // Changed int to uint32_t

class DataExtractor
{
public:
    enum class OutputFormat {
        CSV,
        JSON
    };

    DataExtractor() = default;

    void ProcessSnapshot(GamePlayer& player, uint32_t gameframe);
    void flush(OutputFormat format); // Changed signature to include format

private:
    std::vector<SnapshotData> snapshots_;
    std::set<std::string> fieldNames_; // To track all column headers
};
