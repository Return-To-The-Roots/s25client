// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#include "defines.h" // IWYU pragma: keep
#include "dskGameLoader.h"

#include "Loader.h"
#include "WindowManager.h"
#include "GameClient.h"
#include "LobbyClient.h"
#include "GameManager.h"

#include "dskGameInterface.h"
#include "dskLobby.h"
#include "dskDirectIP.h"
#include "controls/ctrlText.h"
#include "controls/ctrlTimer.h"
#include "ingameWindows/iwMsgbox.h"
#include "ogl/glArchivItem_Font.h"
#include "files.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p dskGameLoader.
 *  Startet das Spiel und lädt alles Notwendige.
 *
 *  @author FloSoft
 */
dskGameLoader::dskGameLoader(GameWorldViewer* gwv)
    : Desktop(LOADER.GetImageN(FILE_LOAD_IDS[rand() % FILE_LOAD_IDS_COUNT], 0)),
      position(0), gwv(gwv)
{
    GAMEMANAGER.SetCursor(CURSOR_NONE);

    AddTimer(1, 50);

    AddText(10, 800 / 2, 600 - 50, "", COLOR_YELLOW, glArchivItem_Font::DF_CENTER, LargeFont);

    for(unsigned int i = 0; i < 8; ++i)
        AddText(11 + i, 30, 30 + i * 20, "", COLOR_GREEN, 0, LargeFont);

    LOBBYCLIENT.SetInterface(this);
    GAMECLIENT.SetInterface(this);
}

dskGameLoader::~dskGameLoader()
{
    GAMEMANAGER.SetCursor();
}

void dskGameLoader::Msg_MsgBoxResult(const unsigned int msgbox_id, const MsgboxResult  /*mbr*/)
{
    if(msgbox_id == 0) // Verbindung zu Server verloren?
    {
        GAMECLIENT.Stop();

        if(LOBBYCLIENT.LoggedIn()) // steht die Lobbyverbindung noch?
            WINDOWMANAGER.Switch(new dskLobby);
        else
            WINDOWMANAGER.Switch(new dskDirectIP);
    }
}

void dskGameLoader::Msg_Timer(const unsigned int  /*ctrl_id*/)
{
    static bool load_nations[NAT_COUNT];

    ctrlTimer* timer = GetCtrl<ctrlTimer>(1);
    ctrlText* text = GetCtrl<ctrlText>(10 + position);
    int interval = 500;

    timer->Stop();

    switch(position)
    {
        case 0: // Kartename anzeigen
        {
            text->SetText(GAMECLIENT.GetMapTitle());
            interval = 50;
        } break;

        case 1: // Karte geladen
        {
            text->SetText(_("Map was loaded and pinned at the wall..."));
            interval = 50;
        } break;

        case 2: // Nationen ermitteln
        {
            memset(load_nations, 0, sizeof(bool) * NAT_COUNT);
            for(unsigned char i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
                load_nations[GAMECLIENT.GetPlayer(i).nation] = true;

            text->SetText(_("Tribal chiefs assembled around the table..."));
            interval = 50;
        } break;

        case 3: // Objekte laden
        {
            if(!LOADER.LoadFilesAtGame(gwv->GetLandscapeType(), load_nations))
            {
                LC_Status_Error(_("Failed to load map objects."));
                return;
            }

            if(GAMECLIENT.GetGGS().isEnabled(AddonId::CATAPULT_GRAPHICS))
            {
                if(!LOADER.LoadFilesFromAddon(AddonId::CATAPULT_GRAPHICS))
                {
                    LC_Status_Error(_("Failed to load addon objects."));
                    return;
                }
            }

            LOADER.fillCaches();

            text->SetText(_("Game crate was picked and spread out..."));
            interval = 50;
        } break;

        case 4: // Welt erstellen
        {
            if(!LOADER.CreateTerrainTextures())
            {
                LC_Status_Error(_("Failed to load terrain data."));
                return;
            }

            // TODO: richtige Messages senden, um das zu laden /*GetMap()->GenerateOpenGL();*/

            text->SetText(_("World was put together and glued..."));
            interval = 50;
        } break;

        case 5: // nochmal text anzeigen
        {
            text->SetText(_("And let's go!"));
            text->SetColor(COLOR_RED);
            interval = 50;
        } break;

        case 6: // zum Spiel wechseln
        {
            WINDOWMANAGER.Switch(new dskGameInterface);
            return;
        } break;
    }

    ++position;
    timer->Start(interval);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  (Lobby-)Status: Benutzerdefinierter Fehler (kann auch Conn-Loss o.ä sein)
 *
 *  @author FloSoft
 */
void dskGameLoader::LC_Status_Error(const std::string& error)
{
    WINDOWMANAGER.Show(new iwMsgbox(_("Error"), error, this, MSB_OK, MSB_EXCLAMATIONRED, 0));
}
