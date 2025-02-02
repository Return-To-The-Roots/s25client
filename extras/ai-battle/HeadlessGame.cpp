// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "HeadlessGame.h"
#include "EventManager.h"
#include "GlobalGameSettings.h"
#include "PlayerInfo.h"
#include "Savegame.h"
#include "factories/AIFactory.h"
#include "network/PlayerGameCommands.h"
#include "world/GameWorld.h"
#include "world/MapLoader.h"
#include "gameTypes/MapInfo.h"
#include "gameData/GameConsts.h"
#include <boost/nowide/iostream.hpp>
#include <chrono>
#include <cstdio>
#include <sstream>
#ifdef WIN32
#    include "Windows.h"
#endif

std::vector<PlayerInfo> GeneratePlayerInfo(const std::vector<AI::Info>& ais);
std::string ToString(const std::chrono::milliseconds& time);
std::string HumanReadableNumber(unsigned num);

namespace bfs = boost::filesystem;
namespace bnw = boost::nowide;
using bfs::canonical;

#ifdef WIN32
HANDLE setupStdOut();
#endif

#if defined(__MINGW32__) && !defined(__clang__)
void printConsole(const char* fmt, ...) __attribute__((format(gnu_printf, 1, 2)));
#elif defined __GNUC__
void printConsole(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
#else
void printConsole(const char* fmt, ...);
#endif

HeadlessGame::HeadlessGame(const GlobalGameSettings& ggs, const bfs::path& map, const std::vector<AI::Info>& ais)
    : map_(map), game_(ggs, std::make_unique<EventManager>(0), GeneratePlayerInfo(ais)), world_(game_.world_),
      em_(*static_cast<EventManager*>(game_.em_.get()))
{
    MapLoader loader(world_);
    if(!loader.Load(map))
        throw std::runtime_error("Could not load " + map.string());

    players_.clear();
    for(unsigned playerId = 0; playerId < world_.GetNumPlayers(); ++playerId)
        players_.push_back(AIFactory::Create(world_.GetPlayer(playerId).aiInfo, playerId, world_));

    world_.InitAfterLoad();
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

    for(unsigned playerId = 0; playerId < world_.GetNumPlayers(); ++playerId)
        replay_.AddPlayer(world_.GetPlayer(playerId));
    replay_.ggs = game_.ggs_;
    if(!replay_.StartRecording(path, mapInfo, random_init))
        throw std::runtime_error("Replayfile could not be opened!");
}

void HeadlessGame::SaveGame(const bfs::path& path) const
{
    // Remove old savegame
    bfs::remove(path);

    Savegame save;
    for(unsigned playerId = 0; playerId < world_.GetNumPlayers(); ++playerId)
        save.AddPlayer(world_.GetPlayer(playerId));
    save.ggs = game_.ggs_;
    save.ggs.exploration = Exploration::Disabled; // no FOW
    save.start_gf = em_.GetCurrentGF();
    save.sgd.MakeSnapshot(game_);
    save.Save(path, "AI Battle");

    bnw::cout << "Savegame written to " << canonical(path) << '\n';
}

std::string ToString(const std::chrono::milliseconds& time)
{
    char buffer[90];
    const auto hours = std::chrono::duration_cast<std::chrono::hours>(time);
    const auto minutes = std::chrono::duration_cast<std::chrono::minutes>(time % std::chrono::hours(1));
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(time % std::chrono::minutes(1));
    snprintf(buffer, std::size(buffer), "%03ld:%02ld:%02ld", static_cast<long int>(hours.count()),
             static_cast<long int>(minutes.count()), static_cast<long int>(seconds.count()));
    return std::string(buffer);
}

std::string HumanReadableNumber(unsigned num)
{
    std::stringstream ss;
    ss.imbue(std::locale(""));
    ss << std::fixed << num;
    return ss.str();
}

void HeadlessGame::PrintState()
{
    static bool first_run = true;
    if(first_run)
        first_run = false;
    else
        printConsole("\x1b[%dA", 8 + world_.GetNumPlayers()); // Move cursor back up

    printConsole("┌───────────────┬───────────────────────┬───────────────────────┬────────────────┐\n");
    printConsole(
      "│ GF %10s │ Game Clock  %s │ Wall Clock  %s │ %7s GF/sec │\n", HumanReadableNumber(em_.GetCurrentGF()).c_str(),
      ToString(SPEED_GF_LENGTHS[GameSpeed::Normal] * em_.GetCurrentGF()).c_str(), // elapsed time
      ToString(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - gameStartTime_))
        .c_str(),                                                       // wall clock
      HumanReadableNumber(em_.GetCurrentGF() - lastReportGf_).c_str()); // GF per second
    printConsole("└───────────────┴───────────────────────┴───────────────────────┴────────────────┘\n");
    printConsole("\n");
    printConsole("┌────────────────────────┬─────────────────┬─────────────┬───────────┬───────────┬───────────┐\n");
    printConsole("│ Player                 │ Country         │ Buildings   │ Military  │ Gold      │ Kills     │\n");
    printConsole("├────────────────────────┼─────────────────┼─────────────┼───────────┼───────────┼───────────┤\n");
    for(unsigned playerId = 0; playerId < world_.GetNumPlayers(); ++playerId)
    {
        const GamePlayer& player = world_.GetPlayer(playerId);
        printConsole("│ %s%-22s%s │ %15s │ %11s │ %9s │ %9s │ %9s │\n", player.IsDefeated() ? "\x1b[9m" : "",
                     player.name.c_str(), player.IsDefeated() ? "\x1b[29m" : "",
                     HumanReadableNumber(player.GetStatisticCurrentValue(StatisticType::Country)).c_str(),
                     HumanReadableNumber(player.GetStatisticCurrentValue(StatisticType::Buildings)).c_str(),
                     HumanReadableNumber(player.GetStatisticCurrentValue(StatisticType::Military)).c_str(),
                     HumanReadableNumber(player.GetStatisticCurrentValue(StatisticType::Gold)).c_str(),
                     HumanReadableNumber(player.GetStatisticCurrentValue(StatisticType::Vanquished)).c_str());
    }
    printConsole("└────────────────────────┴─────────────────┴─────────────┴───────────┴───────────┴───────────┘\n");

    lastReportGf_ = em_.GetCurrentGF();
}

std::vector<PlayerInfo> GeneratePlayerInfo(const std::vector<AI::Info>& ais)
{
    std::vector<PlayerInfo> ret;
    PlayerInfo pi;

    for(const AI::Info& ai : ais)
    {
        PlayerInfo pi;
        pi.aiInfo = ai;
        if(ai.type == AI::Type::None)
        {
            pi.ps = PlayerState::Locked;
        } else
        {
            pi.ps = PlayerState::Occupied;
        }
        switch(ai.type)
        {
            case AI::Type::None: pi.name = "None"; break;
            case AI::Type::Default: pi.name = "AIJH " + std::to_string(ret.size()); break;
            case AI::Type::Dummy:
            default: pi.name = "Dummy " + std::to_string(ret.size()); break;
        }
        pi.nation = Nation::Romans;
        pi.team = Team::None;
        ret.push_back(pi);
    }
    return ret;
}

#ifdef WIN32
HANDLE setupStdOut()
{
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleMode(h, ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    SetConsoleOutputCP(65001);
    return h;
}
#endif

void printConsole(const char* fmt, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    const int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    if(len > 0 && (size_t)len < sizeof(buffer))
    {
#ifdef WIN32
        static auto h = setupStdOut();
        WriteConsoleA(h, buffer, len, 0, 0);
#else
        bnw::cout << buffer;
#endif
    }
}
