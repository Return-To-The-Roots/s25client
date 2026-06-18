// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AsyncChecksum.h"
#include "EventManager.h"
#include "Game.h"
#include "GamePlayer.h"
#include "HeadlessReplay.h"
#include "PlayerInfo.h"
#include "RTTR_Version.h"
#include "Replay.h"
#include "RttrConfig.h"
#include "Savegame.h"
#include "ogl/glAllocator.h"
#include "random/Random.h"
#include "random/randomIO.h"
#include "variant.h"
#include "world/GameWorld.h"
#include "world/MapLoader.h"
#include "gameTypes/MapInfo.h"
#include "libsiedler2/libsiedler2.h"
#include "s25util/System.h"
#include "s25util/tmpFile.h"

#include <boost/filesystem.hpp>
#include <boost/nowide/args.hpp>
#include <boost/nowide/filesystem.hpp>
#include <boost/nowide/iostream.hpp>
#include <boost/program_options.hpp>
#include <chrono>

namespace bfs = boost::filesystem;
namespace bnw = boost::nowide;
namespace po = boost::program_options;

static void printBriefUsage(const char* prog)
{
    bnw::cerr << "Usage: " << prog
              << " [options] <replay-file>\n"
                 "Run with --help or -h for a full list of options.\n";
}

static const char PATH_EXPANSION_HELP[] =
  "\nPath expansion (--replay):\n"
  "  The replay path supports <RTTR_X> placeholders and a leading ~.\n"
  "  ~ is expanded by the application: %USERPROFILE%\\Saved Games on Windows,\n"
  "  $HOME on Linux/macOS.\n"
  "  Available placeholders:\n"
  "    <RTTR_USERDATA>   user data dir\n"
  "                        Windows : %USERPROFILE%\\Saved Games\\Return To The Roots\n"
  "                        Linux   : $HOME/.s25rttr\n"
  "                        macOS   : $HOME/Library/Application Support/Return To The Roots\n"
  "    <RTTR_DATA>       RTTR data directory\n"
  "    <RTTR_GAME>       directory containing S2 DATA/ and GFX/ folders\n"
  "    <RTTR_RTTR>       <RTTR_DATA>/RTTR sub-directory\n"
  "    <RTTR_BIN>        binary directory\n"
  "    <RTTR_EXTRA_BIN>  extra binary directory\n"
  "    <RTTR_LIB>        library directory\n"
  "    <RTTR_DRIVER>     driver directory\n"
  "  Each placeholder can be overridden with the RTTR_<ID>_DIR environment variable.\n"
  "\nExample:\n"
  "  replay-player \"<RTTR_USERDATA>/REPLAYS/game.rpl\"\n";

