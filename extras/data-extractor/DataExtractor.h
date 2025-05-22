#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <cstdint>

class GamePlayer;

// Define the snapshot data type
using SnapshotData = std::unordered_map<std::string, int>;

class DataExtractor
{
public:
    DataExtractor() = default;

    void ProcessSnapshot(GamePlayer& player, uint32_t gameframe);
    void flush(const std::string& filePath);

private:
    std::vector<SnapshotData> snapshots_;
    std::set<std::string> fieldNames_; // To track all column headers
};