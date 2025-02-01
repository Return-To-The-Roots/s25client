// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GlobalGameSettings.h"
#include "HeadlessGame.h"
#include "QuickStartGame.h"
#include "RTTR_Version.h"
#include "RttrConfig.h"
#include "addons/Addon.h"
#include "ai/aijh/AIConfig.h"
#include "files.h"
#include "random/Random.h"
#include "s25util/System.h"
#include <yaml-cpp/yaml.h>

#include <boost/filesystem.hpp>
#include <boost/nowide/args.hpp>
#include <boost/nowide/filesystem.hpp>
#include <boost/nowide/iostream.hpp>
#include <boost/optional.hpp>
#include <boost/program_options.hpp>

namespace bnw = boost::nowide;
namespace bfs = boost::filesystem;
namespace po = boost::program_options;

int main(int argc, char** argv)
{
    bnw::nowide_filesystem();
    bnw::args _(argc, argv);

    boost::optional<std::string> replay_path;
    boost::optional<std::string> savegame_path;
    unsigned random_init = static_cast<unsigned>(std::chrono::high_resolution_clock::now().time_since_epoch().count());

    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help,h", "Show help")
        ("map,m", po::value<std::string>()->required(),"Map to load")
        ("ai", po::value<std::vector<std::string>>()->required(),"AI player(s) to add")
        ("objective", po::value<std::string>()->default_value("domination"),"domination(default)|conquer")
        ("configfile", po::value<std::string>()->required(), "AI configuration file")
        ("start_wares", po::value<std::string>()->default_value("start_wares"),"Start wares")
        ("replay", po::value(&replay_path),"Filename to write replay to (optional)")
        ("save", po::value(&savegame_path),"Filename to write savegame to (optional)")
        ("random_init", po::value(&random_init),"Seed value for the random number generator (optional)")
        ("maxGF", po::value<unsigned>()->default_value(std::numeric_limits<unsigned>::max()),"Maximum number of game frames to run (optional)")
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

    try
    {
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

        const auto configfile = options["configfile"].as<std::string>();
        YAML::Node configNode = YAML::LoadFile(configfile);
        try
        {
            YAML::Node configNode = YAML::LoadFile(configfile);

            AI_CONFIG.runId = configNode["run_id"].as<std::string>();
            AI_CONFIG.startupMilBuildings = configNode["startup_mil_buildings"].as<unsigned int>();
            AI_CONFIG.farmToIronMineRatio = configNode["farm_to_ironMine_ratio"].as<float>();
            AI_CONFIG.woodcutterToForesterRatio = configNode["woodcutter_to_forester_ratio"].as<float>();
            AI_CONFIG.woodcutterToStorehouseRatio = configNode["woodcutter_to_storehouse_ratio"].as<float>();
            AI_CONFIG.breweryToArmoryRatio = configNode["brewery_to_armory_ratio"].as<float>();
            AI_CONFIG.millToFarmRatio = configNode["mill_to_farm_ratio"].as<double>();
            AI_CONFIG.statsPath = configNode["stats_path"].as<std::string>();
        } catch(const YAML::Exception& e)
        {
            std::cerr << "Error parsing YAML file: " << e.what() << std::endl;
            exit(1);
        }

        GlobalGameSettings ggs;
        const auto objective = options["objective"].as<std::string>();
        if(objective == "domination")
            ggs.objective = GameObjective::TotalDomination;
        else if(objective == "conquer")
            ggs.objective = GameObjective::Conquer3_4;
        else if(objective == "none")
            ggs.objective = GameObjective::None;
        else
        {
            bnw::cerr << "unknown objective: " << objective << std::endl;
            return 1;
        }

        const auto startWares = options["start_wares"].as<std::string>();
        if(startWares == "low")
            ggs.startWares = StartWares::Low;
        else if(startWares == "vlow")
            ggs.startWares = StartWares::VLow;
        else if(startWares == "normal")
            ggs.startWares = StartWares::Normal;
        else if(startWares == "alot")
            ggs.startWares = StartWares::ALot;
        else
        {
            bnw::cerr << "unknown start wares: " << startWares << std::endl;
            return 1;
        }

        ggs.setSelection(AddonId::INEXHAUSTIBLE_MINES, 1);
        ggs.setSelection(AddonId::CHANGE_GOLD_DEPOSITS, 4);
        ggs.setSelection(AddonId::MAX_RANK, 4);

        // ggs.objective = GameObjective::TotalDomination;
        HeadlessGame game(ggs, mapPath, ais);
        if(replay_path)
            game.RecordReplay(*replay_path, random_init);

        game.Run(options["maxGF"].as<unsigned>());
        game.Close();

        if(savegame_path)
        {
            std::string saveTo = *savegame_path + AI_CONFIG.runId + "ai_run_final.sav";
            game.SaveGame(saveTo);
        }
    } catch(const std::exception& e)
    {
        bnw::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
