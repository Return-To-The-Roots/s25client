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
    : Desktop(LOADER.getImageN(ResourceId(LOAD_SCREENS[rand() % LOAD_SCREENS.size()]), 0)), position(0),
      loader_(LOADER, std::move(game))
{
    WINDOWMANAGER.setCursor(Cursor::None);

    using namespace std::chrono_literals;
    addTimer(1, 50ms);

    addText(10, DrawPoint(800 / 2, 600 - 50), "", COLOR_YELLOW, FontStyle::CENTER, LargeFont);

    for(unsigned i = 0; i < 8; ++i)
        addText(11 + i, DrawPoint(30, 30 + i * 20), "", COLOR_GREEN, FontStyle{}, LargeFont);

    LOBBYCLIENT.addListener(this);
    GAMECLIENT.setInterface(this);
}

dskGameLoader::~dskGameLoader()
{
    WINDOWMANAGER.setCursor();
    LOBBYCLIENT.removeListener(this);
    GAMECLIENT.removeInterface(this);
}

void dskGameLoader::msgMsgBoxResult(const unsigned msgbox_id, const MsgboxResult /*mbr*/)
{
    if(msgbox_id == 0) // Verbindung zu Server verloren?
    {
        GAMECLIENT.stop();

        if(LOBBYCLIENT.isLoggedIn()) // steht die Lobbyverbindung noch?
            WINDOWMANAGER.switchTo(std::make_unique<dskLobby>());
        else if(loader_.getGame()->world_.isSinglePlayer())
            WINDOWMANAGER.switchTo(std::make_unique<dskSinglePlayer>());
        else
            WINDOWMANAGER.switchTo(std::make_unique<dskDirectIP>());
    }
}

void dskGameLoader::msgTimer(const unsigned /*ctrl_id*/)
{
    auto* timer = getCtrl<ctrlTimer>(1);
    auto* text = getCtrl<ctrlText>(10 + position);
    using namespace std::chrono_literals;
    const auto interval = 50ms;

    timer->stop();

    switch(position)
    {
        case 0: // Kartename anzeigen
            text->setText(GAMECLIENT.getMapTitle());
            break;

        case 1: // Karte geladen
            text->setText(_("Map was loaded and pinned at the wall..."));
            break;

        case 2: // Nationen ermitteln
            loader_.initNations();

            text->setText(_("Tribal chiefs assembled around the table..."));
            break;

        case 3: // Objekte laden
        {
            loader_.initTextures();
            if(!loader_.loadTextures())
            {
                showErrorMsg(_("Failed to load game resources"));
                return; // Don't restart timer!
            }

            text->setText(_("Game crate was picked and spread out..."));
            break;
        }
        case 4: // Welt erstellen
            try
            {
                // Do this here as it will init OGL
                gameInterface = std::make_unique<dskGameInterface>(loader_.getGame(), GAMECLIENT.getNWFInfo(),
                                                                   GAMECLIENT.getPlayerId());
            } catch(const std::runtime_error& e)
            {
                showErrorMsg(std::string(_("Failed to init GUI: ")) + e.what());
                return;
            }
            // TODO: richtige Messages senden, um das zu laden /*GetMap()->GenerateOpenGL();*/

            // We are done, wait for others
            GAMECLIENT.gameLoaded();

            text->setText(_("World was put together and glued..."));
            break;

        case 5: // nochmal text anzeigen
            text->setText(_("And let's go!"));
            text->setTextColor(COLOR_RED);
            return;
    }

    ++position;
    timer->start(interval);
}

void dskGameLoader::showErrorMsg(const std::string& error)
{
    auto wnd = std::make_unique<iwMsgbox>(_("Error"), error, this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed, 0);
    if(isActive())
        WINDOWMANAGER.show(std::move(wnd));
    else
        WINDOWMANAGER.showAfterSwitch(std::move(wnd));
    getCtrl<ctrlTimer>(1)->stop();
}

/**
 *  (Lobby-)Status: Benutzerdefinierter Fehler (kann auch Conn-Loss o.ä sein)
 */
void dskGameLoader::LC_Status_Error(const std::string& error)
{
    showErrorMsg(error);
}

void dskGameLoader::CI_GameStarted()
{
    RTTR_Assert(gameInterface);
    WINDOWMANAGER.switchTo(std::move(gameInterface));
}

void dskGameLoader::CI_Error(const ClientError ce)
{
    showErrorMsg(clientErrorToStr(ce));
}
