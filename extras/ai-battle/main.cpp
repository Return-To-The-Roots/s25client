// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GlobalGameSettings.h"
#include "HeadlessGame.h"
#include "QuickStartGame.h"
#include "RTTR_Version.h"
#include "RttrConfig.h"
#include "addons/Addon.h"
#include "addons/AddonBool.h"
#include "addons/AddonList.h"
#include "addons/const_addons.h"
#include "ai/random.h"
#include "files.h"
#include "random/Random.h"
#include "gameTypes/TeamTypes.h"
#include "s25util/Log.h"
#include "s25util/StringConversion.h"
#include "s25util/System.h"

#include <boost/filesystem.hpp>
#include <boost/nowide/args.hpp>
#include <boost/nowide/filesystem.hpp>
#include <boost/nowide/iostream.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <iomanip>
#include <sstream>
#if BOOST_VERSION >= 109000
#    include <optional>
using std::optional;
#else
#    include <boost/optional.hpp>
using boost::optional;
#endif

namespace bnw = boost::nowide;
namespace bfs = boost::filesystem;
namespace po = boost::program_options;

static void loadAddonsFromIni(GlobalGameSettings& ggs, const bfs::path& iniPath)
{
    if(!bfs::exists(iniPath))
        throw std::runtime_error("Settings file not found: " + iniPath.string());

    boost::property_tree::ptree tree;
    boost::property_tree::read_ini(iniPath.string(), tree);

    if(tree.empty()) // empty file -> nothing to configure, that's fine
        return;

    // Anything else is intentional configuration, so surface mistakes as hard errors instead of
    // silently ignoring them (a mistyped section or key/value would otherwise go unnoticed).
    const auto addons = tree.get_child_optional("addons");
    if(!addons)
        throw std::runtime_error("No [addons] section in " + iniPath.string());

    unsigned loaded = 0;
    for(const auto& entry : *addons)
    {
        AddonId id{};
        unsigned value = 0;
        try
        {
            id = static_cast<AddonId>(s25util::fromStringClassic<unsigned>(entry.first));
            value = entry.second.get_value<unsigned>();
        } catch(const std::exception&)
        {
            throw std::runtime_error("Invalid addon entry '" + entry.first + "' in " + iniPath.string());
        }
        if(!ggs.getAddon(id)) // unknown/unsupported addon id
            throw std::runtime_error("Unknown addon id '" + entry.first + "' in " + iniPath.string());
        ggs.setSelection(id, value);
        ++loaded;
    }
    bnw::cout << "Loaded " << loaded << " addon settings from " << iniPath << '\n';
}

