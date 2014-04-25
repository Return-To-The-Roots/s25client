// $Id: dskSinglePlayer.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "dskSinglePlayer.h"

#include "WindowManager.h"
#include "Loader.h"
#include "GameServer.h"
#include "ListDir.h"
#include "GameSavegame.h"

#include "dskMainMenu.h"
#include "dskSelectMap.h"
#include "iwPlayReplay.h"
#include "iwSave.h"
#include "iwMsgbox.h"
#include "iwPleaseWait.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/** @class dskSinglePlayer
 *
 *  Klasse des Einzelspieler Desktops.
 *
 *  @author OLiver
 */

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p dskSinglePlayer.
 *
 *  @author OLiver
 *  @author FloSoft
 */
dskSinglePlayer::dskSinglePlayer(void) : Desktop(LOADER.GetImageN("menu", 0))
{
    // Version
    AddVarText(0, 0, 600, _("Return To The Roots - v%s-%s"), COLOR_YELLOW, 0 | glArchivItem_Font::DF_BOTTOM, NormalFont, 2, GetWindowVersion(), GetWindowRevision());
    // URL
    AddText(1, 400, 600, _("http://www.siedler25.org"), COLOR_GREEN, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, NormalFont);
    // Copyright
    AddVarText(2, 800, 600, _("\xA9 2005 - %s Settlers Freaks"), COLOR_YELLOW, glArchivItem_Font::DF_RIGHT | glArchivItem_Font::DF_BOTTOM, NormalFont, 1, GetCurrentYear());

    // "Letztes Spiel fortsetzen"
    AddTextButton(3, 115, 180, 220, 22, TC_GREEN2, _("Resume last game"), NormalFont);
    // "Replay abspielen"
    AddTextButton(4, 115, 220, 220, 22, TC_GREEN2, _("Play Replay"), NormalFont);
    // "Kampagne"
    AddTextButton(5, 115, 260, 220, 22, TC_GREEN2, _("Campaign"), NormalFont);
    // "Freies Spiel"
    AddTextButton(6, 115, 290, 220, 22, TC_GREEN2, _("Unlimited Play"), NormalFont);
    // "Spiel laden"
    AddTextButton(7, 115, 320, 220, 22, TC_GREEN2, _("Load game"), NormalFont);
    // "Zurück"
    AddTextButton(8, 115, 360, 220, 22, TC_RED1, _("Back"), NormalFont);

    AddImage(11, 20, 20, LOADER.GetImageN("logo", 0));
}



void dskSinglePlayer::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 3: // "Letztes Spiel fortsetzen"
        {
            std::list<std::string> liste;
            std::string tmp = GetFilePath(FILE_PATHS[85]);

            tmp += "*.sav";
            ListDir(tmp.c_str(), false, NULL, NULL, &liste);

            std::string path;
            unser_time_t recent = 0;
            for(std::list<std::string>::iterator it = liste.begin(); it != liste.end(); ++it)
            {
                Savegame save;

                // Datei öffnen
                if (!save.Load(*it, false, false))
                    continue;

                if (save.save_time > recent)
                {
                    recent = save.save_time;
                    path = *it;
                }
            }

            if (recent != 0)
            {
                // Dateiname noch rausextrahieren aus dem Pfad
                size_t pos = path.find_last_of('/');
                if(pos == std::string::npos)
                    return;
                std::string extracted_filename = path.substr(pos + 1);

                // ".sav" am Ende weg
                assert(extracted_filename.length() >= 4);
                extracted_filename.erase(extracted_filename.length() - 4);

                // Server info
                CreateServerInfo csi;

                csi.gamename = extracted_filename;
                csi.password = "localgame";
                csi.port = 3665;
                csi.type = NP_LOCAL;
                csi.ipv6 = false;
                csi.use_upnp = false;

                WindowManager::inst().Switch(new dskSelectMap(csi));

                if(GAMESERVER.TryToStart(csi, path, MAPTYPE_SAVEGAME))
                {
                    WindowManager::inst().Draw();
                    WindowManager::inst().Show(new iwPleaseWait);
                }
                else
                {
                    WindowManager::inst().Show(new iwMsgbox(_("Error"), _("The specified file couldn't be loaded!"), this, MSB_OK, MSB_EXCLAMATIONRED));
                }
            }
            else
            {
                WindowManager::inst().Show(new iwMsgbox(_("Error"), _("The specified file couldn't be loaded!"), this, MSB_OK, MSB_EXCLAMATIONRED));
            }

            liste.clear();
        } break;
        case 4: // "Replay abspielen"
        {
            WindowManager::inst().Show(new iwPlayReplay);
        } break;
        case 5: // "Kampagne"
        {
            /// @todo Hier dann Auswahl zwischen Kampagne(n) und "Freies Spiel"
            WindowManager::inst().Show(new iwMsgbox(_("Not available"), _("Please use \'Unlimited Play\' to create a Singleplayer game."), this, MSB_OK, MSB_EXCLAMATIONGREEN));
        } break;
        case 6: // "Freies Spiel"
        {
            PrepareSinglePlayerServer();
        } break;
        case 7: // "Spiel laden"
        {
            PrepareLoadGame();
        } break;
        case 8: // "Zurück"
        {
            WindowManager::inst().Switch(new dskMainMenu);
        } break;
    }
}

void dskSinglePlayer::PrepareSinglePlayerServer()
{
    CreateServerInfo csi;
    csi.gamename = _("Unlimited Play");
    csi.password = "localgame";
    csi.port = 3665;
    csi.type = NP_LOCAL;
    csi.ipv6 = false;
    csi.use_upnp = false;

    WindowManager::inst().Switch(new dskSelectMap(csi));
}

void dskSinglePlayer::PrepareLoadGame()
{
    CreateServerInfo csi;
    csi.gamename = _("Unlimited Play");
    csi.password = "localgame";
    csi.port = 3665;
    csi.type = NP_LOCAL;
    csi.ipv6 = false;
    csi.use_upnp = false;

    WindowManager::inst().Switch(new dskSelectMap(csi));
    WindowManager::inst().Draw();
    WindowManager::inst().Show(new iwLoad(csi));
}
