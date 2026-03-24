// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "HeadlessGame.h"
#include "EventManager.h"
#include "GlobalGameSettings.h"
#include "ILocalGameState.h"
#include "PlayerInfo.h"
#include "Savegame.h"
#include "ai/aijh/config/AIConfig.h"
#include "ai/aijh/debug/StatsConfig.h"
#include "ai/aijh/runtime/AIPlayerJH.h"
#include "factories/AIFactory.h"
#include "helpers/format.hpp"
#include "helpers/random.h"
#include "network/PlayerGameCommands.h"
#include "world/GameWorld.h"
#include "world/GameWorldViewer.h"
#include "world/MapLoader.h"
#include "gameTypes/MapInfo.h"
#include "gameData/NationConsts.h"
#include "gameData/GameConsts.h"
#include "IngameMinimap.h"
#include "nlohmann/json.hpp"
#include "s25util/colors.h"
#include <boost/nowide/iostream.hpp>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>
#ifdef WIN32
#    include "Windows.h"
#endif

std::vector<PlayerInfo> GeneratePlayerInfo(const std::vector<AI::Info>& ais);
std::string ToString(const std::chrono::milliseconds& time);
std::string HumanReadableNumber(unsigned num);

namespace bfs = boost::filesystem;
namespace bnw = boost::nowide;
using bfs::canonical;

namespace {
std::string TeamToString(const Team team)
{
    switch(team)
    {
        case Team::None: return "None";
        case Team::Random: return "Random";
        case Team::Team1: return "Team1";
        case Team::Team2: return "Team2";
        case Team::Team3: return "Team3";
        case Team::Team4: return "Team4";
        case Team::Random1To2: return "Random1To2";
        case Team::Random1To3: return "Random1To3";
        case Team::Random1To4: return "Random1To4";
    }
    return "Unknown";
}

std::string ToHexColor(const unsigned color)
{
    std::ostringstream ss;
    ss << "0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << color;
    return ss.str();
}

class HeadlessLocalGameState : public ILocalGameState
{
public:
    unsigned GetPlayerId() const override { return 0u; }
    bool IsHost() const override { return true; }

    std::string FormatGFTime(unsigned gf) const override
    {
        using seconds = std::chrono::duration<uint32_t, std::chrono::seconds::period>;
        using hours = std::chrono::duration<uint32_t, std::chrono::hours::period>;
        using minutes = std::chrono::duration<uint32_t, std::chrono::minutes::period>;
        using std::chrono::duration_cast;

        seconds numSeconds = duration_cast<seconds>(gf * SPEED_GF_LENGTHS[referenceSpeed]);
        hours numHours = duration_cast<hours>(numSeconds);
        numSeconds -= numHours;
        minutes numMinutes = duration_cast<minutes>(numSeconds);
        numSeconds -= numMinutes;

        if(numHours.count())
            return helpers::format("%u:%02u:%02u", numHours.count(), numMinutes.count(), numSeconds.count());
        else
            return helpers::format("%02u:%02u", numMinutes.count(), numSeconds.count());
    }

    void SystemChat(const std::string& text) override { bnw::cout << text << '\n'; }
};

Game CreateHeadlessGame(const GlobalGameSettings& ggs, const std::vector<AI::Info>& ais,
                        const boost::optional<bfs::path>& startSavePath, std::unique_ptr<Savegame>& startSave)
{
    if(startSavePath)
    {
        auto save = std::make_unique<Savegame>();
        if(!save->Load(*startSavePath, SaveGameDataToLoad::All))
        {
            const std::string lastError = save->GetLastErrorMsg();
            throw std::runtime_error("Could not load savegame " + startSavePath->string()
                                     + (lastError.empty() ? "" : ": " + lastError));
        }
        std::vector<PlayerInfo> players;
        const unsigned numPlayers = save->GetNumPlayers();
        players.reserve(numPlayers);
        for(unsigned i = 0; i < numPlayers; ++i)
            players.emplace_back(save->GetPlayer(i));
        GlobalGameSettings saveSettings = save->ggs;
        const unsigned startGF = save->start_gf;
        startSave = std::move(save);
        return Game(std::move(saveSettings), startGF, players);
    }

    return Game(ggs, std::make_unique<EventManager>(0), GeneratePlayerInfo(ais));
}
} // namespace

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

