// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dskSinglePlayer.h"
#include "ListDir.h"
#include "Loader.h"
#include "RttrConfig.h"
#include "Savegame.h"
#include "Settings.h"
#include "WindowManager.h"
#include "controls/ctrlButton.h"
#include "dskCampaignSelection.h"
#include "dskMainMenu.h"
#include "dskSelectMap.h"
#include "files.h"
#include "ingameWindows/iwConnecting.h"
#include "ingameWindows/iwMsgbox.h"
#include "ingameWindows/iwPlayReplay.h"
#include "ingameWindows/iwSave.h"
#include "network/CreateServerInfo.h"
#include "network/GameClient.h"

namespace bfs = boost::filesystem;

static CreateServerInfo createLocalGameInfo(const std::string& name)
{
    return CreateServerInfo(ServerType::Local, SETTINGS.server.localPort, name);
}

/** @class dskSinglePlayer
 *
 *  Klasse des Einzelspieler Desktops.
 */

dskSinglePlayer::dskSinglePlayer()
{
    RTTR_Assert(dskMenuBase::ID_FIRST_FREE <= 3);

    addTextButton(3, DrawPoint(115, 180), Extent(220, 22), TextureColor::Green2, _("Resume last game"), NormalFont);
    addTextButton(7, DrawPoint(115, 210), Extent(220, 22), TextureColor::Green2, _("Load game"), NormalFont);

    addTextButton(5, DrawPoint(115, 250), Extent(220, 22), TextureColor::Green2, std::string(_("Campaigns")),
                  NormalFont);
    addTextButton(6, DrawPoint(115, 280), Extent(220, 22), TextureColor::Green2, _("Unlimited Play"), NormalFont);

    addTextButton(4, DrawPoint(115, 320), Extent(220, 22), TextureColor::Green2, _("Play Replay"), NormalFont);

    addTextButton(8, DrawPoint(115, 390), Extent(220, 22), TextureColor::Red1, _("Back"), NormalFont);

    addImage(11, DrawPoint(20, 20), LOADER.getImageN("logo", 0));
}

void dskSinglePlayer::msgButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 3: // "Letztes Spiel fortsetzen"
        {
            const std::vector<bfs::path> savFiles = listDir(RTTRCONFIG.expandPath(s25::folders::save), "sav");

            bfs::path mostRecentFilepath;
            s25util::time64_t recent = 0;
            for(const auto& savFile : savFiles)
            {
                Savegame save;

                // Datei öffnen
                if(!save.load(savFile, SaveGameDataToLoad::Header))
                    continue;

                if(save.getSaveTime() > recent)
                {
                    recent = save.getSaveTime();
                    mostRecentFilepath = savFile;
                }
            }

            if(recent != 0)
            {
                // Dateiname noch rausextrahieren aus dem Pfad
                if(!mostRecentFilepath.has_filename())
                    return;
                const auto name = mostRecentFilepath.stem().string();

                // Server info
                CreateServerInfo csi = createLocalGameInfo(name);

                WINDOWMANAGER.switchTo(std::make_unique<dskSelectMap>(csi));

                if(GAMECLIENT.hostGame(csi, {mostRecentFilepath, MapType::Savegame}))
                    WINDOWMANAGER.showAfterSwitch(std::make_unique<iwConnecting>(csi.type, nullptr));
                else
                {
                    WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Error"),
                                                                  _("The specified file couldn't be loaded!"), nullptr,
                                                                  MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
                }
            } else
                WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Error"), _("The specified file couldn't be loaded!"),
                                                              nullptr, MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
        }
        break;
        case 4: // "Replay abspielen"
        {
            WINDOWMANAGER.toggleWindow(std::make_unique<iwPlayReplay>());
        }
        break;
        case 5: // "Kampagne"
        {
            WINDOWMANAGER.switchTo(std::make_unique<dskCampaignSelection>(createLocalGameInfo(_("Campaign"))));
        }
        break;
        case 6: // "Freies Spiel"
        {
            prepareSinglePlayerServer();
        }
        break;
        case 7: // "Spiel laden"
        {
            prepareLoadGame();
        }
        break;
        case 8: // "Zurück"
        {
            WINDOWMANAGER.switchTo(std::make_unique<dskMainMenu>());
        }
        break;
    }
}

void dskSinglePlayer::prepareSinglePlayerServer()
{
    WINDOWMANAGER.switchTo(std::make_unique<dskSelectMap>(createLocalGameInfo(_("Unlimited Play"))));
}

void dskSinglePlayer::prepareLoadGame()
{
    CreateServerInfo csi = createLocalGameInfo(_("Unlimited Play"));

    WINDOWMANAGER.switchTo(std::make_unique<dskSelectMap>(csi));
    WINDOWMANAGER.showAfterSwitch(std::make_unique<iwLoad>(csi));
}
