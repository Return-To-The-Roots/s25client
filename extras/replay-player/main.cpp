// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "HeadlessConsole.h"
#include "HeadlessReplay.h"
#include "RTTR_Version.h"
#include "Replay.h"
#include "ReplayOutput.h"
#include "RttrConfig.h"
#include "files.h"
#include "ogl/glAllocator.h"
#include "world/GameWorld.h"
#include "gameData/GameConsts.h"
#include "libsiedler2/libsiedler2.h"
#include "s25util/Log.h"
#include "s25util/System.h"

#include <boost/filesystem.hpp>
#include <boost/nowide/args.hpp>
#include <boost/nowide/filesystem.hpp>
#include <boost/nowide/iostream.hpp>
#include <boost/program_options.hpp>
#include <chrono>

namespace bfs = boost::filesystem;
namespace bnw = boost::nowide;
namespace po = boost::program_options;

int main(int argc, char** argv)
{
    bnw::nowide_filesystem();
    bnw::args _(argc, argv);

    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help,h", "Show help")
        ("replay,r", po::value<std::string>()->required(), "Replay file (.rpl) to play back\n"
                        "Supports <RTTR_USERDATA> placeholder (user data dir)")
        ("verbose,V", "Print the async-log entries when a desync is detected")
        ("version,v", "Show version information and exit")
    ;
    // clang-format on
    po::positional_options_description pos;
    pos.add("replay", 1);

    if(argc == 1)
    {
        bnw::cerr << desc << std::endl;
        return 1;
    }

    po::variables_map options;
    try
    {
        po::store(po::command_line_parser(argc, argv).options(desc).positional(pos).run(), options);
        if(options.count("help"))
        {
            bnw::cout << desc << std::endl;
            return 0;
        }
        if(options.count("version"))
        {
            bnw::cout << rttr::version::GetTitle() << " v" << rttr::version::GetVersion() << "-"
                      << rttr::version::GetRevision() << std::endl
                      << "Compiled with " << System::getCompilerName() << " for " << System::getOSName() << std::endl;
            return 0;
        }
        po::notify(options);
    } catch(const std::exception& e)
    {
        bnw::cerr << "Error: " << e.what() << std::endl;
        bnw::cerr << desc << std::endl;
        return 1;
    }

    for(int i = 0; i < argc; ++i)
        bnw::cout << argv[i] << " ";
    bnw::cout << "\n\n";

    try
    {
        RTTRCONFIG.Init();
        // Lua output goes to the log file, and writing it throws if the log directory does not exist
        const bfs::path logDir = RTTRCONFIG.ExpandPath(s25::folders::logs);
        bfs::create_directories(logDir);
        LOG.setLogFilepath(logDir);
        libsiedler2::setAllocator(new GlAllocator);

        const bfs::path replayPath = RTTRCONFIG.ExpandPath(options["replay"].as<std::string>());
        const bool verbose = options.count("verbose") > 0;

        bnw::cout << "Loading: " << replayPath << "\n";

        HeadlessReplay replay(replayPath);
        printInitialInfo(replay);

        const auto startTime = std::chrono::steady_clock::now();
        auto nextReport = startTime + std::chrono::seconds(1);
        unsigned lastReportGF = replay.getStartGF();
        StatsTablePrinter statsPrinter;

        const auto printStats = [&] {
            HeadlessStats stats;
            stats.currentGF = replay.getCurrentGF();
            stats.totalGFs = replay.getReplay().GetLastGF() + 1;
            stats.gameTime = std::chrono::duration_cast<std::chrono::milliseconds>(SPEED_GF_LENGTHS[GameSpeed::Normal]
                                                                                   * stats.currentGF);
            stats.wallTime =
              std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime);
            stats.gfPerSecond = stats.currentGF - lastReportGF;
            statsPrinter.print(stats, replay.getWorld());
            lastReportGF = stats.currentGF;
        };

        replay.Run([&](const HeadlessReplay&) {
            const auto now = std::chrono::steady_clock::now();
            if(now >= nextReport)
            {
                nextReport += std::chrono::seconds(1);
                printStats();
            }
        });
        printStats();
        printConsole("\n");

        const auto elapsed =
          std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::steady_clock::now() - startTime)
            .count();
        const unsigned playedGFs = replay.getCurrentGF() - replay.getStartGF();
        bnw::cout << "Finished " << playedGFs << " GFs in " << elapsed << "s ("
                  << static_cast<unsigned>(playedGFs / elapsed) << " GF/s)\n";

        if(const auto desync = replay.getDesync())
        {
            printDesync(*desync, verbose);
            return 1;
        }
        bnw::cout << "OK: No desync detected.\n";
        return 0;

    } catch(const std::exception& e)
    {
        bnw::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
}