HeadlessGame::HeadlessGame(const GlobalGameSettings& ggs, const bfs::path& map, const std::vector<AI::Info>& ais,
                           const boost::optional<bfs::path>& startSavePath)
    : map_(map), startSave_(nullptr), game_(CreateHeadlessGame(ggs, ais, startSavePath, startSave_)),
      world_(game_.world_), em_(*static_cast<EventManager*>(game_.em_.get()))
{
    if(startSave_)
    {
        HeadlessLocalGameState localGameState;
        startSave_->sgd.ReadSnapshot(game_, localGameState);
        startSave_.reset();
    } else
    {
        MapLoader loader(world_);
        if(!loader.Load(map))
            throw std::runtime_error("Could not load " + map.string());
    }

    players_.clear();
    for(unsigned playerId = 0; playerId < world_.GetNumPlayers(); ++playerId)
        players_.push_back(AIFactory::Create(world_.GetPlayer(playerId).aiInfo, playerId, world_));

    world_.InitAfterLoad();
    WritePlayersMetadata();
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

        const unsigned currentGF = em_.GetCurrentGF();
        if(currentGF % 1000u == 0u)
            world_.IncreaseGlobalResource(currentGF);

        if(replay_.IsRecording())
            replay_.UpdateLastGF(currentGF);

        if(IsStatsPeriodHit(currentGF, STATS_CONFIG.minimap_period))
            SaveMinimap(currentGF);

        if(std::chrono::steady_clock::now() > nextReport)
        {
            nextReport += std::chrono::seconds(1);
            PrintState();
        }
        const auto saveGameForFrame = [&](unsigned gf) {
            boost::format fmt("%s/ai_run_%s.sav");
            fmt % STATS_CONFIG.savesPath % toPaddedString(gf, 8);
            SaveGame(fmt.str());
        };
        if((STATS_CONFIG.stats_period == 0 || currentGF % STATS_CONFIG.stats_period == 1) && GetActivePlayerCount() <= 1u)
        {
            if(STATS_CONFIG.save_period > 0) saveGameForFrame(currentGF);
            break;
        }

        if(STATS_CONFIG.save_period > 0 && (currentGF == 1 || currentGF % STATS_CONFIG.save_period == 0))
            saveGameForFrame(currentGF);
    }
    PrintState();
}

std::string HeadlessGame::toPaddedString(unsigned int value, int width) {
    std::ostringstream oss;
    oss << std::setw(width) << std::setfill('0') << value;
    return oss.str();
}

unsigned HeadlessGame::GetActivePlayerCount() const
{
    unsigned active_players = 0u;
    const unsigned num_players = world_.GetNumPlayers();
    for(unsigned player_id = 0; player_id < num_players; ++player_id)
    {
        if(!world_.GetPlayer(player_id).IsDefeated())
        {
            ++active_players;
            if(active_players > 1u)
                break;
        }
    }
    return active_players;
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

void HeadlessGame::WritePlayersMetadata() const
{
    if(STATS_CONFIG.statsPath.empty())
        return;

    const bfs::path path = bfs::path(STATS_CONFIG.statsPath) / "players.json";
    try
    {
        bfs::create_directories(path.parent_path());

        nlohmann::json players = nlohmann::json::array();
        for(unsigned playerId = 0; playerId < world_.GetNumPlayers(); ++playerId)
        {
            const GamePlayer& player = world_.GetPlayer(playerId);
            if(player.ps == PlayerState::Locked)
                continue;
            players.push_back({{"playerId", player.GetPlayerId() + 1},
                               {"team", static_cast<unsigned>(player.team)},
                               {"teamName", TeamToString(player.team)},
                               {"color", player.color},
                               {"colorHex", ToHexColor(player.color)},
                               {"nation", static_cast<unsigned>(player.nation)},
                               {"nationName", NationNames[player.nation]}});
        }

        std::ofstream file(path.string(), std::ios::trunc);
        if(!file)
            throw std::runtime_error("Could not open " + path.string());
        file << players.dump(2) << std::endl;
    } catch(const std::exception& e)
    {
        bnw::cerr << "Failed to write player metadata: " << e.what() << std::endl;
    }
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
    bnw::cout << "frame:" << em_.GetCurrentGF() << '\n';
    static bool first_run = true;
    if(first_run)
        first_run = false;
    else
        printConsole("\x1b[%dA", 14 + 2 * world_.GetNumPlayers()); // Move cursor back up

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

    printConsole("\n");

    lastReportGf_ = em_.GetCurrentGF();
}

void HeadlessGame::SaveMinimap(unsigned currentGF)
{
    if(STATS_CONFIG.minimap_period == 0)
        return;
    if(currentGF == 0 || currentGF % STATS_CONFIG.minimap_period != 0)
        return;
    if(lastMinimapSaveGF_ == currentGF)
        return;
    if(STATS_CONFIG.screensPath.empty())
        return;

    lastMinimapSaveGF_ = currentGF;

    try
    {
        minimapViewer_ = std::make_unique<GameWorldViewer>(0, world_);
        minimap_ = std::make_unique<IngameMinimap>(*minimapViewer_);

        const bfs::path filePath =
          bfs::path(STATS_CONFIG.screensPath) / ("ai_map_" + toPaddedString(currentGF, 8) + ".bmp");
        minimap_->SaveToFile(filePath);
    } catch(const std::exception& e)
    {
        bnw::cerr << "Failed to save minimap: " << e.what() << std::endl;
    }
}

std::vector<PlayerInfo> GeneratePlayerInfo(const std::vector<AI::Info>& ais)
{
    std::vector<PlayerInfo> ret;
    ret.reserve(ais.size());

    srand(std::time(0));
    std::size_t nextColorIdx = rand() % (PLAYER_COLORS.size()-1);

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
            // Fallback: reuse colors if the number of AIs exceeds the available set
            nextColorIdx+=2;
            pi.color = PLAYER_COLORS[nextColorIdx % PLAYER_COLORS.size()];
        }
        switch(ai.type)
        {
            case AI::Type::None: pi.name = "None"; break;
            case AI::Type::Default: pi.name = "AIJH " + std::to_string(ret.size()); break;
            case AI::Type::Dummy:
            default: pi.name = "Dummy " + std::to_string(ret.size()); break;
        }
        pi.nation = ai_random::randomEnum<Nation>();
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
