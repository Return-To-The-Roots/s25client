// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "QuickStartGame.h"
#include "JoinPlayerInfo.h"
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

bool QuickStartGame(const boost::filesystem::path& mapOrReplayPath, const std::vector<std::string>& ais,
                    bool singlePlayer)
{
    if(!exists(mapOrReplayPath))
    {
        LOG.write(_("Given map or replay (%1%) does not exist!")) % mapOrReplayPath;
        return false;
    }

    ApplicationLoader loader(RTTRCONFIG, LOADER, LOG, SETTINGS.sound.playlist);
    if(!loader.load())
        return false;
    if(loader.getPlaylist())
        MUSICPLAYER.SetPlaylist(std::move(*loader.getPlaylist()));
    if(SETTINGS.sound.musicEnabled)
        MUSICPLAYER.Play();

    const CreateServerInfo csi(singlePlayer ? ServerType::Local : ServerType::Direct, SETTINGS.server.localPort,
                               _("Unlimited Play"));

    LOG.write(_("Loading game...\n"));
    const std::string extension = s25util::toLower(mapOrReplayPath.extension().string());

    WINDOWMANAGER.Switch(std::make_unique<dskSelectMap>(csi));

    static_cast<void>(ais);
    if(!ais.empty() && (extension == ".swd" || extension == ".wld")
       && GAMECLIENT.HostGame(csi, mapOrReplayPath, MapType::AIBattle))
    {
        std::vector<JoinPlayerInfo> playerInfos;
        for(unsigned playerId = 0; playerId < ais.size(); ++playerId)
        {
            JoinPlayerInfo pi;
            pi.ps = PlayerState::AI;
            pi.nation = Nation::Romans;
            pi.aiInfo.level = AI::Level::Hard;

            auto ai_lower = s25util::toLower(ais[playerId]);
            if(ai_lower == "aijh")
            {
                pi.aiInfo.type = AI::Type::Default;
            } else if(ai_lower == "dummy")
            {
                pi.aiInfo.type = AI::Type::Dummy;
            } else
            {
                LOG.write(_("Invalid AI player name: %1%\n")) % ais[playerId];
                return false;
            }

            pi.SetAIName(playerId);

            playerInfos.push_back(pi);
        }
        GAMECLIENT.SetAIBattlePlayers(std::move(playerInfos));

        WINDOWMANAGER.ShowAfterSwitch(std::make_unique<iwConnecting>(csi.type, nullptr));
        return true;
    } else if((extension == ".sav" && GAMECLIENT.HostGame(csi, mapOrReplayPath, MapType::Savegame))
              || ((extension == ".swd" || extension == ".wld")
                  && GAMECLIENT.HostGame(csi, mapOrReplayPath, MapType::OldMap)))
    {
        WINDOWMANAGER.ShowAfterSwitch(std::make_unique<iwConnecting>(csi.type, nullptr));
        return true;
    } else
    {
        SwitchOnStart switchOnStart;
        return GAMECLIENT.StartReplay(mapOrReplayPath);
    }
}
