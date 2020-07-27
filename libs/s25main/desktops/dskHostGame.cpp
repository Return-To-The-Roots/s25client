// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "dskHostGame.h"
#include "GameLobby.h"
#include "GameLobbyController.h"
#include "ILobbyClient.hpp"
#include "JoinPlayerInfo.h"
#include "Loader.h"
#include "WindowManager.h"
#include "animation/BlinkButtonAnim.h"
#include "controls/ctrlBaseColor.h"
#include "controls/ctrlChat.h"
#include "controls/ctrlCheck.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlGroup.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlPreviewMinimap.h"
#include "controls/ctrlText.h"
#include "controls/ctrlTextButton.h"
#include "controls/ctrlVarDeepening.h"
#include "desktops/dskDirectIP.h"
#include "desktops/dskGameLoader.h"
#include "desktops/dskLAN.h"
#include "desktops/dskLobby.h"
#include "desktops/dskSinglePlayer.h"
#include "helpers/containerUtils.h"
#include "helpers/format.hpp"
#include "ingameWindows/iwAddons.h"
#include "ingameWindows/iwMsgbox.h"
#include "lua/LuaInterfaceSettings.h"
#include "network/GameClient.h"
#include "ogl/FontStyle.h"
#include "ogl/glArchivItem_Map.h"
#include "gameData/GameConsts.h"
#include "gameData/const_gui_ids.h"
#include "liblobby/LobbyPlayerInfo.h"
#include "libsiedler2/ErrorCodes.h"
#include "libsiedler2/prototypen.h"
#include "s25util/Log.h"
#include "s25util/MyTime.h"
#include <memory>
#include <mygettext/mygettext.h>
#include <set>

namespace {
enum CtrlIds
{
    ID_PLAYER_GROUP_START = 50,
    ID_SWAP_BUTTON = 80,
    ID_FIRST_FREE = ID_SWAP_BUTTON + MAX_PLAYERS,
    ID_GAME_CHAT,
    ID_LOBBY_CHAT,
    ID_CHAT_INPUT,
    ID_CHAT_TAB,
    TAB_GAMECHAT,
    TAB_LOBBYCHAT
};
}

