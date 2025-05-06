#include "Game.h"
#include "GamePlayer.h"
#include "ParquetDataMiner.h"
#include "PlayerInfo.h"
#include "RttrConfig.h"
#include "Savegame.h"
#include "network/GameClient.h"

#include <boost/filesystem/directory.hpp>

#include <filesystem>
#include <iostream>
#include <vector>

namespace fs = boost::filesystem;

void ProcessSaveFile(ParquetDataMiner& miner, const fs::path save_path);

// Function to process a single save file
void ProcessSaveFile(ParquetDataMiner& miner, const fs::path save_path)
{
    try
    {
        std::cout << "Processing: " << save_path.filename().string() << std::endl;

        // 1. Load the savegame
        Savegame savegame;
        if(!savegame.Load(save_path, SaveGameDataToLoad::All))
        {
            std::cerr << "Failed to load savegame: " << save_path << std::endl;
            return;
        }

        // 2. Prepare player data
        const unsigned numPlayers = savegame.GetNumPlayers();
        std::vector<PlayerInfo> players;
        players.reserve(numPlayers);

        for(unsigned i = 0; i < numPlayers; ++i)
        {
            players.emplace_back(savegame.GetPlayer(i));
        }

        // 3. Create Game instance
        Game game(GlobalGameSettings(), 0, players);

        // 4. Deserialize game state
        savegame.sgd.ReadSnapshot(game, GameClient::inst());

        // 5. Process with data miner

        // Get all players (assuming you want to process each player)
        for(unsigned i = 0; i < game.world_.GetNumPlayers(); ++i)
        {
            auto player = game.world_.GetPlayer(i);
            if(player.isUsed())
            {
                miner.RecordFrameData(game.world_.GetPlayer(i), savegame.start_gf); // Assuming Game has GetCurrentGF()
            }
        }

        std::cout << "Successfully processed: " << save_path.filename().string() << std::endl;
    } catch(const std::exception& e)
    {
        std::cerr << "Error processing " << save_path << ": " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[])
{
    RTTRCONFIG.Init();
    // 1. Parse command line arguments
    if(argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <saves_directory> <output_directory>" << std::endl;
        return 1;
    }

    fs::path saves_dir(argv[1]);
    fs::path output_dir(argv[2]);

    // 2. Validate directories
    // if(!fs::exists(saves_dir) || !fs::is_directory(saves_dir))
    // {
    //     std::cerr << "Invalid saves directory: " << saves_dir << std::endl;
    //     return 1;
    // }
    //
    // if(!fs::exists(output_dir))
    // {
    //     if(!fs::create_directories(output_dir))
    //     {
    //         std::cerr << "Failed to create output directory: " << output_dir << std::endl;
    //         return 1;
    //     }
    // }

    // 3. Process all .sav files

    std::string run_id = "run_" + std::to_string(std::time(nullptr));
    ParquetDataMiner miner(output_dir.string(), run_id);
    unsigned processed_files = 0;

    for(const auto& entry : fs::directory_iterator(saves_dir))
    {
        if(entry.path().extension() == ".sav")
        {
            // Generate a unique run ID for each file (using filename + timestamp)
            std::string run_id = entry.path().stem().string() + "_" + std::to_string(std::time(nullptr));

            ProcessSaveFile(miner, entry.path());
            processed_files++;
        }
    }

    std::cout << "Processing complete. " << processed_files << " files processed." << std::endl;
    return 0;
}