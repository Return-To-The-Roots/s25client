#pragma once

#include <iosfwd>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <cstdint>
#include <optional> // Required for std::optional

#include "helpers/EnumArray.h"
#include "gameTypes/GoodTypes.h"

class GamePlayer;
namespace AIJH {
class AIStatsSource;
}

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

    void ProcessSnapshot(const GamePlayer& player, uint32_t gameframe, const AIJH::AIStatsSource* aiStats = nullptr);

    const SnapshotData* GetCurrentSnapshot() const;
    bool HasSnapshot() const { return currentSnapshot_.has_value(); }
    void ClearSnapshot();

    void SerializeCsv(const SnapshotData& snapshot, std::ostream& output, bool write_header) const;
    void SerializeJson(const SnapshotData& snapshot, std::ostream& output) const;

private:
    std::optional<SnapshotData> currentSnapshot_; // Changed from std::vector
    std::set<std::string> fieldNames_; // To track all column headers
};
