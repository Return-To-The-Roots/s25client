// SnapshotLoader.h
#ifndef SNAPSHOT_LOADER_H
#define SNAPSHOT_LOADER_H

#include "GamePlayer.h"
#include "Game.h"
#include <memory>
#include <vector>

namespace Snapshot {

struct PlayerSnapshot
{
    std::shared_ptr<Game> game;   // Shared ownership among snapshots
    GamePlayer* player;
    unsigned gameframe;
};

std::vector<PlayerSnapshot> GetActivePlayer(const boost::filesystem::path& save_path);
}
#endif // SNAPSHOT_LOADER_H
