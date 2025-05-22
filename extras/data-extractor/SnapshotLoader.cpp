#include "SnapshotLoader.h"

#include "Game.h"
#include "GamePlayer.h"
#include "PlayerInfo.h"
#include "Savegame.h"
#include "network/GameClient.h"
#include <iostream>
#include <vector>

namespace Snapshot {
std::unique_ptr<PlayerSnapshot> GetActivePlayer(const boost::filesystem::path& save_path)
{
    try
    {
        std::cout << "Processing: " << save_path.filename().string() << std::endl;

        Savegame savegame;
        if(!savegame.Load(save_path, SaveGameDataToLoad::All))
        {
            std::cerr << "Failed to load savegame: " << save_path << std::endl;
            return nullptr;
        }

        const unsigned numPlayers = savegame.GetNumPlayers();
        std::vector<PlayerInfo> players;
        players.reserve(numPlayers);
        for(unsigned i = 0; i < numPlayers; ++i)
        {
            players.emplace_back(savegame.GetPlayer(i));
        }

        auto game = std::make_unique<Game>(GlobalGameSettings(), 0, players);
        savegame.sgd.ReadSnapshot(*game, GameClient::inst());

        unsigned gameframe = savegame.start_gf;

        for(unsigned i = 0; i < game->world_.GetNumPlayers(); ++i)
        {
            auto* player = &game->world_.GetPlayer(i);
            if(player->isUsed())
            {
                auto snapshot = std::make_unique<PlayerSnapshot>();
                snapshot->game = std::move(game);  // Transfer ownership
                snapshot->player = player;
                snapshot->gameframe = gameframe;

                return snapshot;
            }
        }

        std::cerr << "Active player not found in: " << save_path << std::endl;
        return nullptr;

    } catch(const std::exception& e)
    {
        std::cerr << "Error processing " << save_path << ": " << e.what() << std::endl;
        return nullptr;
    }
}
} // namespace Snapshot