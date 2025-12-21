#include <iostream>
#include <boost/filesystem/directory.hpp>
#include <vector>
#include <string>
#include <algorithm> // For std::transform
#include <exception>

#include "dataextractor/DataExtractor.h"
#include "ai/aijh/AIPlayerJH.h"
#include "RttrConfig.h"
#include "SnapshotLoader.h"

#include "s25util/Log.h"

#include <boost/filesystem/operations.hpp>

namespace fs = boost::filesystem;

namespace {
bool FlushSnapshot(DataExtractor& extractor, DataExtractor::OutputFormat format, bool write_header)
{
    const SnapshotData* current_snapshot = extractor.GetCurrentSnapshot();
    if(!current_snapshot)
    {
        std::cerr << "No snapshot data available after processing player." << std::endl;
        return false;
    }

    try
    {
        if(format == DataExtractor::OutputFormat::CSV)
        {
            extractor.SerializeCsv(*current_snapshot, std::cout, write_header);
            std::cerr << "Successfully wrote 1 snapshot as CSV to stdout." << std::endl;
        }
        else if(format == DataExtractor::OutputFormat::JSON)
        {
            extractor.SerializeJson(*current_snapshot, std::cout);
            std::cerr << "Successfully wrote 1 snapshot as JSON object to stdout." << std::endl;
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "Error writing data to stdout: " << e.what() << std::endl;
        extractor.ClearSnapshot();
        return false;
    }

    extractor.ClearSnapshot();
    return true;
}
} // namespace

int main(int argc, char* argv[]) {
    RTTRCONFIG.Init();
    LOG.setLogFilepath("/tmp/logs"); // Note: Ensure this path is suitable for your logging setup.

    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: " << argv[0] << " <snapshot_file> [format]\n";
        std::cerr << "  format: csv (default) or json\n";
        return 1;
    }

    fs::path snapshot_file_path = argv[1];
    DataExtractor::OutputFormat outputFormat = DataExtractor::OutputFormat::CSV; // Default to CSV

    if (argc == 3) {
        std::string format_str = argv[2];
        std::transform(format_str.begin(), format_str.end(), format_str.begin(),
                       [](unsigned char c){ return std::tolower(c); }); // Convert to lowercase
        if (format_str == "json") {
            outputFormat = DataExtractor::OutputFormat::JSON;
        } else if (format_str != "csv") {
            std::cerr << "Invalid format: " << argv[2] << ". Use 'csv' or 'json'.\n";
            return 1;
        }
    }

    if (!fs::exists(snapshot_file_path)) {
        std::cerr << "Snapshot file not found: " << snapshot_file_path << "\n";
        return 1;
    }
    if (!fs::is_regular_file(snapshot_file_path)) {
        std::cerr << "Snapshot path is not a regular file: " << snapshot_file_path << "\n";
        return 1;
    }
    if (snapshot_file_path.extension() != ".sav") {
        std::cerr << "Snapshot file must have a .sav extension: " << snapshot_file_path << "\n";
        return 1;
    }

    DataExtractor extractor{};

    auto snapshots = Snapshot::GetActivePlayer(snapshot_file_path);
    if (snapshots.empty()) {
        std::cerr << "Skipping file due to load failure or missing players: " << snapshot_file_path << "\n";
        return 1; // Exit if no players could be processed
    }

    bool write_header = true;
    for (const auto& snapshot : snapshots) {
        const AIPlayer* ai = snapshot.game->GetAIPlayer(snapshot.player->GetPlayerId());
        extractor.ProcessSnapshot(*snapshot.player, snapshot.gameframe, ai);
        const bool flushed = FlushSnapshot(extractor, outputFormat, write_header);
        if(flushed && write_header)
            write_header = false;
    }

    // The FlushSnapshot helper prints status messages to cerr.
    return 0;
}
