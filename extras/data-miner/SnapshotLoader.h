// SnapshotLoader.h
#ifndef SNAPSHOT_LOADER_H
#define SNAPSHOT_LOADER_H

#include "GamePlayer.h"
#include "Game.h"
#include <memory>

namespace Snapshot {

struct PlayerSnapshot
{
    std::unique_ptr<Game> game;   // Owns the Game
    GamePlayer* player;
    unsigned gameframe;
};

std::unique_ptr<PlayerSnapshot> GetActivePlayer(const boost::filesystem::path& save_path);
}
#endif // SNAPSHOT_LOADER_H