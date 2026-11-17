// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameSetup.h"
#include "Game.h"
#include "GamePlayer.h"
#include "ILocalGameState.h"
#include "LeatherLoader.h"
#include "RTTR_Assert.h"
#include "Replay.h"
#include "Savegame.h"
#include "world/GameWorld.h"
#include "world/MapLoader.h"
#include "gameTypes/MapInfo.h"
#include "gameTypes/SettingsTypes.h"
#include "gameTypes/VisualSettings.h"
#include "s25util/tmpFile.h"
#include <boost/filesystem/path.hpp>
#include <optional>
#include <tuple>

namespace {
/*
  We have to do this if we have a old replay starting from scratch not containing a savegame. If a savegame is
  contained in the replay the compatibility code in GamePlayer deserialization function takes care of handling this.
  If we have a replay starting from scratch in the constructor of the gameplayer the standard distributions are
  loaded. These contain also the new leather addon buildings. When the distribution is recomputed these buildings
  are added to the possible goals for wares. This leads to the problem that we have more buildings then before in
  the list. So it happens for example for wood that the ware is deliverd to a different goal and then the replay
  gets out of sync.
*/
void removeLeatherAddonDistributions(Game& game, unsigned localPlayerId)
{
    VisualSettings settings;
    game.world_.getPlayer(localPlayerId).fillVisualSettings(settings);

    Distributions newDistributions = settings.distribution;
    unsigned idx = 0;
    for(const DistributionMapping& mapping : distributionMap)
    {
        if(leatheraddon::isLeatherAddonBuildingType(std::get<1>(mapping)))
            newDistributions[idx] = 0;
        idx++;
    }

    for(auto& player : game.world_.getPlayers())
        player.ChangeDistribution(newDistributions);
}
} // namespace

void setupGameWorld(Game& game, const MapInfo& mapInfo, ILocalGameState& localGameState, const Replay* replay)
{
    GameWorld& gameWorld = game.world_;
    if(mapInfo.savegame)
        mapInfo.savegame->sgd.readSnapshot(game, localGameState);
    else
    {
        RTTR_Assert(mapInfo.type != MapType::Savegame);
        for(auto& player : gameWorld.getPlayers())
            player.makeStartPacts();

        // Kept until the loader has read them
        std::optional<TmpFile> tmpMap, tmpLua;
        boost::filesystem::path mapFilePath, luaFilePath;
        // Replays carry the map and Lua script they were recorded with; their paths refer to the recording machine.
        // Otherwise the paths are local files (on clients, the copy received from the host).
        if(!replay)
        {
            mapFilePath = mapInfo.filepath;
            luaFilePath = mapInfo.luaFilepath;
        } else
        {
            tmpMap.emplace();
            tmpMap->close();
            if(!mapInfo.mapData.decompressToFile(tmpMap->filePath))
                throw GameSetupError("Could not decompress the map data");
            mapFilePath = tmpMap->filePath;
            if(mapInfo.luaData.uncompressedLength > 0)
            {
                tmpLua.emplace(".lua");
                tmpLua->close();
                if(!mapInfo.luaData.decompressToFile(tmpLua->filePath))
                    throw GameSetupError("Could not decompress the Lua script");
                luaFilePath = tmpLua->filePath;
            }
        }

        MapLoader loader(gameWorld);
        if(!loader.Load(mapFilePath))
            throw GameSetupError("Could not load the map " + mapFilePath.string());
        if(!luaFilePath.empty() && !loader.loadLuaScript(game, localGameState, luaFilePath))
            throw GameSetupError("Could not load the Lua script " + luaFilePath.string());

        MapLoader::setupResources(gameWorld, !replay || replay->usesFishFix());
    }
    if(replay)
        gameWorld.setReplayCompatVersion(replay->getMinorVersion());
    gameWorld.initAfterLoad();

    if(replay && !mapInfo.savegame && replay->getMinorVersion() < 2)
        removeLeatherAddonDistributions(game, localGameState.getPlayerId());
}
