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
#include "dskGameLoader.h"
#include "Game.h"
#include "GameManager.h"
#include "Loader.h"
#include "WindowManager.h"
#include "controls/ctrlText.h"
#include "controls/ctrlTimer.h"
#include "dskDirectIP.h"
#include "dskGameInterface.h"
#include "dskLobby.h"
#include "dskSinglePlayer.h"
#include "files.h"
#include "ingameWindows/iwMsgbox.h"
#include "network/GameClient.h"
#include "ogl/FontStyle.h"
#include "liblobby/LobbyClient.h"

/**
 *  Konstruktor von @p dskGameLoader.
 *  Startet das Spiel und lädt alles Notwendige.
 */
dskGameLoader::dskGameLoader(std::shared_ptr<Game> game)
    : Desktop(LOADER.GetImageN(LOAD_SCREENS[rand() % LOAD_SCREENS.size()], 0)), position(0), loader_(game)
{
    GAMEMANAGER.SetCursor(CURSOR_NONE);

    AddTimer(1, 50);

    AddText(10, DrawPoint(800 / 2, 600 - 50), "", COLOR_YELLOW, FontStyle::CENTER, LargeFont);

    for(unsigned i = 0; i < 8; ++i)
        AddText(11 + i, DrawPoint(30, 30 + i * 20), "", COLOR_GREEN, 0, LargeFont);

    LOBBYCLIENT.AddListener(this);
    GAMECLIENT.SetInterface(this);
}

dskGameLoader::~dskGameLoader()
{
    GAMEMANAGER.SetCursor();
    LOBBYCLIENT.RemoveListener(this);
    GAMECLIENT.RemoveInterface(this);
}

void dskGameLoader::Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult /*mbr*/)
{
    if(msgbox_id == 0) // Verbindung zu Server verloren?
    {
        GAMECLIENT.Stop();

        if(LOBBYCLIENT.IsLoggedIn()) // steht die Lobbyverbindung noch?
            WINDOWMANAGER.Switch(new dskLobby);
        else if(loader_.getGame()->world.IsSinglePlayer())
            WINDOWMANAGER.Switch(new dskSinglePlayer);
        else
            WINDOWMANAGER.Switch(new dskDirectIP);
    }
}

void dskGameLoader::Msg_Timer(const unsigned /*ctrl_id*/)
{
    ctrlTimer* timer = GetCtrl<ctrlTimer>(1);
    ctrlText* text = GetCtrl<ctrlText>(10 + position);
    int interval = 50;

    timer->Stop();

    switch(position)
    {
        case 0: // Kartename anzeigen
            text->SetText(GAMECLIENT.GetMapTitle());
            break;

        case 1: // Karte geladen
            text->SetText(_("Map was loaded and pinned at the wall..."));
            break;

        case 2: // Nationen ermitteln
            loader_.initNations();

            text->SetText(_("Tribal chiefs assembled around the table..."));
            break;

        case 3: // Objekte laden
        {
            loader_.initTextures();
            loader_.loadTextures();

            text->SetText(_("Game crate was picked and spread out..."));
            break;
        }
        case 4: // Welt erstellen
            try
            {
                // Do this here as it will init OGL
                gameInterface.reset(new dskGameInterface(loader_.getGame(), GAMECLIENT.GetNWFInfo(), GAMECLIENT.GetPlayerId()));
            } catch(std::runtime_error& e)
            {
                LC_Status_Error(std::string(_("Failed to init GUI: ")) + e.what());
                return;
            }
            // TODO: richtige Messages senden, um das zu laden /*GetMap()->GenerateOpenGL();*/

            // We are done, wait for others
            GAMECLIENT.GameLoaded();

            text->SetText(_("World was put together and glued..."));
            break;

        case 5: // nochmal text anzeigen
            text->SetText(_("And let's go!"));
            text->SetTextColor(COLOR_RED);
            return;
    }

    ++position;
    timer->Start(interval);
}

/**
 *  (Lobby-)Status: Benutzerdefinierter Fehler (kann auch Conn-Loss o.ä sein)
 */
void dskGameLoader::LC_Status_Error(const std::string& error)
{
    WINDOWMANAGER.Show(new iwMsgbox(_("Error"), error, this, MSB_OK, MSB_EXCLAMATIONRED, 0));
    GetCtrl<ctrlTimer>(1)->Stop();
}

void dskGameLoader::CI_GameStarted(std::shared_ptr<Game>)
{
    RTTR_Assert(gameInterface);
    WINDOWMANAGER.Switch(gameInterface.release());
}

void dskGameLoader::CI_Error(const ClientError ce)
{
    WINDOWMANAGER.Show(new iwMsgbox(_("Error"), ClientErrorToStr(ce), this, MSB_OK, MSB_EXCLAMATIONRED, 0));
    GetCtrl<ctrlTimer>(1)->Stop();
}
