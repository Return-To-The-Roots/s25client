// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#include "rttrDefines.h" // IWYU pragma: keep
#include "dskSinglePlayer.h"
#include "ListDir.h"
#include "Loader.h"
#include "RttrConfig.h"
#include "Savegame.h"
#include "WindowManager.h"
#include "controls/ctrlButton.h"
#include "dskMainMenu.h"
#include "dskSelectMap.h"
#include "files.h"
#include "ingameWindows/iwMsgbox.h"
#include "ingameWindows/iwPlayReplay.h"
#include "ingameWindows/iwPleaseWait.h"
#include "ingameWindows/iwSave.h"
#include "network/CreateServerInfo.h"
#include "network/GameClient.h"
#include <boost/filesystem.hpp>

/** @class dskSinglePlayer
 *
 *  Klasse des Einzelspieler Desktops.
 */

dskSinglePlayer::dskSinglePlayer()
{
    RTTR_Assert(dskMenuBase::ID_FIRST_FREE <= 3);

    AddTextButton(3, DrawPoint(115, 180), Extent(220, 22), TC_GREEN2, _("Resume last game"), NormalFont);
    AddTextButton(7, DrawPoint(115, 210), Extent(220, 22), TC_GREEN2, _("Load game"), NormalFont);

    AddTextButton(5, DrawPoint(115, 250), Extent(220, 22), TC_GREEN2, std::string(_("Campaign")) + " (" + _("Coming soon") + ")",
                  NormalFont)
      ->SetEnabled(false);
    AddTextButton(6, DrawPoint(115, 280), Extent(220, 22), TC_GREEN2, _("Unlimited Play"), NormalFont);

    AddTextButton(4, DrawPoint(115, 320), Extent(220, 22), TC_GREEN2, _("Play Replay"), NormalFont);

    AddTextButton(8, DrawPoint(115, 390), Extent(220, 22), TC_RED1, _("Back"), NormalFont);

    AddImage(11, DrawPoint(20, 20), LOADER.GetImageN("logo", 0));
}

void dskSinglePlayer::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 3: // "Letztes Spiel fortsetzen"
        {
            std::vector<std::string> savFiles = ListDir(RTTRCONFIG.ExpandPath(FILE_PATHS[85]), "sav");

            bfs::path path;
            s25util::time64_t recent = 0;
            for(std::vector<std::string>::iterator it = savFiles.begin(); it != savFiles.end(); ++it)
            {
                Savegame save;

                // Datei öffnen
                if(!save.Load(*it, false, false))
                    continue;

                if(save.GetSaveTime() > recent)
                {
                    recent = save.GetSaveTime();
                    path = *it;
                }
            }

            if(recent != 0)
            {
                // Dateiname noch rausextrahieren aus dem Pfad
                if(!path.has_filename())
                    return;
                bfs::path fileName = path.filename();

                // ".sav" am Ende weg
                RTTR_Assert(fileName.has_extension());
                fileName.replace_extension();

                // Server info
                CreateServerInfo csi;

                csi.gamename = fileName.string();
                csi.password = "localgame";
                csi.port = 3665;
                csi.type = ServerType::LOCAL;
                csi.ipv6 = false;
                csi.use_upnp = false;

                WINDOWMANAGER.Switch(new dskSelectMap(csi));

                if(GAMECLIENT.HostGame(csi, path.string(), MAPTYPE_SAVEGAME))
                    WINDOWMANAGER.ShowAfterSwitch(new iwPleaseWait);
                else
                {
                    WINDOWMANAGER.Show(
                      new iwMsgbox(_("Error"), _("The specified file couldn't be loaded!"), NULL, MSB_OK, MSB_EXCLAMATIONRED));
                }
            } else
                WINDOWMANAGER.Show(new iwMsgbox(_("Error"), _("The specified file couldn't be loaded!"), NULL, MSB_OK, MSB_EXCLAMATIONRED));
        }
        break;
        case 4: // "Replay abspielen"
        {
            WINDOWMANAGER.Show(new iwPlayReplay);
        }
        break;
        case 5: // "Kampagne"
        {
            /// @todo Hier dann Auswahl zwischen Kampagne(n) und "Freies Spiel"
            WINDOWMANAGER.Show(new iwMsgbox(_("Not available"), _("Please use \'Unlimited Play\' to create a Singleplayer game."), this,
                                            MSB_OK, MSB_EXCLAMATIONGREEN));
        }
        break;
        case 6: // "Freies Spiel"
        {
            PrepareSinglePlayerServer();
        }
        break;
        case 7: // "Spiel laden"
        {
            PrepareLoadGame();
        }
        break;
        case 8: // "Zurück"
        {
            WINDOWMANAGER.Switch(new dskMainMenu);
        }
        break;
    }
}

void dskSinglePlayer::PrepareSinglePlayerServer()
{
    CreateServerInfo csi;
    csi.gamename = _("Unlimited Play");
    csi.password = "localgame";
    csi.port = 3665;
    csi.type = ServerType::LOCAL;
    csi.ipv6 = false;
    csi.use_upnp = false;

    WINDOWMANAGER.Switch(new dskSelectMap(csi));
}

void dskSinglePlayer::PrepareLoadGame()
{
    CreateServerInfo csi;
    csi.gamename = _("Unlimited Play");
    csi.password = "localgame";
    csi.port = 3665;
    csi.type = ServerType::LOCAL;
    csi.ipv6 = false;
    csi.use_upnp = false;

    WINDOWMANAGER.Switch(new dskSelectMap(csi));
    WINDOWMANAGER.ShowAfterSwitch(new iwLoad(csi));
}
