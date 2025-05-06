// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DataMiner.h"
#include "Game.h"
#include "GamePlayer.h"
#include "PlayerInfo.h"
#include "RTTR_Version.h"
#include "RttrConfig.h"
#include "Savegame.h"
#include "ai/aijh/StatsConfig.h"
#include "network/GameClient.h"
#include "random/Random.h"

#include "s25util/Log.h"
#include <yaml-cpp/yaml.h>

#include <boost/nowide/iostream.hpp>
#include <boost/program_options.hpp>
#include <filesystem>

namespace bnw = boost::nowide;
namespace bfs = boost::filesystem;
namespace po = boost::program_options;

int main(int argc, char* argv[])
{
    RTTRCONFIG.Init();
    // 1. Read save file path from argument
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <savefile_path>" << std::endl;
        return 1;
    }

    const boost::filesystem::path saveFilePath("/home/pavel/Documents/settlers_ai/manual/runsets/002/001/saves/ai_run_final_002_001.sav");
//    const boost::filesystem::path saveFilePath(argv[1]);

    // 2. Load it into Savegame class
    Savegame savegame;
    if (!savegame.Load(saveFilePath, SaveGameDataToLoad::All)) {
        std::cerr << "Failed to load savegame: " << saveFilePath << std::endl;
        return 1;
    }

    // Prepare data for Game constructor
    const unsigned numPlayers = savegame.GetNumPlayers();
    std::vector<PlayerInfo> players;
    players.reserve(numPlayers);

    for (unsigned i = 0; i < numPlayers; ++i) {
        players.emplace_back(savegame.GetPlayer(i));
    }

    Game game(GlobalGameSettings(), savegame.start_gf, players);

    savegame.sgd.ReadSnapshot(game, GameClient::inst());

    DataMiner miner(savegame.start_gf, game);
    miner.Run();

    std::cout << "Game successfully loaded from savefile: " << saveFilePath << std::endl;
    return 0;
}
