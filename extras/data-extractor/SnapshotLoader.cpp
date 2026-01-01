#include "SnapshotLoader.h"

#include "Game.h"
#include "GamePlayer.h"
#include "PlayerInfo.h"
#include "Savegame.h"
#include "network/GameClient.h"
#include <iostream>
#include <vector>

namespace Snapshot {
std::vector<PlayerSnapshot> GetActivePlayer(const boost::filesystem::path& save_path)
{
    std::vector<PlayerSnapshot> snapshots;
    try
    {
        std::cerr << "Processing: " << save_path.filename().string() << std::endl;

        Savegame savegame;
        if(!savegame.Load(save_path, SaveGameDataToLoad::All))
        {
            std::cerr << "Failed to load savegame: " << save_path << std::endl;
            return {};
        }

        const unsigned numPlayers = savegame.GetNumPlayers();
        std::vector<PlayerInfo> players;
        players.reserve(numPlayers);
        for(unsigned i = 0; i < numPlayers; ++i)
        {
            players.emplace_back(savegame.GetPlayer(i));
        }

        auto game = std::make_shared<Game>(GlobalGameSettings(), 0, players);
        savegame.sgd.ReadSnapshot(*game, GameClient::inst());

        unsigned gameframe = savegame.start_gf;

        for(unsigned i = 0; i < game->world_.GetNumPlayers(); ++i)
        {
            auto* player = &game->world_.GetPlayer(i);
            if(player->isUsed())
            {
                snapshots.push_back(PlayerSnapshot{game, player, gameframe});
            }
        }

        if(snapshots.empty())
            std::cerr << "No active players found in: " << save_path << std::endl;

    } catch(const std::exception& e)
    {
        std::cerr << "Error processing " << save_path << ": " << e.what() << std::endl;
    }
    return snapshots;
}
} // namespace Snapshot
