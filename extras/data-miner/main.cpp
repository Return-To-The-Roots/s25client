#include <iostream>
#include <boost/filesystem/directory.hpp>
#include <vector>
#include <string>
#include <algorithm>

#include "DataMiner.h"
#include "RttrConfig.h"
#include "SnapshotLoader.h"

#include "s25util/Log.h"

#include <boost/filesystem/operations.hpp>

namespace fs = boost::filesystem;

int main(int argc, char* argv[]) {
    RTTRCONFIG.Init();
    LOG.setLogFilepath("/tmp/logs");
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <snapshot_dir> <output_file>\n";
        return 1;
    }

    fs::path snapshot_dir = argv[1];
    std::string output_file = argv[2];

    if (!fs::exists(snapshot_dir) || !fs::is_directory(snapshot_dir)) {
        std::cerr << "Snapshot directory not found: " << snapshot_dir << "\n";
        return 1;
    }

    if (!fs::exists(output_file)) {
        std::cout << "Output directory doesn't exist. Creating it: " << output_file << "\n";
        fs::create_directories(output_file);
    }

    // Collect all .sav files
    std::vector<fs::path> snapshot_files;
    for (const auto& entry : fs::directory_iterator(snapshot_dir)) {
        if (is_regular_file(entry) && entry.path().extension() == ".sav") {
            snapshot_files.push_back(entry.path());
        }
    }

    if (snapshot_files.empty()) {
        std::cerr << "No .sav files found in: " << snapshot_dir << "\n";
        return 1;
    }

    // Sort files for consistent processing order (optional but recommended)
    std::sort(snapshot_files.begin(), snapshot_files.end());

    DataMiner miner{};

    for (const auto& path : snapshot_files) {
        auto snapshot = Snapshot::GetActivePlayer(path);
        if (!snapshot) {
            std::cerr << "Skipping file due to load failure or missing player: " << path << "\n";
            continue;
        }
        miner.ProcessSnapshot(*snapshot->player, snapshot->gameframe);
    }

    // Save all results to a single JSON file
    miner.flush(output_file);

    std::cout << "All snapshots processed and written to: " << output_file << "\n";
    return 0;
}
