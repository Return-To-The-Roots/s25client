// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "HeadlessGame.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "GameSetup.h"
#include "GlobalGameSettings.h"
#include "HeadlessConsole.h"
#include "PlayerInfo.h"
#include "Savegame.h"
#include "factories/AIFactory.h"
#include "network/PlayerGameCommands.h"
#include "world/GameWorld.h"
#include "gameTypes/MapInfo.h"
#include "gameData/GameConsts.h"
#include "s25util/colors.h"
#include <boost/nowide/iostream.hpp>
#include <chrono>

std::vector<PlayerInfo> GeneratePlayerInfo(const std::vector<AI::Info>& ais, const std::vector<Team>& teams);

namespace bfs = boost::filesystem;
namespace bnw = boost::nowide;

using bfs::canonical;

HeadlessGame::HeadlessGame(const GlobalGameSettings& ggs, const bfs::path& map, const std::vector<AI::Info>& ais,
                           const bfs::path& luaPath, const std::vector<Team>& teams)
    : map_(map), game_(ggs, std::make_unique<EventManager>(0), GeneratePlayerInfo(ais, teams)), world_(game_.world_),
      em_(*static_cast<EventManager*>(game_.em_.get()))
{
    MapInfo mapInfo;
    mapInfo.type = MapType::OldMap;
    mapInfo.filepath = map;
    mapInfo.luaFilepath = luaPath;
    SetupGameWorld(game_, mapInfo, localState_, nullptr);

    if(world_.HasLua())
    {
        world_.GetLua().setSuppressStdout(true);
        luaPath_ = luaPath;
        bnw::cout << "Lua script loaded: " << luaPath << '\n';
    }

    for(unsigned playerId = 0; playerId < world_.GetNumPlayers(); ++playerId)
        players_.push_back(AIFactory::Create(world_.GetPlayer(playerId).aiInfo, playerId, world_));
}

HeadlessGame::~HeadlessGame()
{
    Close();
}

void HeadlessGame::Run(unsigned maxGF)
{
    AsyncChecksum checksum;
    gameStartTime_ = std::chrono::steady_clock::now();
    auto nextReport = gameStartTime_ + std::chrono::seconds(1);

    game_.Start(false);

    while(em_.GetCurrentGF() < maxGF && !game_.IsGameFinished())
    {
        // In the actual game, the network frame intervall is based on ping (highest_ping < NFW-length < 20*gf_length).
        bool isnfw = em_.GetCurrentGF() % 20 == 0;

        if(isnfw)
        {
            if(replay_.IsRecording())
                checksum = AsyncChecksum::create(game_);
            for(unsigned playerId = 0; playerId < world_.GetNumPlayers(); ++playerId)
            {
                world_.GetPlayer(playerId);
                AIPlayer* player = players_[playerId].get();
                PlayerGameCommands cmds;
                cmds.gcs = player->FetchGameCommands();

                if(replay_.IsRecording() && !cmds.gcs.empty())
                {
                    cmds.checksum = checksum;
                    replay_.AddGameCommand(em_.GetCurrentGF(), playerId, cmds);
                }

                for(const gc::GameCommandPtr& gc : cmds.gcs)
                    gc->Execute(world_, player->GetPlayerId());
            }
        }

        for(auto& player : players_)
            player->RunGF(em_.GetCurrentGF(), isnfw);

        game_.RunGF();

        if(replay_.IsRecording())
            replay_.UpdateLastGF(em_.GetCurrentGF());

        if(std::chrono::steady_clock::now() > nextReport)
        {
            nextReport += std::chrono::seconds(1);
            PrintState();
        }
    }
    PrintState();
}

void HeadlessGame::Close()
{
    bnw::cout << '\n';

    if(replay_.IsRecording())
    {
        replay_.StopRecording();
        bnw::cout << "Replay written to " << canonical(replayPath_) << '\n';
    }

    replay_.Close();
}

void HeadlessGame::RecordReplay(const bfs::path& path, unsigned random_init)
{
    // Remove old replay
    bfs::remove(path);

    replayPath_ = path;

    MapInfo mapInfo;
    mapInfo.filepath = map_;
    mapInfo.mapData.CompressFromFile(mapInfo.filepath, &mapInfo.mapChecksum);
    mapInfo.type = MapType::OldMap;

    if(!luaPath_.empty() && bfs::exists(luaPath_))
    {
        mapInfo.luaFilepath = luaPath_;
        mapInfo.luaData.CompressFromFile(luaPath_, &mapInfo.luaChecksum);
    }

    for(auto& player : world_.getPlayers())
        replay_.AddPlayer(player);
    replay_.ggs = game_.ggs_;
    if(!replay_.StartRecording(path, mapInfo, random_init))
        throw std::runtime_error("Replayfile could not be opened!");
}

void HeadlessGame::SaveGame(const bfs::path& path) const
{
    // Remove old savegame
    bfs::remove(path);

    Savegame save;
    for(auto& player : world_.getPlayers())
        save.AddPlayer(player);
    save.ggs = game_.ggs_;
    save.ggs.exploration = Exploration::Disabled; // no FOW
    save.start_gf = em_.GetCurrentGF();
    save.sgd.MakeSnapshot(game_);
    save.Save(path, "AI Battle");

    bnw::cout << "Savegame written to " << canonical(path) << '\n';
}

void HeadlessGame::PrintState()
{
    HeadlessStats stats;
    stats.currentGF = em_.GetCurrentGF();
    stats.gameTime =
      std::chrono::duration_cast<std::chrono::milliseconds>(SPEED_GF_LENGTHS[GameSpeed::Normal] * em_.GetCurrentGF());
    stats.wallTime =
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - gameStartTime_);
    stats.gfPerSecond = em_.GetCurrentGF() - lastReportGf_;
    statsPrinter_.print(stats, world_);
    lastReportGf_ = em_.GetCurrentGF();
}

std::vector<PlayerInfo> GeneratePlayerInfo(const std::vector<AI::Info>& ais, const std::vector<Team>& teams)
{
    std::vector<PlayerInfo> ret;
    for(const AI::Info& ai : ais)
    {
        PlayerInfo pi;
        pi.ps = PlayerState::Occupied;
        pi.aiInfo = ai;
        switch(ai.type)
        {
            case AI::Type::Default: pi.name = "AIJH " + std::to_string(ret.size()); break;
            case AI::Type::Dummy:
            default: pi.name = "Dummy " + std::to_string(ret.size()); break;
        }
        pi.nation = Nation::Romans;
        pi.team = (ret.size() < teams.size()) ? teams[ret.size()] : Team::None;
        pi.color = PLAYER_COLORS[ret.size() % PLAYER_COLORS.size()];
        ret.push_back(pi);
    }
    return ret;
}
