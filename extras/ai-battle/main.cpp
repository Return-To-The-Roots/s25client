#include "GlobalGameSettings.h"
#include "HeadlessGame.h"
#include "QuickStartGame.h"
#include "RTTR_Version.h"
#include "RttrConfig.h"
#include "addons/Addon.h"
#include "ai/aijh/config/AIConfig.h"
#include "ai/aijh/debug/StatsConfig.h"
#include "gameData/MaxPlayers.h"
#include "files.h"
#include "random/Random.h"

#include "s25util/Log.h"
#include "s25util/System.h"
#include <yaml-cpp/yaml.h>

#include <boost/filesystem.hpp>
#include <boost/nowide/args.hpp>
#include <boost/nowide/filesystem.hpp>
#include <boost/nowide/iostream.hpp>
#include <boost/optional.hpp>
#include <boost/program_options.hpp>
#include <filesystem>
#include <stdexcept>

namespace bnw = boost::nowide;
namespace bfs = boost::filesystem;
namespace po = boost::program_options;

int main(int argc, char** argv)
{
    bnw::nowide_filesystem();
    bnw::args _(argc, argv);

    boost::optional<std::string> replay_path;
    boost::optional<std::string> output_path;
    boost::optional<std::string> runId;
    boost::optional<std::string> profileId;
    boost::optional<std::string> runSetId;
    boost::optional<std::string> start_save_path;
    boost::optional<unsigned int> statsPeriod;
    boost::optional<unsigned int> savePeriod;
    boost::optional<unsigned int> debugStatsPeriod;
    boost::optional<unsigned int> minimapPeriod;
    bool disableEventLogging = false;
    unsigned random_init = static_cast<unsigned>(std::chrono::high_resolution_clock::now().time_since_epoch().count());

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Show help")
        ("max_gf", po::value<unsigned>()->default_value(std::numeric_limits<unsigned>::max()),"Maximum number of game frames to run (optional)")
        ("output_path", po::value(&output_path),"Filename to write savegame to (optional)")
        ("stats_period", po::value(&statsPeriod),"Stats period")
        ("save_period", po::value(&savePeriod),"Save period")
        ("debug_stats_period", po::value(&debugStatsPeriod),"Debug stats period")
        ("minimap_period", po::value(&minimapPeriod),"Minimap save period")
        ("disable_event_logging", po::bool_switch(&disableEventLogging),
         "Disable event logger output (combat/building/ware/tool-priority)")
        ("map,m", po::value<std::string>()->required(),"Map to load")
        ("ai", po::value<std::vector<std::string>>()->required(),"AI player(s) to add")
        ("objective", po::value<std::string>()->default_value("none"),"none(default)|domination|conquer")
        ("weights_file", po::value<std::string>()->required(), "AI weights file")
        ("player_weights", po::value<std::vector<std::string>>()->multitoken(),
         "Per-player weights overrides (<player>=<file>)")
        ("start_wares", po::value<std::string>()->default_value("alot"),"Start wares")
        ("start_from_save", po::value(&start_save_path),"Path to savegame to load (optional)")
        ("replay", po::value(&replay_path),"Filename to write stats_interval to (optional)")
        ("random_init", po::value(&random_init),"Seed value for the random number generator (optional)")
        ("version", "Show version information and exit")
        ;

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
        STATS_CONFIG.outputPath = *output_path;
        if(!bfs::exists(*output_path))
            bfs::create_directories(*output_path);
        std::string statsDir = *output_path + "/stats/";
        bfs::create_directory(statsDir);
        STATS_CONFIG.statsPath = statsDir;
        std::string savesDir = *output_path + "/saves/";
        bfs::create_directory(savesDir);
        STATS_CONFIG.savesPath = savesDir;
        std::string minimapsDir = *output_path + "/minimaps/";
        bfs::create_directory(minimapsDir);
        STATS_CONFIG.screensPath = minimapsDir;

        std::string logsDir = *output_path + "/logs/";
        bfs::create_directory(logsDir);
        LOG.setLogFilepath(logsDir);

        for(int i = 0; i < argc; ++i)
            bnw::cout << argv[i] << " ";
        bnw::cout << std::endl;
        bnw::cout << "random_init: " << random_init << std::endl;
        bnw::cout << std::endl;

        RTTRCONFIG.Init();
        RANDOM.Init(random_init);

        const bfs::path mapPath = RTTRCONFIG.ExpandPath(options["map"].as<std::string>());
        boost::optional<bfs::path> startSavePath;
        if(start_save_path)
            startSavePath = RTTRCONFIG.ExpandPath(*start_save_path);
        const std::vector<AI::Info> ais = ParseAIOptions(options["ai"].as<std::vector<std::string>>());

        const auto weightsFile = options["weights_file"].as<std::string>();
        applyWeightsCfg(weightsFile);
        if(options.count("player_weights"))
        {
            const auto overrides = options["player_weights"].as<std::vector<std::string>>();
            for(const std::string& entry : overrides)
            {
                const auto eqPos = entry.find('=');
                if(eqPos == std::string::npos || eqPos == 0 || eqPos == entry.size() - 1)
                {
                    throw std::invalid_argument("Invalid player_weights entry: " + entry);
                }
                unsigned long parsedIdx = std::stoul(entry.substr(0, eqPos));
                if(parsedIdx == 0 || parsedIdx > MAX_PLAYERS)
                    throw std::out_of_range("player_weights index out of bounds: " + entry);
                const std::string path = entry.substr(eqPos + 1);
                ApplyPlayerWeightsCfg(static_cast<unsigned char>(parsedIdx - 1), path);
            }
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
        ggs.exploration = Exploration::Disabled;

        STATS_CONFIG.stats_period = statsPeriod.get_value_or(0);
        STATS_CONFIG.save_period = savePeriod.get_value_or(0);
        STATS_CONFIG.debug_stats_period = debugStatsPeriod.get_value_or(0);
        STATS_CONFIG.minimap_period = minimapPeriod.get_value_or(0);
        STATS_CONFIG.disableEventLogging = disableEventLogging;

        ggs.setSelection(AddonId::INEXHAUSTIBLE_MINES, 1);
        ggs.setSelection(AddonId::DEMOLITION_PROHIBITION, 2);
        ggs.setSelection(AddonId::MILITARY_HITPOINTS, 2);
        HeadlessGame game(ggs, mapPath, ais, startSavePath);
        if(replay_path)
            game.RecordReplay(*replay_path, random_init);


        game.Run(options["max_gf"].as<unsigned>());
        game.Close();

    } catch(const std::exception& e)
    {
        bnw::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
