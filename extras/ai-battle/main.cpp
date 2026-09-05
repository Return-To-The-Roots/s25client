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
#include "gameTypes/TeamTypes.h"
#include "random/Random.h"
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

    optional<std::string> replay_path;
    optional<std::string> savegame_path;
    optional<std::string> lua_path;
    optional<std::string> settings_path;
    unsigned random_init = static_cast<unsigned>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    unsigned random_ai_init = random_init;

    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help,h", "Show help")
        ("map,m", po::value<std::string>()->required(),"Map to load")
        ("ai", po::value<std::vector<std::string>>()->required(),"AI player(s) to add (aijh | dummy)")
        ("teams", po::value<std::string>(),"Team assignment, e.g. \"0,1;2,3\" for a 2v2 (groups separated by ';', player indices by ','). Allied players get start pacts.")
        ("objective", po::value<std::string>()->default_value("domination"),"domination(default) | conquer")
        ("wares", po::value<std::string>()->default_value("normal"),"Starting wares: vlow | low | normal (default) | alot")
        ("settings", po::value(&settings_path),"INI file with an [addons] section to configure addon settings (optional)")
        ("replay", po::value(&replay_path),"Filename to write replay to (optional)")
        ("save", po::value(&savegame_path),"Filename to write savegame to (optional)")
        ("lua", po::value(&lua_path),"Lua script to execute during the game (optional)")
        ("random_init", po::value(&random_init),"Seed value for the random number generator (optional)")
        ("random_ai_init", po::value(&random_ai_init),"Seed value for the AI random number generator (optional)")
        ("maxGF", po::value<unsigned>()->default_value(std::numeric_limits<unsigned>::max()),"Maximum number of game frames to run (optional)")
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

    po::variables_map options;
    try
    {
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
        bnw::cout << "random_init: " << random_init << std::endl;
        bnw::cout << "random_ai_init: " << random_ai_init << std::endl;
        bnw::cout << std::endl;

        RTTRCONFIG.Init();
        RANDOM.Init(random_init);
        AI::getRandomGenerator().seed(random_ai_init);

        const bfs::path mapPath = RTTRCONFIG.ExpandPath(options["map"].as<std::string>());
        const std::vector<AI::Info> ais = ParseAIOptions(options["ai"].as<std::vector<std::string>>());

        GlobalGameSettings ggs;
        const auto objective = options["objective"].as<std::string>();
        if(objective == "domination")
            ggs.objective = GameObjective::TotalDomination;
        else if(objective == "conquer")
            ggs.objective = GameObjective::Conquer3_4;
        else
        {
            bnw::cerr << "unknown objective: " << objective << std::endl;
            return 1;
        }

        const auto wares = options["wares"].as<std::string>();
        if(wares == "vlow")
            ggs.startWares = StartWares::VLow;
        else if(wares == "low")
            ggs.startWares = StartWares::Low;
        else if(wares == "normal")
            ggs.startWares = StartWares::Normal;
        else if(wares == "alot")
            ggs.startWares = StartWares::ALot;
        else
        {
            bnw::cerr << "Unknown wares value: " << wares << std::endl;
            return 1;
        }

        if(settings_path)
        {
            loadAddonsFromIni(ggs, RTTRCONFIG.ExpandPath(*settings_path));

            bnw::cout << "settings: " << RTTRCONFIG.ExpandPath(*settings_path) << std::endl;
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
        if(options.count("teams"))
        {
            std::stringstream groups(options["teams"].as<std::string>());
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

        HeadlessGame game(ggs, mapPath, ais, lua_path ? RTTRCONFIG.ExpandPath(*lua_path) : bfs::path{}, teams);
        if(replay_path)
            game.RecordReplay(RTTRCONFIG.ExpandPath(*replay_path), random_init);

        game.Run(options["maxGF"].as<unsigned>());
        game.Close();
        if(savegame_path)
            game.SaveGame(RTTRCONFIG.ExpandPath(*savegame_path));
    } catch(const std::exception& e)
    {
        bnw::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
