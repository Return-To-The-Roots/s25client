// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "QuickStartGame.h"
#include "Loader.h"
#include "MusicPlayer.h"
#include "RttrConfig.h"
#include "Settings.h"
#include "WindowManager.h"
#include "desktops/dskGameLoader.h"
#include "desktops/dskSelectMap.h"
#include "ingameWindows/iwConnecting.h"
#include "ingameWindows/iwMusicPlayer.h"
#include "network/ClientInterface.h"
#include "network/CreateServerInfo.h"
#include "network/GameClient.h"
#include "gameTypes/AIInfo.h"
#include "gameData/ApplicationLoader.h"
#include "s25util/Log.h"
#include "s25util/strAlgos.h"
#include <boost/filesystem.hpp>

class SwitchOnStart : public ClientInterface
{
public:
    SwitchOnStart() { GAMECLIENT.SetInterface(this); }
    ~SwitchOnStart() override { GAMECLIENT.RemoveInterface(this); }

    void CI_GameLoading(std::shared_ptr<Game> game) override
    {
        WINDOWMANAGER.Switch(std::make_unique<dskGameLoader>(std::move(game)));
    }
};

static bool ParseAIOptions(std::vector<AI::Info>& aiInfos, const std::vector<std::string>& aiArguments)
{
    aiInfos.clear();
    for(const std::string& aiArgument : aiArguments)
    {
        const auto aiArgument_lower = s25util::toLower(aiArgument);
        AI::Type type = AI::Type::Dummy;
        if(aiArgument_lower == "aijh")
            type = AI::Type::Default;
        else if(aiArgument_lower != "dummy")
        {
            LOG.write(_("Invalid AI player name: %1%\n")) % aiArgument_lower;
            return false;
        }

        aiInfos.push_back({type, AI::Level::Hard});
    }

    return true;
}

bool QuickStartGame(const boost::filesystem::path& mapOrReplayPath, const std::vector<std::string>& ais)
{
    if(!exists(mapOrReplayPath))
    {
        LOG.write(_("Given map or replay (%1%) does not exist!")) % mapOrReplayPath;
        return false;
    }

    std::vector<AI::Info> aiInfos;
    if(!ParseAIOptions(aiInfos, ais))
    {
        LOG.write(_("Could not parse AI player(s)"));
        return false;
    }

    ApplicationLoader loader(RTTRCONFIG, LOADER, LOG, SETTINGS.sound.playlist);
    if(!loader.load())
        return false;
    if(loader.getPlaylist())
        MUSICPLAYER.SetPlaylist(std::move(*loader.getPlaylist()));
    if(SETTINGS.sound.musicEnabled)
        MUSICPLAYER.Play();

    // An AI-battle is a single-player game.
    bool isSinglePlayer = !ais.empty();

    const CreateServerInfo csi(isSinglePlayer ? ServerType::Local : ServerType::Direct, SETTINGS.server.localPort,
                               _("Unlimited Play"));

    LOG.write(_("Loading game...\n"));
    const std::string extension = s25util::toLower(mapOrReplayPath.extension().string());

    WINDOWMANAGER.Switch(std::make_unique<dskSelectMap>(csi));

    if((extension == ".sav" && GAMECLIENT.HostGame(csi, mapOrReplayPath, MapType::Savegame))
       || ((extension == ".swd" || extension == ".wld") && GAMECLIENT.HostGame(csi, mapOrReplayPath, MapType::OldMap)))
    {
        GAMECLIENT.SetAIBattlePlayers(aiInfos);
        WINDOWMANAGER.ShowAfterSwitch(std::make_unique<iwConnecting>(csi.type, nullptr));
        return true;
    } else
    {
        SwitchOnStart switchOnStart;
        return GAMECLIENT.StartReplay(mapOrReplayPath);
    }
}
