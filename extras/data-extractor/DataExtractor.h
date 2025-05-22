#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <cstdint>
#include <optional> // Required for std::optional

class GamePlayer;

// Define the snapshot data type
using SnapshotData = std::unordered_map<std::string, uint32_t>;

class DataExtractor
{
public:
    enum class OutputFormat {
        CSV,
        JSON
    };

    DataExtractor() = default;

    void ProcessSnapshot(GamePlayer& player, uint32_t gameframe);
    void flush(OutputFormat format);

private:
    std::optional<SnapshotData> currentSnapshot_; // Changed from std::vector
    std::set<std::string> fieldNames_; // To track all column headers
};
