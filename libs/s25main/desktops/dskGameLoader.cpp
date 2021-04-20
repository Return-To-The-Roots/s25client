// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dskGameLoader.h"
#include "Game.h"
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
#include <memory>
#include <utility>

/**
 *  Konstruktor von @p dskGameLoader.
 *  Startet das Spiel und lädt alles Notwendige.
 */
dskGameLoader::dskGameLoader(std::shared_ptr<Game> game)
    : Desktop(LOADER.GetImageN(ResourceId(LOAD_SCREENS[rand() % LOAD_SCREENS.size()]), 0)), position(0),
      loader_(LOADER, std::move(game))
{
    WINDOWMANAGER.SetCursor(Cursor::None);

    using namespace std::chrono_literals;
    AddTimer(1, 50ms);

    AddText(10, DrawPoint(800 / 2, 600 - 50), "", COLOR_YELLOW, FontStyle::CENTER, LargeFont);

    for(unsigned i = 0; i < 8; ++i)
        AddText(11 + i, DrawPoint(30, 30 + i * 20), "", COLOR_GREEN, FontStyle{}, LargeFont);

    LOBBYCLIENT.AddListener(this);
    GAMECLIENT.SetInterface(this);
}

dskGameLoader::~dskGameLoader()
{
    WINDOWMANAGER.SetCursor();
    LOBBYCLIENT.RemoveListener(this);
    GAMECLIENT.RemoveInterface(this);
}

void dskGameLoader::Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult /*mbr*/)
{
    if(msgbox_id == 0) // Verbindung zu Server verloren?
    {
        GAMECLIENT.Stop();

        if(LOBBYCLIENT.IsLoggedIn()) // steht die Lobbyverbindung noch?
            WINDOWMANAGER.Switch(std::make_unique<dskLobby>());
        else if(loader_.getGame()->world_.IsSinglePlayer())
            WINDOWMANAGER.Switch(std::make_unique<dskSinglePlayer>());
        else
            WINDOWMANAGER.Switch(std::make_unique<dskDirectIP>());
    }
}

void dskGameLoader::Msg_Timer(const unsigned /*ctrl_id*/)
{
    auto* timer = GetCtrl<ctrlTimer>(1);
    auto* text = GetCtrl<ctrlText>(10 + position);
    using namespace std::chrono_literals;
    const auto interval = 50ms;

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
            if(!loader_.loadTextures())
            {
                ShowErrorMsg(_("Failed to load game resources"));
                return; // Don't restart timer!
            }

            text->SetText(_("Game crate was picked and spread out..."));
            break;
        }
        case 4: // Welt erstellen
            try
            {
                // Do this here as it will init OGL
                gameInterface = std::make_unique<dskGameInterface>(loader_.getGame(), GAMECLIENT.GetNWFInfo(),
                                                                   GAMECLIENT.GetPlayerId());
            } catch(std::runtime_error& e)
            {
                ShowErrorMsg(std::string(_("Failed to init GUI: ")) + e.what());
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

void dskGameLoader::ShowErrorMsg(const std::string& error)
{
    WINDOWMANAGER.Show(
      std::make_unique<iwMsgbox>(_("Error"), error, this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed, 0));
    GetCtrl<ctrlTimer>(1)->Stop();
}

/**
 *  (Lobby-)Status: Benutzerdefinierter Fehler (kann auch Conn-Loss o.ä sein)
 */
void dskGameLoader::LC_Status_Error(const std::string& error)
{
    ShowErrorMsg(error);
}

void dskGameLoader::CI_GameStarted()
{
    RTTR_Assert(gameInterface);
    WINDOWMANAGER.Switch(std::move(gameInterface));
}

void dskGameLoader::CI_Error(const ClientError ce)
{
    ShowErrorMsg(ClientErrorToStr(ce));
}
