// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GlobalGameSettings.h"
#include "HeadlessGame.h"
#include "QuickStartGame.h"
#include "RTTR_Version.h"
#include "RttrConfig.h"
#include "files.h"
#include "random/Random.h"
#include "s25util/System.h"

#include <boost/filesystem.hpp>
#include <boost/nowide/args.hpp>
#include <boost/nowide/filesystem.hpp>
#include <boost/nowide/iostream.hpp>
#include <boost/program_options.hpp>

#include <atomic>

namespace bnw = boost::nowide;
namespace bfs = boost::filesystem;
namespace po = boost::program_options;

std::atomic<bool> g_abort = false;

int main(int argc, char** argv)
{
    bnw::nowide_filesystem();
    bnw::args _(argc, argv);

    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help,h", "Show help")
        ("map,m", po::value<std::string>(),"Map to load")
        ("ai", po::value<std::vector<std::string>>(),"AI player(s) to add")
        ("objective", po::value<std::string>(),"domination(default)|conquer")
        ("replay", po::value<std::string>(),"Filename to write replay to (optional)")
        ("save", po::value<std::string>(),"Filename to write savegame to (optional)")
        ("random_init", po::value<unsigned>(),"Seed value for the random number generator (optional)")
        ("maxGF", po::value<unsigned>(),"Maximum number of game frames to run (optional)")
        ("version", "Show version information and exit")
        ;
    // clang-format on

    if(argc == 1)
    {
        bnw::cerr << desc << std::endl;
        return 1;
    }

    po::variables_map options;
    try
    {
        po::store(po::command_line_parser(argc, argv).options(desc).run(), options);
        po::notify(options);
    } catch(const std::exception& e)
    {
        bnw::cerr << "Error: " << e.what() << std::endl;
        bnw::cerr << desc << std::endl;
        return 1;
    }

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
    if(options.count("map") == 0)
    {
        bnw::cerr << "No map specified" << std::endl;
        return 1;
    }
    if(options.count("ai") == 0)
    {
        bnw::cerr << "No AI specified" << std::endl;
        return 1;
    }

    try
    {
        auto random_init = static_cast<unsigned>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        if(options.count("random_init"))
            random_init = options["random_init"].as<unsigned>();

        // We print arguments and seed in order to be able to reproduce crashes.
        for(int i = 0; i < argc; ++i)
            bnw::cout << argv[i] << " ";
        bnw::cout << std::endl;
        bnw::cout << "random_init: " << random_init << std::endl;
        bnw::cout << std::endl;

        RTTRCONFIG.Init();
        RANDOM.Init(random_init);

        const bfs::path mapPath = RTTRCONFIG.ExpandPath(options["map"].as<std::string>());
        const std::vector<AI::Info> ais = ParseAIOptions(options["ai"].as<std::vector<std::string>>());

        GlobalGameSettings ggs;
        if(options.count("objective"))
        {
            std::string objective = options["objective"].as<std::string>();
            if(objective == "domination")
                ggs.objective = GameObjective::TotalDomination;
            else if(objective == "conquer")
                ggs.objective = GameObjective::Conquer3_4;
            else
            {
                bnw::cerr << "unknown objective: " << objective << std::endl;
                return 1;
            }
        } else
            ggs.objective = GameObjective::TotalDomination;

        ggs.objective = GameObjective::TotalDomination;
        HeadlessGame game(ggs, mapPath, ais);
        if(options.count("replay"))
            game.StartReplay(options["replay"].as<std::string>(), random_init);

        unsigned maxGF = std::numeric_limits<unsigned>::max();
        if(options.count("maxGF"))
            maxGF = options["maxGF"].as<unsigned>();

        game.Run(maxGF);
        game.Close();
        if(options.count("save"))
            game.SaveGame(options["save"].as<std::string>());
    } catch(const std::exception& e)
    {
        bnw::cerr << e.what() << std::endl;
        return 1;
    } catch(...)
    {
        bnw::cerr << "An unknown exception occurred" << std::endl;
        return 1;
    }

    return 0;
}
