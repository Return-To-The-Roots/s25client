#include <iostream>
#include <boost/filesystem/directory.hpp>
#include <vector>
#include <string>
#include <algorithm>

#include "DataExtractor.h"
#include "RttrConfig.h"
#include "SnapshotLoader.h"

#include "s25util/Log.h"

#include <boost/filesystem/operations.hpp>

namespace fs = boost::filesystem;

int main(int argc, char* argv[]) {
    RTTRCONFIG.Init();
    LOG.setLogFilepath("/tmp/logs"); // Note: Ensure this path is suitable for your logging setup.
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <snapshot_file> <output_file>\n";
        return 1;
    }

    fs::path snapshot_file_path = argv[1];
    std::string output_file_str = argv[2];

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

    // The DataExtractor::flush method will handle creating parent directories for the output file.
    // No need to explicitly create directories for output_file_str here.

    DataExtractor miner{};

    auto snapshot = Snapshot::GetActivePlayer(snapshot_file_path);
    if (!snapshot) {
        std::cerr << "Skipping file due to load failure or missing player: " << snapshot_file_path << "\n";
        return 1; // Exit if the single specified file cannot be processed
    }
    miner.ProcessSnapshot(*snapshot->player, snapshot->gameframe);

    // Save/append results to the CSV file
    miner.flush(output_file_str);

    std::cout << "Snapshot " << snapshot_file_path.filename() << " processed and data appended to: " << output_file_str << "\n";
    return 0;
}