dskHostGame::dskHostGame(ServerType serverType, const std::shared_ptr<GameLobby>& gameLobby, unsigned playerId,
                         std::unique_ptr<ILobbyClient> lobbyClient)
    : Desktop(LOADER.GetImageN("setup015", 0)), serverType(serverType), gameLobby(gameLobby), localPlayerId_(playerId),
      lobbyClient_(std::move(lobbyClient)), hasCountdown_(false), wasActivated(false), gameChat(nullptr), lobbyChat(nullptr),
      lobbyChatTabAnimId(0), localChatTabAnimId(0)
{
    if(gameLobby->isHost())
        lobbyHostController = std::make_unique<GameLobbyController>(gameLobby, GAMECLIENT.GetMainPlayer());

    if(!GAMECLIENT.GetLuaFilePath().empty() && gameLobby->isHost())
    {
        lua = std::make_unique<LuaInterfaceSettings>(*lobbyHostController, GAMECLIENT);
        if(!lua->loadScript(GAMECLIENT.GetLuaFilePath()))
        {
            WINDOWMANAGER.ShowAfterSwitch(
              std::make_unique<iwMsgbox>(_("Error"), _("Lua script was found but failed to load. Map might not work as expected!"), this,
                                         MSB_OK, MSB_EXCLAMATIONRED, 1));
            lua.reset();
        } else if(!lua->CheckScriptVersion())
        {
            WINDOWMANAGER.ShowAfterSwitch(std::make_unique<iwMsgbox>(
              _("Error"), _("Lua script uses a different version and cannot be used. Map might not work as expected!"), this, MSB_OK,
              MSB_EXCLAMATIONRED, 1));
            lua.reset();
        } else if(!lua->EventSettingsInit(serverType == ServerType::LOCAL, gameLobby->isSavegame()))
        {
            RTTR_Assert(gameLobby->isHost()); // This should be done first for the host so others won't even see the script
            LOG.write(_("Lua was disabled by the script itself\n"));
            lua.reset();
        }
        if(!lua)
            lobbyHostController->RemoveLuaScript();
    }

    const bool readonlySettings = !gameLobby->isHost() || gameLobby->isSavegame() || (lua && !lua->IsChangeAllowed("general"));
    allowAddonChange =
      gameLobby->isHost() && !gameLobby->isSavegame() && (!lua || lua->IsChangeAllowed("addonsAll") || lua->IsChangeAllowed("addonsSome"));

    // Kartenname
    AddText(0, DrawPoint(400, 5), GAMECLIENT.GetGameName(), COLOR_YELLOW, FontStyle::CENTER, LargeFont);

    // "Spielername"
    AddText(10, DrawPoint(125, 40), _("Player Name"), COLOR_YELLOW, FontStyle::CENTER, NormalFont);
    // "Volk"
    AddText(12, DrawPoint(285, 40), _("Race"), COLOR_YELLOW, FontStyle::CENTER, NormalFont);
    // "Farbe"
    AddText(13, DrawPoint(355, 40), _("Color"), COLOR_YELLOW, FontStyle::CENTER, NormalFont);
    // "Team"
    AddText(14, DrawPoint(405, 40), _("Team"), COLOR_YELLOW, FontStyle::CENTER, NormalFont);

    if(!IsSinglePlayer())
    {
        // "Bereit"
        AddText(15, DrawPoint(465, 40), _("Ready?"), COLOR_YELLOW, FontStyle::CENTER, NormalFont);
        // "Ping"
        AddText(16, DrawPoint(515, 40), _("Ping"), COLOR_YELLOW, FontStyle::CENTER, NormalFont);
    }
    // "Swap"
    if(gameLobby->isHost() && !gameLobby->isSavegame())
        AddText(24, DrawPoint(0, 40), _("Swap"), COLOR_YELLOW, FontStyle::LEFT, NormalFont);
    // "Verschieben" (nur bei Savegames!)
    if(gameLobby->isSavegame())
        AddText(17, DrawPoint(645, 40), _("Past player"), COLOR_YELLOW, FontStyle::CENTER, NormalFont);

    if(!IsSinglePlayer())
    {
        // Enable lobby chat when we are logged in
        if(lobbyClient_ && lobbyClient_->IsLoggedIn())
        {
            ctrlOptionGroup* chatTab = AddOptionGroup(ID_CHAT_TAB, ctrlOptionGroup::CHECK);
            chatTab->AddTextButton(TAB_GAMECHAT, DrawPoint(20, 320), Extent(178, 22), TC_GREEN2, _("Game Chat"), NormalFont);
            chatTab->AddTextButton(TAB_LOBBYCHAT, DrawPoint(202, 320), Extent(178, 22), TC_GREEN2, _("Lobby Chat"), NormalFont);
            gameChat = AddChatCtrl(ID_GAME_CHAT, DrawPoint(20, 345), Extent(360, 218 - 25), TC_GREY, NormalFont);
            lobbyChat = AddChatCtrl(ID_LOBBY_CHAT, DrawPoint(20, 345), Extent(360, 218 - 25), TC_GREY, NormalFont);
            chatTab->SetSelection(TAB_GAMECHAT, true);
        } else
        {
            // Chatfenster
            gameChat = AddChatCtrl(ID_GAME_CHAT, DrawPoint(20, 320), Extent(360, 218), TC_GREY, NormalFont);
        }
        // Edit für Chatfenster
        AddEdit(ID_CHAT_INPUT, DrawPoint(20, 540), Extent(360, 22), TC_GREY, NormalFont);
    }

    // "Spiel starten"
    AddTextButton(2, DrawPoint(600, 560), Extent(180, 22), TC_GREEN2, (gameLobby->isHost() ? _("Start game") : _("Ready")), NormalFont);

    // "Zurück"
    AddTextButton(3, DrawPoint(400, 560), Extent(180, 22), TC_RED1, _("Return"), NormalFont);

    // "Teams sperren"
    AddCheckBox(20, DrawPoint(400, 460), Extent(180, 26), TC_GREY, _("Lock teams:"), NormalFont, readonlySettings);
    // "Gemeinsame Team-Sicht"
    AddCheckBox(19, DrawPoint(600, 460), Extent(180, 26), TC_GREY, _("Shared team view"), NormalFont, readonlySettings);
    // "Random Start Locations"
    AddCheckBox(23, DrawPoint(600, 430), Extent(180, 26), TC_GREY, _("Random start locations"), NormalFont, readonlySettings);

    // "Enhancements"
    AddText(21, DrawPoint(400, 499), _("Addons:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddTextButton(22, DrawPoint(600, 495), Extent(180, 22), TC_GREEN2, allowAddonChange ? _("Change Settings...") : _("View Settings..."),
                  NormalFont);

    ctrlComboBox* combo;

    // umgedrehte Reihenfolge, damit die Listen nicht dahinter sind

    // "Aufklärung"
    AddText(30, DrawPoint(400, 405), _("Exploration:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo = AddComboBox(40, DrawPoint(600, 400), Extent(180, 20), TC_GREY, NormalFont, 100, readonlySettings);
    combo->AddString(_("Off (all visible)"));
    combo->AddString(_("Classic (Settlers 2)"));
    combo->AddString(_("Fog of War"));
    combo->AddString(_("FoW - all explored"));

    // "Waren zu Beginn"
    AddText(31, DrawPoint(400, 375), _("Goods at start:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo = AddComboBox(41, DrawPoint(600, 370), Extent(180, 20), TC_GREY, NormalFont, 100, readonlySettings);
    combo->AddString(_("Very Low"));
    combo->AddString(_("Low"));
    combo->AddString(_("Normal"));
    combo->AddString(_("A lot"));

    // "Spielziel"
    AddText(32, DrawPoint(400, 345), _("Goals:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo = AddComboBox(42, DrawPoint(600, 340), Extent(180, 20), TC_GREY, NormalFont, 100, readonlySettings);
    combo->AddString(_("None"));               // Kein Spielziel
    combo->AddString(_("Conquer 3/4 of map")); // Besitz 3/4 des Landes
    combo->AddString(_("Total domination"));   // Alleinherrschaft
    // Lobby game?
    if(lobbyClient_ && lobbyClient_->IsLoggedIn())
    {
        // Then add tournament modes as possible "objectives"
        for(unsigned i = 0; i < NUM_TOURNAMENT_MODESS; ++i)
        {
            combo->AddString(helpers::format(_("Tournament: %u minutes"), TOURNAMENT_MODES_DURATION[i]));
        }
    }

    // "Geschwindigkeit"
    AddText(33, DrawPoint(400, 315), _("Speed:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo = AddComboBox(43, DrawPoint(600, 310), Extent(180, 20), TC_GREY, NormalFont, 100, !gameLobby->isHost());
    combo->AddString(_("Very slow")); // Sehr Langsam
    combo->AddString(_("Slow"));      // Langsam
    combo->AddString(_("Normal"));    // Normal
    combo->AddString(_("Fast"));      // Schnell
    combo->AddString(_("Very fast")); // Sehr Schnell

    // Karte laden, um Kartenvorschau anzuzeigen
    if(!gameLobby->isSavegame())
    {
        // Map laden
        libsiedler2::Archiv mapArchiv;
        // Karteninformationen laden
        if(int ec = libsiedler2::loader::LoadMAP(GAMECLIENT.GetMapPath(), mapArchiv))
        {
            WINDOWMANAGER.ShowAfterSwitch(std::make_unique<iwMsgbox>(
              _("Error"), _("Could not load map:\n") + libsiedler2::getErrorString(ec), this, MSB_OK, MSB_EXCLAMATIONRED, 0));
        } else
        {
            auto* map = static_cast<glArchivItem_Map*>(mapArchiv.get(0));
            ctrlPreviewMinimap* preview = AddPreviewMinimap(70, DrawPoint(560, 40), Extent(220, 220), map);

            // Titel der Karte, Y-Position relativ je nach Höhe der Minimap festlegen, daher nochmals danach
            // verschieben, da diese Position sonst skaliert wird!
            ctrlText* text =
              AddText(71, DrawPoint(670, 0), _("Map: ") + GAMECLIENT.GetMapTitle(), COLOR_YELLOW, FontStyle::CENTER, NormalFont);
            text->SetPos(DrawPoint(text->GetPos().x, preview->GetPos().y + preview->GetMapArea().bottom + 10));
        }
    }

    if(IsSinglePlayer() && !gameLobby->isSavegame())
    {
        // Setze initial auf KI
        for(unsigned char i = 0; i < gameLobby->getNumPlayers(); i++)
        {
            if(!gameLobby->getPlayer(i).isHost)
                lobbyHostController->SetPlayerState(i, PS_AI, AI::Info(AI::DEFAULT, AI::EASY));
        }
    }

    // Alle Spielercontrols erstellen
    for(unsigned i = 0; i < gameLobby->getNumPlayers(); i++)
        UpdatePlayerRow(i);
    // swap buttons erstellen
    if(gameLobby->isHost() && !gameLobby->isSavegame() && (!lua || lua->IsChangeAllowed("swapping")))
    {
        for(unsigned i = 0; i < gameLobby->getNumPlayers(); i++)
        {
            DrawPoint rowPos = GetCtrl<Window>(ID_PLAYER_GROUP_START + i)->GetCtrl<Window>(1)->GetPos();
            ctrlButton* bt = AddTextButton(ID_SWAP_BUTTON + i, DrawPoint(5, 0), Extent(22, 22), TC_RED1, _("-"), NormalFont);
            bt->SetPos(DrawPoint(bt->GetPos().x, rowPos.y));
        }
    }
    CI_GGSChanged(gameLobby->getSettings());

    if(serverType == ServerType::LOBBY && lobbyClient_ && lobbyClient_->IsLoggedIn())
    {
        lobbyClient_->AddListener(this);
        lobbyClient_->SendServerJoinRequest();
    }

    GAMECLIENT.SetInterface(this);
}

dskHostGame::~dskHostGame()
{
    if(lobbyClient_)
        lobbyClient_->RemoveListener(this);
    GAMECLIENT.RemoveInterface(this);
}

/**
 *  Größe ändern-Reaktionen die nicht vom Skaling-Mechanismus erfasst werden.
 */
void dskHostGame::Resize(const Extent& newSize)
{
    Window::Resize(newSize);

    // Text unter der PreviewMinimap verschieben, dessen Höhe von der Höhe der
    // PreviewMinimap abhängt, welche sich gerade geändert hat.
    auto* preview = GetCtrl<ctrlPreviewMinimap>(70);
    auto* text = GetCtrl<ctrlText>(71);
    if(preview && text)
    {
        DrawPoint txtPos = text->GetPos();
        txtPos.y = preview->GetPos().y + preview->GetMapArea().bottom + 10;
        text->SetPos(txtPos);
    }
}

void dskHostGame::SetActive(bool activate /*= true*/)
{
    Desktop::SetActive(activate);
    if(activate && !wasActivated && lua && gameLobby->isHost())
    {
        wasActivated = true;
        try
        {
            lua->EventSettingsReady();
        } catch(LuaExecutionError&)
        {
            WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Error"),
                                                          _("Lua script was found but failed to load. Map might not work as expected!"),
                                                          this, MSB_OK, MSB_EXCLAMATIONRED, 1));
            lua.reset();
        }
    }
}

void dskHostGame::UpdatePlayerRow(const unsigned row)
{
    const JoinPlayerInfo& player = gameLobby->getPlayer(row);

    unsigned cy = 80 + row * 30;
    TextureColor tc = (row & 1 ? TC_GREY : TC_GREEN2);

    // Alle Controls erstmal zerstören (die ganze Gruppe)
    DeleteCtrl(ID_PLAYER_GROUP_START + row);
    // und neu erzeugen
    ctrlGroup* group = AddGroup(ID_PLAYER_GROUP_START + row);

    std::string name;
    // Name
    switch(player.ps)
    {
        default: name.clear(); break;
        case PS_OCCUPIED:
        case PS_AI: name = player.name; break;
        case PS_FREE: name = _("Open"); break;
        case PS_LOCKED: name = _("Closed"); break;
    }

    if(GetCtrl<ctrlPreviewMinimap>(70))
    {
        if(player.isUsed())
            // Nur KIs und richtige Spieler haben eine Farbe auf der Karte
            GetCtrl<ctrlPreviewMinimap>(70)->SetPlayerColor(row, player.color);
        else
            // Keine richtigen Spieler --> Startposition auf der Karte ausblenden
            GetCtrl<ctrlPreviewMinimap>(70)->SetPlayerColor(row, 0);
    }

    // Spielername, beim Hosts Spielerbuttons, aber nich beim ihm selber, er kann sich ja nich selber kicken!
    if(gameLobby->isHost() && !player.isHost && (!lua || lua->IsChangeAllowed("playerState")))
        group->AddTextButton(1, DrawPoint(30, cy), Extent(200, 22), tc, name, NormalFont);
    else
        group->AddTextDeepening(1, DrawPoint(30, cy), Extent(200, 22), tc, name, NormalFont, COLOR_YELLOW);
    auto* text = group->GetCtrl<ctrlBaseText>(1);

    // Is das der Host? Dann farblich markieren
    if(player.isHost)
        text->SetTextColor(0xFF00FF00);

    // Bei geschlossenem nicht sichtbar
    if(player.isUsed())
    {
        // If not in savegame -> Player can change own row and host can change AIs
        const bool allowPlayerChange = ((gameLobby->isHost() && player.ps == PS_AI) || localPlayerId_ == row) && !gameLobby->isSavegame();
        bool allowNationChange = allowPlayerChange;
        bool allowColorChange = allowPlayerChange;
        bool allowTeamChange = allowPlayerChange;
        if(lua)
        {
            if(localPlayerId_ == row)
            {
                allowNationChange &= lua->IsChangeAllowed("ownNation", true);
                allowColorChange &= lua->IsChangeAllowed("ownColor", true);
                allowTeamChange &= lua->IsChangeAllowed("ownTeam", true);
            } else
            {
                allowNationChange &= lua->IsChangeAllowed("aiNation", true);
                allowColorChange &= lua->IsChangeAllowed("aiColor", true);
                allowTeamChange &= lua->IsChangeAllowed("aiTeam", true);
            }
        }

        if(allowNationChange)
            group->AddTextButton(3, DrawPoint(240, cy), Extent(90, 22), tc, _(NationNames[0]), NormalFont);
        else
            group->AddTextDeepening(3, DrawPoint(240, cy), Extent(90, 22), tc, _(NationNames[0]), NormalFont, COLOR_YELLOW);

        if(allowColorChange)
            group->AddColorButton(4, DrawPoint(340, cy), Extent(30, 22), tc, 0);
        else
            group->AddColorDeepening(4, DrawPoint(340, cy), Extent(30, 22), tc, 0);

        if(allowTeamChange)
            group->AddTextButton(5, DrawPoint(380, cy), Extent(50, 22), tc, _("-"), NormalFont);
        else
            group->AddTextDeepening(5, DrawPoint(380, cy), Extent(50, 22), tc, _("-"), NormalFont, COLOR_YELLOW);

        // Bereit (nicht bei KIs und Host)
        if(player.ps == PS_OCCUPIED && !player.isHost)
            group->AddCheckBox(6, DrawPoint(450, cy), Extent(22, 22), tc, "", nullptr, (localPlayerId_ != row));

        // Ping ( "%d" )
        ctrlVarDeepening* ping =
          group->AddVarDeepening(7, DrawPoint(490, cy), Extent(50, 22), tc, _("%d"), NormalFont, COLOR_YELLOW, 1, &player.ping); //-V111

        // Verschieben (nur bei Savegames und beim Host!)
        if(gameLobby->isSavegame() && player.ps == PS_OCCUPIED)
        {
            ctrlComboBox* combo = group->AddComboBox(8, DrawPoint(560, cy), Extent(160, 22), tc, NormalFont, 150, !gameLobby->isHost());

            // Mit den alten Namen füllen
            for(unsigned i = 0; i < gameLobby->getNumPlayers(); ++i)
            {
                if(!gameLobby->getPlayer(i).originName.empty())
                {
                    combo->AddString(gameLobby->getPlayer(i).originName);
                    if(i == row)
                        combo->SetSelection(combo->GetNumItems() - 1);
                }
            }
        }

        // Hide ping for AIs or on single player games
        if(player.ps == PS_AI || IsSinglePlayer())
            ping->SetVisible(false);

        // Felder ausfüllen
        ChangeNation(row, player.nation);
        ChangeTeam(row, player.team);
        ChangePing(row);
        ChangeReady(row, player.isReady);
        ChangeColor(row, player.color);
    }
    group->SetActive(IsActive());
}

/**
 *  Methode vor dem Zeichnen
 */
void dskHostGame::Msg_PaintBefore()
{
    Desktop::Msg_PaintBefore();
    // Chatfenster Fokus geben
    if(!IsSinglePlayer())
        GetCtrl<ctrlEdit>(ID_CHAT_INPUT)->SetFocus();
}

void dskHostGame::Msg_Group_ButtonClick(const unsigned group_id, const unsigned ctrl_id)
{
    unsigned playerId = group_id - ID_PLAYER_GROUP_START;

    switch(ctrl_id)
    {
        // Klick auf Spielername
        case 1:
        {
            if(gameLobby->isHost())
                lobbyHostController->TogglePlayerState(playerId);
        }
        break;
        // Volk
        case 3:
        {
            SetPlayerReady(playerId, false);

            if(playerId == localPlayerId_ || gameLobby->isHost())
            {
                JoinPlayerInfo& player = gameLobby->getPlayer(playerId);
                player.nation = Nation((unsigned(player.nation) + 1) % NUM_NATIONS);
                if(gameLobby->isHost())
                    lobbyHostController->SetNation(playerId, player.nation);
                else
                    GAMECLIENT.Command_SetNation(player.nation);
                ChangeNation(playerId, player.nation);
            }
        }
        break;

        // Farbe
        case 4:
        {
            SetPlayerReady(playerId, false);

            if(playerId == localPlayerId_ || gameLobby->isHost())
            {
                // Get colors used by other players
                std::set<unsigned> takenColors;
                for(unsigned p = 0; p < gameLobby->getNumPlayers(); ++p)
                {
                    // Skip self
                    if(p == playerId)
                        continue;

                    const JoinPlayerInfo& otherPlayer = gameLobby->getPlayer(p);
                    if(otherPlayer.isUsed())
                        takenColors.insert(otherPlayer.color);
                }

                // Look for a unique color
                JoinPlayerInfo& player = gameLobby->getPlayer(playerId);
                int newColorIdx = JoinPlayerInfo::GetColorIdx(player.color);
                do
                {
                    player.color = PLAYER_COLORS[(++newColorIdx) % PLAYER_COLORS.size()];
                } while(helpers::contains(takenColors, player.color));

                if(gameLobby->isHost())
                    lobbyHostController->SetColor(playerId, player.color);
                else
                    GAMECLIENT.Command_SetColor(player.color);
                ChangeColor(playerId, player.color);
            }

            // Start-Farbe der Minimap ändern
        }
        break;

        // Team
        case 5:
        {
            SetPlayerReady(playerId, false);

            if(playerId == localPlayerId_ || gameLobby->isHost())
            {
                JoinPlayerInfo& player = gameLobby->getPlayer(playerId);
                if(player.team >= TM_TEAM1 && player.team < Team(NUM_TEAMS)) // team: 1->2->3->4->0 //-V807
                {
                    player.team = Team((player.team + 1) % NUM_TEAMS);
                } else
                {
                    if(player.team == TM_NOTEAM) // 0(noteam)->randomteam(1-4)
                    {
                        int rnd = rand() % 4;
                        if(!rnd)
                            player.team = TM_RANDOMTEAM;
                        else
                            player.team = Team(TM_RANDOMTEAM2 + rnd - 1);
                    } else // any randomteam -> team 1
                    {
                        player.team = TM_TEAM1;
                    }
                }
                if(gameLobby->isHost())
                    lobbyHostController->SetTeam(playerId, player.team);
                else
                    GAMECLIENT.Command_SetTeam(player.team);
                ChangeTeam(playerId, player.team);
            }
        }
        break;
    }
}

void dskHostGame::Msg_Group_CheckboxChange(const unsigned group_id, const unsigned /*ctrl_id*/, const bool checked)
{
    unsigned playerId = group_id - ID_PLAYER_GROUP_START;

    // Bereit
    if(playerId < MAX_PLAYERS)
        SetPlayerReady(playerId, checked);
}

void dskHostGame::Msg_Group_ComboSelectItem(const unsigned group_id, const unsigned /*ctrl_id*/, const unsigned selection)
{
    if(!gameLobby->isHost())
        return;
    // Swap players
    const unsigned playerId = group_id - ID_PLAYER_GROUP_START;

    int player2 = -1;
    for(unsigned i = 0, playerCtr = 0; i < gameLobby->getNumPlayers(); ++i)
    {
        if(!gameLobby->getPlayer(i).originName.empty() && playerCtr++ == selection)
        {
            player2 = i;
            break;
        }
    }

    if(player2 < 0)
        LOG.write("dskHostGame: ERROR: Selected player not found, stop swapping!\n");
    else
        lobbyHostController->SwapPlayers(playerId, static_cast<unsigned>(player2));
}

void dskHostGame::GoBack()
{
    if(IsSinglePlayer())
        WINDOWMANAGER.Switch(std::make_unique<dskSinglePlayer>());
    else if(serverType == ServerType::LAN)
        WINDOWMANAGER.Switch(std::make_unique<dskLAN>());
    else if(serverType == ServerType::LOBBY && lobbyClient_ && lobbyClient_->IsLoggedIn())
        WINDOWMANAGER.Switch(std::make_unique<dskLobby>());
    else
        WINDOWMANAGER.Switch(std::make_unique<dskDirectIP>());
}

void dskHostGame::Msg_ButtonClick(const unsigned ctrl_id)
{
    if(ctrl_id >= ID_SWAP_BUTTON && ctrl_id < ID_SWAP_BUTTON + MAX_PLAYERS)
    {
        unsigned targetPlayer = ctrl_id - ID_SWAP_BUTTON;
        if(targetPlayer != localPlayerId_ && gameLobby->isHost())
            lobbyHostController->SwapPlayers(localPlayerId_, targetPlayer);
        return;
    }
    switch(ctrl_id)
    {
        case 3: // Zurück
            GAMECLIENT.Stop();
            GoBack();
            break;

        case 2: // Starten
        {
            auto* ready = GetCtrl<ctrlTextButton>(2);
            if(gameLobby->isHost())
            {
                SetPlayerReady(localPlayerId_, true);
                if(lua)
                    lua->EventPlayerReady(localPlayerId_);
                if(ready->GetText() == _("Start game"))
                    lobbyHostController->StartCountdown(5);
                else
                    lobbyHostController->CancelCountdown();
            } else
            {
                if(ready->GetText() == _("Ready"))
                    SetPlayerReady(localPlayerId_, true);
                else
                    SetPlayerReady(localPlayerId_, false);
            }
        }
        break;
        case 22: // Addons
        {
            if(auto* wnd = WINDOWMANAGER.FindNonModalWindow(CGI_ADDONS))
                wnd->Close();
            else
            {
                std::unique_ptr<iwAddons> w;
                if(allowAddonChange && (!lua || lua->IsChangeAllowed("addonsAll")))
                    w = std::make_unique<iwAddons>(gameLobby->getSettings(), this, AddonChangeAllowed::All);
                else if(allowAddonChange)
                    w = std::make_unique<iwAddons>(gameLobby->getSettings(), this, AddonChangeAllowed::WhitelistOnly,
                                                   lua->GetAllowedAddons());
                else
                    w = std::make_unique<iwAddons>(gameLobby->getSettings(), this, AddonChangeAllowed::None);
                WINDOWMANAGER.Show(std::move(w));
            }
        }
        break;
    }
}

void dskHostGame::Msg_EditEnter(const unsigned ctrl_id)
{
    if(ctrl_id != ID_CHAT_INPUT)
        return;
    auto* edit = GetCtrl<ctrlEdit>(ctrl_id);
    const std::string msg = edit->GetText();
    edit->SetText("");
    if(gameChat->IsVisible())
        GAMECLIENT.Command_Chat(msg, CD_ALL);
    else if(lobbyClient_ && lobbyClient_->IsLoggedIn() && lobbyChat->IsVisible())
        lobbyClient_->SendChat(msg);
}

void dskHostGame::CI_Countdown(unsigned remainingTimeInSec)
{
    if(IsSinglePlayer())
        return;

    if(!hasCountdown_)
    {
        const std::string startMsg = helpers::format(_("You have %u seconds until game starts"), remainingTimeInSec);
        gameChat->AddMessage("", "", 0, startMsg, COLOR_RED);
        gameChat->AddMessage("", "", 0, _("Don't forget to check the addon configuration!"), 0xFFFFDD00);
        gameChat->AddMessage("", "", 0, "", 0xFFFFCC00);
        hasCountdown_ = true;
    }

    const std::string message = (remainingTimeInSec > 0) ? " " + std::to_string(remainingTimeInSec) : _("Starting game, please wait");

    gameChat->AddMessage("", "", 0, message, 0xFFFFBB00);
}

void dskHostGame::CI_CancelCountdown(bool error)
{
    if(hasCountdown_)
    {
        hasCountdown_ = false;
        gameChat->AddMessage("", "", 0xFFCC2222, _("Start aborted"), 0xFFFFCC00);
        FlashGameChat();
    }

    if(gameLobby->isHost())
    {
        if(error)
        {
            WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Error"),
                                                          _("Game can only be started as soon as everybody has a unique color,everyone is "
                                                            "ready and all free slots are closed."),
                                                          this, MSB_OK, MSB_EXCLAMATIONRED, 10));
        }

        ChangeReady(localPlayerId_, true);
    }
}

void dskHostGame::FlashGameChat()
{
    if(!gameChat->IsVisible())
    {
        auto* tab = GetCtrl<Window>(ID_CHAT_TAB);
        auto* bt = tab->GetCtrl<ctrlButton>(TAB_GAMECHAT);
        if(!localChatTabAnimId)
            localChatTabAnimId = tab->GetAnimationManager().addAnimation(new BlinkButtonAnim(bt));
    }
}

void dskHostGame::Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult mbr)
{
    switch(msgbox_id)
    {
        case 0: // Verbindung zu Server verloren?
        {
            GAMECLIENT.Stop();

            GoBack();
        }
        break;
        case CGI_ADDONS: // addon-window applied settings?
        {
            if(mbr == MSR_YES)
                UpdateGGS();
        }
        break;
    }
}

void dskHostGame::Msg_ComboSelectItem(const unsigned ctrl_id, const unsigned /*selection*/)
{
    switch(ctrl_id)
    {
        default: break;

        case 43: // Geschwindigkeit
        case 42: // Ziel
        case 41: // Waren
        case 40: // Aufklärung
        {
            // GameSettings wurden verändert, resetten
            UpdateGGS();
        }
        break;
    }
}

void dskHostGame::Msg_CheckboxChange(const unsigned ctrl_id, const bool /*checked*/)
{
    switch(ctrl_id)
    {
        default: break;
        case 19: // Team-Sicht
        case 20: // Teams
        case 23: // random startlocation
        {
            // GameSettings wurden verändert, resetten
            UpdateGGS();
        }
        break;
    }
}

void dskHostGame::Msg_OptionGroupChange(const unsigned ctrl_id, const unsigned selection)
{
    if(ctrl_id == ID_CHAT_TAB)
    {
        gameChat->SetVisible(selection == TAB_GAMECHAT);
        lobbyChat->SetVisible(selection == TAB_LOBBYCHAT);
        auto* tab = GetCtrl<Window>(ID_CHAT_TAB);
        tab->GetCtrl<ctrlButton>(selection)->SetTexture(TC_GREEN2);
        if(selection == TAB_GAMECHAT)
        {
            tab->GetAnimationManager().finishAnimation(localChatTabAnimId, false);
            localChatTabAnimId = 0;
        } else
        {
            tab->GetAnimationManager().finishAnimation(lobbyChatTabAnimId, false);
            lobbyChatTabAnimId = 0;
        }
    }
}

void dskHostGame::UpdateGGS()
{
    RTTR_Assert(gameLobby->isHost());

    GlobalGameSettings& ggs = gameLobby->getSettings();

    // Geschwindigkeit
    ggs.speed = static_cast<GameSpeed>(GetCtrl<ctrlComboBox>(43)->GetSelection());
    // Spielziel
    ggs.objective = static_cast<GameObjective>(GetCtrl<ctrlComboBox>(42)->GetSelection());
    // Waren zu Beginn
    ggs.startWares = static_cast<StartWares>(GetCtrl<ctrlComboBox>(41)->GetSelection());
    // Aufklärung
    ggs.exploration = static_cast<Exploration>(GetCtrl<ctrlComboBox>(40)->GetSelection());
    // Teams gesperrt
    ggs.lockedTeams = GetCtrl<ctrlCheck>(20)->GetCheck();
    // Team sicht
    ggs.teamView = GetCtrl<ctrlCheck>(19)->GetCheck();
    // random locations
    ggs.randomStartPosition = GetCtrl<ctrlCheck>(23)->GetCheck();

    // An Server übermitteln
    lobbyHostController->ChangeGlobalGameSettings(ggs);
}

void dskHostGame::ChangeTeam(const unsigned i, const unsigned char nr)
{
    const std::array<std::string, 9> teams = {"-", "?", "1", "2", "3", "4", "?", "?", "?"};

    GetCtrl<ctrlGroup>(ID_PLAYER_GROUP_START + i)->GetCtrl<ctrlBaseText>(5)->SetText(teams[nr]);
}

void dskHostGame::ChangeReady(const unsigned player, const bool ready)
{
    auto* check = GetCtrl<ctrlGroup>(ID_PLAYER_GROUP_START + player)->GetCtrl<ctrlCheck>(6);
    if(check)
        check->SetCheck(ready);

    if(player == localPlayerId_)
    {
        auto* start = GetCtrl<ctrlTextButton>(2);
        if(gameLobby->isHost())
            start->SetText(hasCountdown_ ? _("Cancel start") : _("Start game"));
        else
            start->SetText(ready ? _("Not Ready") : _("Ready"));
    }
}

void dskHostGame::ChangeNation(const unsigned i, const Nation nation)
{
    GetCtrl<ctrlGroup>(ID_PLAYER_GROUP_START + i)->GetCtrl<ctrlBaseText>(3)->SetText(_(NationNames[nation]));
}

void dskHostGame::ChangePing(unsigned playerId)
{
    unsigned color = COLOR_RED;

    // Farbe bestimmen
    if(gameLobby->getPlayer(playerId).ping < 300)
        color = COLOR_GREEN;
    else if(gameLobby->getPlayer(playerId).ping < 800)
        color = COLOR_YELLOW;

    // und setzen
    GetCtrl<ctrlGroup>(ID_PLAYER_GROUP_START + playerId)->GetCtrl<ctrlVarDeepening>(7)->SetTextColor(color);
}

void dskHostGame::ChangeColor(const unsigned i, const unsigned color)
{
    GetCtrl<ctrlGroup>(ID_PLAYER_GROUP_START + i)->GetCtrl<ctrlBaseColor>(4)->SetColor(color);

    // Minimap-Startfarbe ändern
    if(GetCtrl<ctrlPreviewMinimap>(70))
        GetCtrl<ctrlPreviewMinimap>(70)->SetPlayerColor(i, color);
}

void dskHostGame::SetPlayerReady(unsigned char player, bool ready)
{
    if(player != localPlayerId_)
        return;
    if(gameLobby->isHost())
        ready = true;
    if(gameLobby->getPlayer(player).isReady != ready)
    {
        gameLobby->getPlayer(player).isReady = ready;
        GAMECLIENT.Command_SetReady(ready);
    }
    ChangeReady(player, ready);
}

void dskHostGame::CI_NewPlayer(const unsigned playerId)
{
    // Spielername setzen
    UpdatePlayerRow(playerId);

    if(lua && gameLobby->isHost())
        lua->EventPlayerJoined(playerId);
}

void dskHostGame::CI_PlayerLeft(const unsigned playerId)
{
    UpdatePlayerRow(playerId);
    if(lua && gameLobby->isHost())
        lua->EventPlayerLeft(playerId);
}

void dskHostGame::CI_GameLoading(const std::shared_ptr<Game>& game)
{
    // Desktop wechseln
    WINDOWMANAGER.Switch(std::make_unique<dskGameLoader>(game));
}

void dskHostGame::CI_PlayerDataChanged(unsigned playerId)
{
    UpdatePlayerRow(playerId);
}

void dskHostGame::CI_PingChanged(const unsigned playerId, const unsigned short /*ping*/)
{
    ChangePing(playerId);
}

void dskHostGame::CI_ReadyChanged(const unsigned playerId, const bool ready)
{
    ChangeReady(playerId, ready);
    // Event only called for other players (host ready is done in start game)
    // Also only for host and non-savegames
    if(ready && lua && gameLobby->isHost() && playerId != localPlayerId_)
        lua->EventPlayerReady(playerId);
}

void dskHostGame::CI_PlayersSwapped(const unsigned player1, const unsigned player2)
{
    if(player1 == localPlayerId_)
        localPlayerId_ = player2;
    else if(localPlayerId_ == player2)
        localPlayerId_ = player1;
    // Spieler wurden vertauscht, beide Reihen updaten
    UpdatePlayerRow(player1);
    UpdatePlayerRow(player2);
}

void dskHostGame::CI_GGSChanged(const GlobalGameSettings& /*ggs*/)
{
    const GlobalGameSettings& ggs = gameLobby->getSettings();

    // Geschwindigkeit
    GetCtrl<ctrlComboBox>(43)->SetSelection(static_cast<unsigned short>(ggs.speed));
    // Ziel
    GetCtrl<ctrlComboBox>(42)->SetSelection(static_cast<unsigned short>(ggs.objective));
    // Waren
    GetCtrl<ctrlComboBox>(41)->SetSelection(static_cast<unsigned short>(ggs.startWares));
    // Aufklärung
    GetCtrl<ctrlComboBox>(40)->SetSelection(static_cast<unsigned short>(ggs.exploration));
    // Teams
    GetCtrl<ctrlCheck>(20)->SetCheck(ggs.lockedTeams);
    // Team-Sicht
    GetCtrl<ctrlCheck>(19)->SetCheck(ggs.teamView);
    // random location
    GetCtrl<ctrlCheck>(23)->SetCheck(ggs.randomStartPosition);

    SetPlayerReady(localPlayerId_, false);
}

void dskHostGame::CI_Chat(const unsigned playerId, const ChatDestination /*cd*/, const std::string& msg)
{
    if((playerId != 0xFFFFFFFF) && !IsSinglePlayer())
    {
        std::string time = s25util::Time::FormatTime("(%H:%i:%s)");

        gameChat->AddMessage(time, gameLobby->getPlayer(playerId).name, gameLobby->getPlayer(playerId).color, msg, 0xFFFFFF00); //-V810
        FlashGameChat();
    }
}

void dskHostGame::CI_Error(const ClientError ce)
{
    WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Error"), ClientErrorToStr(ce), this, MSB_OK, MSB_EXCLAMATIONRED, 0));
}

/**
 *  (Lobby-)Status: Benutzerdefinierter Fehler (kann auch Conn-Loss o.ä sein)
 */
void dskHostGame::LC_Status_Error(const std::string& error)
{
    WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Error"), error, this, MSB_OK, MSB_EXCLAMATIONRED, 0));
}

void dskHostGame::LC_Chat(const std::string& player, const std::string& text)
{
    if(!lobbyChat)
        return;
    lobbyChat->AddMessage("", player, ctrlChat::CalcUniqueColor(player), text, COLOR_YELLOW);
    if(!lobbyChat->IsVisible())
    {
        auto* tab = GetCtrl<Window>(ID_CHAT_TAB);
        auto* bt = tab->GetCtrl<ctrlButton>(TAB_LOBBYCHAT);
        if(!lobbyChatTabAnimId)
            lobbyChatTabAnimId = tab->GetAnimationManager().addAnimation(new BlinkButtonAnim(bt));
    }
}
