#include <iostream>
#include <boost/filesystem/directory.hpp>
#include <vector>
#include <string>
#include <algorithm> // For std::transform

#include "DataExtractor.h"
#include "RttrConfig.h"
#include "SnapshotLoader.h"

#include "s25util/Log.h"

#include <boost/filesystem/operations.hpp>

namespace fs = boost::filesystem;

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

    DataExtractor miner{};

    auto snapshot = Snapshot::GetActivePlayer(snapshot_file_path);
    if (!snapshot) {
        std::cerr << "Skipping file due to load failure or missing player: " << snapshot_file_path << "\n";
        return 1; // Exit if the single specified file cannot be processed
    }
    miner.ProcessSnapshot(*snapshot->player, snapshot->gameframe);

    // Output results to stdout in the chosen format
    miner.flush(outputFormat);

    // The flush method now prints its own status message to cerr.
    // std::cout << "Snapshot " << snapshot_file_path.filename() << " processed and data output to stdout.\n";
    return 0;
}