int main(int argc, char** argv)
{
    bnw::nowide_filesystem();
    bnw::args _(argc, argv);

    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help,h",    "Show this detailed help and exit.")
        ("replay,r",  po::value<std::string>()->required(),
                      "Replay file (.rpl) to play back. Can also be passed as a positional argument.\n"
                      "Supports <RTTR_X> path expansion (see Path expansion below).")
        ("verbose,V", "Print the async-log entries when the first desync is detected.")
        ("version,v", "Show version information and exit.")
    ;
    // clang-format on
    po::positional_options_description pos;
    pos.add("replay", 1);

    if(argc == 1)
    {
        printBriefUsage(argv[0]);
        return 1;
    }

    po::variables_map options;
    try
    {
        po::store(po::command_line_parser(argc, argv).options(desc).positional(pos).run(), options);
        if(options.count("help"))
        {
            bnw::cout << desc << PATH_EXPANSION_HELP;
            return 0;
        }
        if(options.count("version"))
        {
            bnw::cout << rttr::version::GetTitle() << " v" << rttr::version::GetVersion() << "-"
                      << rttr::version::GetRevision() << "\n"
                      << "Compiled with " << System::getCompilerName() << " for " << System::getOSName() << "\n";
            return 0;
        }
        po::notify(options);
    } catch(const std::exception& e)
    {
        bnw::cerr << "Error: " << e.what() << "\n";
        printBriefUsage(argv[0]);
        return 1;
    }

    for(int i = 0; i < argc; ++i)
        bnw::cout << argv[i] << " ";
    bnw::cout << "\n\n";

    try
    {
        RTTRCONFIG.Init();
        libsiedler2::setAllocator(new GlAllocator);

        const bfs::path replayPath = RTTRCONFIG.ExpandPath(options["replay"].as<std::string>());
        const bool verbose = options.count("verbose") > 0;

        bnw::cout << "Loading: " << replayPath << "\n";

        Replay replay;
        if(!replay.LoadHeader(replayPath))
        {
            bnw::cerr << "Failed to load replay header: " << replay.GetLastErrorMsg() << "\n";
            return 1;
        }
        MapInfo mapInfo;
        if(!replay.LoadGameData(mapInfo))
        {
            bnw::cerr << "Failed to load replay game data: " << replay.GetLastErrorMsg() << "\n";
            return 1;
        }

        std::vector<PlayerInfo> players;
        for(unsigned i = 0; i < replay.GetNumPlayers(); i++)
            players.emplace_back(replay.GetPlayer(i));

        Game game(replay.ggs, /*startGF*/ 0, players);
        RANDOM.Init(replay.getSeed());
        GameWorld& gameWorld = game.world_;

        const bool isSavegame = (mapInfo.savegame != nullptr);
        if(isSavegame)
        {
            HeadlessGameState gs;
            mapInfo.savegame->sgd.ReadSnapshot(game, gs);
        } else
        {
            TmpFile mapfile;
            mapfile.close();
            if(!mapInfo.mapData.DecompressToFile(mapfile.filePath))
            {
                bnw::cerr << "Failed to decompress embedded map data\n";
                return 1;
            }
            MapLoader loader(gameWorld);
            if(!loader.Load(mapfile.filePath))
            {
                bnw::cerr << "Failed to load map\n";
                return 1;
            }
            if(mapInfo.luaData.uncompressedLength > 0)
            {
                TmpFile luaFile(".lua");
                luaFile.close();
                if(!mapInfo.luaData.DecompressToFile(luaFile.filePath))
                {
                    bnw::cerr << "Failed to decompress embedded Lua script\n";
                    return 1;
                }
                HeadlessGameState gs;
                if(!loader.LoadLuaScript(game, gs, luaFile.filePath))
                {
                    bnw::cerr << "Failed to load embedded Lua script\n";
                    return 1;
                }
                bnw::cout << "Lua script loaded from replay.\n";
            }
            const bool fixFish = !(replay.GetMajorVersion() == 8 && replay.GetMinorVersion() < 3);
            MapLoader::SetupResources(gameWorld, fixFish);
            if(!fixFish)
                bnw::cout << "Note: fish fix skipped (replay version "
                          << static_cast<unsigned>(replay.GetMajorVersion()) << "."
                          << static_cast<unsigned>(replay.GetMinorVersion()) << " predates 8.3)\n";

            for(unsigned i = 0; i < gameWorld.GetNumPlayers(); ++i)
                gameWorld.GetPlayer(i).MakeStartPacts();
        }

        gameWorld.InitAfterLoad();

        const unsigned startGF = game.em_->GetCurrentGF();
        printInitialInfo(replay, gameWorld, isSavegame, startGF);

        auto nextGF = replay.ReadGF();
        if(!nextGF)
        {
            bnw::cerr << "Empty replay: no commands found\n";
            return 1;
        }

        ReplayStatus status{game, gameWorld, replay.GetLastGF(), std::chrono::steady_clock::now(), startGF};
        auto nextReport = status.startTime + std::chrono::seconds(1);

        bool endOfReplay = false;
        unsigned asyncCount = 0;

        do
        {
            const unsigned curGF = game.em_->GetCurrentGF();

            AsyncChecksum checksum;
            if(*nextGF == curGF)
                checksum = AsyncChecksum::create(game);

            while(*nextGF == curGF)
            {
                const auto cmd = replay.ReadCommand();
                visit(composeVisitor([](const Replay::ChatCommand&) {},
                                     [&](const Replay::GameCommand& gcmd) {
                                         for(const gc::GameCommandPtr& gc : gcmd.cmds.gcs)
                                             gc->Execute(game.world_, gcmd.player);

                                         const AsyncChecksum& stored = gcmd.cmds.checksum;
                                         if(stored.randChecksum != 0 && stored != checksum)
                                         {
                                             ++asyncCount;
                                             if(asyncCount == 1)
                                             {
                                                 bnw::cerr << "\nFirst desync at GF " << curGF << ":\n"
                                                           << "  actual:  " << checksum << "\n"
                                                           << "  stored:  " << stored << "\n";
                                                 if(verbose)
                                                 {
                                                     for(const auto& entry : RANDOM.GetAsyncLog())
                                                         bnw::cerr << "  " << entry << "\n";
                                                 }
                                             }
                                         }
                                     }),
                      cmd);

                nextGF = replay.ReadGF();
                if(!nextGF)
                {
                    endOfReplay = true;
                    break;
                }
            }

            game.RunGF();

            const auto now = std::chrono::steady_clock::now();
            if(now >= nextReport)
            {
                nextReport += std::chrono::seconds(1);
                printTable(status);
                status.lastReportGF = game.em_->GetCurrentGF();
            }
        } while(!endOfReplay);

        // final table
        printTable(status);
        printConsole("\n");

        const unsigned finalGF = game.em_->GetCurrentGF();
        const float elapsed =
          std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::steady_clock::now() - status.startTime)
            .count();
        bnw::cout << "Finished " << finalGF << " GFs in " << elapsed << "s ("
                  << static_cast<unsigned>(finalGF / elapsed) << " GF/s)\n";

        if(asyncCount > 0)
        {
            bnw::cerr << "FAIL: " << asyncCount << " async frame(s) detected.\n";
            return 1;
        }
        bnw::cout << "OK: no desync detected.\n";
        return 0;

    } catch(const std::exception& e)
    {
        bnw::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
}