int main(int argc, char** argv)
{
    bnw::nowide_filesystem();
    bnw::args _(argc, argv);

    po::options_description desc("Allowed options");
    struct
    {
        std::string map;
        std::vector<std::string> ais;
        optional<std::string> teams;
        std::string objective, wares;
        optional<std::string> replay_path;
        optional<std::string> savegame_path;
        optional<std::string> lua_path;
        optional<std::string> settings_path;
        unsigned random_init =
          static_cast<unsigned>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        unsigned random_ai_init = random_init;
        unsigned maxGF = std::numeric_limits<unsigned>::max();
    } opts;
    // clang-format off
    desc.add_options()
        ("help,h", "Show help")
        ("map,m", po::value(&opts.map)->required(),"Map to load")
        ("ai", po::value(&opts.ais)->required(),"AI player(s) to add (aijh | dummy)")
        ("teams", po::value(&opts.teams),"Team assignment, e.g. \"0,1;2,3\" for a 2v2 (groups separated by ';', player indices by ','). Allied players get start pacts.")
        ("objective", po::value(&opts.objective)->default_value("domination"),"domination(default) | conquer")
        ("wares", po::value(&opts.wares)->default_value("normal"),"Starting wares: vlow | low | normal (default) | alot")
        ("settings", po::value(&opts.settings_path),"INI file with an [addons] section to configure addon settings (optional)")
        ("replay", po::value(&opts.replay_path),"Filename to write replay to (optional)")
        ("save", po::value(&opts.savegame_path),"Filename to write savegame to (optional)")
        ("lua", po::value(&opts.lua_path),"Lua script to execute during the game (optional)")
        ("random_init", po::value(&opts.random_init),"Seed value for the random number generator (optional)")
        ("random_ai_init", po::value(&opts.random_ai_init),"Seed value for the AI random number generator (optional)")
        ("maxGF", po::value(&opts.maxGF),"Maximum number of game frames to run (optional)")
        ("version", "Show version information and exit")
        ;
    // clang-format on

    const auto printHelp = [&](std::ostream& os) {
        os << desc
           << "\nNote: path arguments support the <RTTR_USERDATA> placeholder "
              "(game data folder: SAVES, REPLAYS, MAPS, PRESETS)."
           << std::endl;
    };

    if(argc == 1)
    {
        printHelp(bnw::cerr);
        return 1;
    }

    try
    {
        po::variables_map options;
        po::store(po::command_line_parser(argc, argv).options(desc).run(), options);

        if(options.count("help"))
        {
            printHelp(bnw::cout);
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
        printHelp(bnw::cerr);
        return 1;
    }

    try
    {
        // We print arguments and seed in order to be able to reproduce crashes.
        for(int i = 0; i < argc; ++i)
            bnw::cout << argv[i] << " ";
        bnw::cout << std::endl;
        bnw::cout << "random_init: " << opts.random_init << std::endl;
        bnw::cout << "random_ai_init: " << opts.random_ai_init << std::endl;
        bnw::cout << std::endl;

        RTTRCONFIG.Init();
        // Lua errors and the AI log to file; without this that is ./logs, and a missing folder there turned
        // every Lua error into "Could not open logs/... for writing"
        const bfs::path logDir = RTTRCONFIG.ExpandPath(s25::folders::logs);
        bfs::create_directories(logDir);
        LOG.setLogFilepath(logDir);
        RANDOM.Init(opts.random_init);
        AI::getRandomGenerator().seed(opts.random_ai_init);

        const bfs::path mapPath = RTTRCONFIG.ExpandPath(opts.map);
        const std::vector<AI::Info> ais = ParseAIOptions(opts.ais);

        GlobalGameSettings ggs;
        if(opts.objective == "domination")
            ggs.objective = GameObjective::TotalDomination;
        else if(opts.objective == "conquer")
            ggs.objective = GameObjective::Conquer3_4;
        else
        {
            bnw::cerr << "unknown objective: " << opts.objective << std::endl;
            return 1;
        }

        if(opts.wares == "vlow")
            ggs.startWares = StartWares::VLow;
        else if(opts.wares == "low")
            ggs.startWares = StartWares::Low;
        else if(opts.wares == "normal")
            ggs.startWares = StartWares::Normal;
        else if(opts.wares == "alot")
            ggs.startWares = StartWares::ALot;
        else
        {
            bnw::cerr << "Unknown wares value: " << opts.wares << std::endl;
            return 1;
        }

        if(opts.settings_path)
        {
            loadAddonsFromIni(ggs, RTTRCONFIG.ExpandPath(*opts.settings_path));

            bnw::cout << "settings: " << RTTRCONFIG.ExpandPath(*opts.settings_path) << std::endl;
            bnw::cout << "addon selections (non-default only):" << std::endl;
            for(unsigned i = 0; i < ggs.getNumAddons(); ++i)
            {
                unsigned status = 0;
                const Addon* addon = ggs.getAddon(i, status);
                if(addon && status != addon->getDefaultStatus())
                {
                    bnw::cout << "  [0x" << std::hex << std::setw(8) << std::setfill('0')
                              << static_cast<unsigned>(addon->getId()) << std::dec << "] " << addon->getName() << " = ";
                    if(const auto* listAddon = dynamic_cast<const AddonList*>(addon))
                        bnw::cout << listAddon->getOptionName(status);
                    else if(dynamic_cast<const AddonBool*>(addon))
                        bnw::cout << (status ? "True" : "False");
                    else
                        bnw::cout << status;
                    bnw::cout << std::endl;
                }
            }
        }

        // Team assignment, e.g. "0,1;2,3". Player index -> Team (Team1, Team2, ...).
        std::vector<Team> teams;
        if(opts.teams)
        {
            std::stringstream groups(*opts.teams);
            std::string group;
            unsigned teamIdx = 0;
            while(std::getline(groups, group, ';'))
            {
                const Team team = static_cast<Team>(static_cast<uint8_t>(Team::Team1) + teamIdx);
                std::stringstream members(group);
                std::string idx;
                while(std::getline(members, idx, ','))
                {
                    if(idx.empty())
                        continue;
                    const unsigned p = static_cast<unsigned>(std::stoul(idx));
                    if(p >= teams.size())
                        teams.resize(p + 1, Team::None);
                    teams[p] = team;
                }
                ++teamIdx;
            }
        }

        HeadlessGame game(ggs, mapPath, ais, opts.lua_path ? RTTRCONFIG.ExpandPath(*opts.lua_path) : bfs::path{},
                          teams);
        if(opts.replay_path)
            game.RecordReplay(RTTRCONFIG.ExpandPath(*opts.replay_path), opts.random_init);

        game.Run(opts.maxGF);
        game.Close();
        if(opts.savegame_path)
            game.SaveGame(RTTRCONFIG.ExpandPath(*opts.savegame_path));
    } catch(const std::exception& e)
    {
        bnw::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
