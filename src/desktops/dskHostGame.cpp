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

#include "defines.h" // IWYU pragma: keep
#include "dskHostGame.h"
#include "dskGameLoader.h"

#include "WindowManager.h"
#include "Loader.h"
#include "GameClient.h"
#include "GameServer.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlChat.h"
#include "controls/ctrlCheck.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlDeepening.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlGroup.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlPreviewMinimap.h"
#include "controls/ctrlText.h"
#include "controls/ctrlVarDeepening.h"
#include "desktops/dskDirectIP.h"
#include "desktops/dskLobby.h"
#include "desktops/dskSinglePlayer.h"
#include "desktops/dskLAN.h"
#include "drivers/VideoDriverWrapper.h"
#include "ingameWindows/iwMsgbox.h"
#include "ingameWindows/iwAddons.h"
#include "GameLobby.h"
#include "ogl/glArchivItem_Font.h"
#include "lua/LuaInterfaceSettings.h"
#include "JoinPlayerInfo.h"
#include "gameData/GameConsts.h"
#include "gameData/const_gui_ids.h"
#include "liblobby/src/LobbyClient.h"
#include "libutil/src/Log.h"
#include "libsiedler2/src/prototypen.h"
#include <set>

namespace{
    enum CtrlIds
    {
        ID_SWAP_BUTTON = 80,
        ID_FIRST_FREE = ID_SWAP_BUTTON + MAX_PLAYERS,
        ID_GAME_CHAT,
        ID_LOBBY_CHAT,
        ID_CHAT_INPUT,
        ID_CHAT_TAB,
        TAB_GAMECHAT,
        TAB_LOBBYCHAT,
        ID_UNREAD_ANIM_TIMER
    };
}

dskHostGame::dskHostGame(const ServerType serverType) :
    Desktop(LOADER.GetImageN("setup015", 0)), serverType(serverType), gameLobby(GAMECLIENT.GetGameLobby()),
    hasCountdown_(false), wasActivated(false), gameChat(NULL), lobbyChat(NULL), hasUnreadChat(false)
{
    if(!GAMECLIENT.GetLuaFilePath().empty())
    {
        lua.reset(new LuaInterfaceSettings(GAMESERVER.GetInterface()));
        if(!lua->LoadScript(GAMECLIENT.GetLuaFilePath()))
        {
            WINDOWMANAGER.ShowAfterSwitch(new iwMsgbox(_("Error"), _("Lua script was found but failed to load. Map might not work as expected!"), this, MSB_OK, MSB_EXCLAMATIONRED, 1));
            lua.reset();
        } else if(!lua->CheckScriptVersion())
        {
            WINDOWMANAGER.ShowAfterSwitch(new iwMsgbox(_("Error"), _("Lua script uses a different version and cannot be used. Map might not work as expected!"), this, MSB_OK, MSB_EXCLAMATIONRED, 1));
            lua.reset();
        } else if(!lua->EventSettingsInit(serverType == ServerType::LOCAL, GAMECLIENT.IsSavegame()))
        {
            RTTR_Assert(GAMECLIENT.IsHost()); // This should be done first for the host so others won't even see the script
            LOG.write("Lua was disabled by the script itself\n");
            lua.reset();
            // Double check...
            if(GAMECLIENT.IsHost())
                GAMESERVER.RemoveLuaScript();
        }
    }


    const bool readonlySettings = !GAMECLIENT.IsHost() || GAMECLIENT.IsSavegame() || (lua && !lua->IsChangeAllowed("general"));
    allowAddonChange = GAMECLIENT.IsHost() && !GAMECLIENT.IsSavegame() && (!lua || lua->IsChangeAllowed("addonsAll") || lua->IsChangeAllowed("addonsSome"));

    // Kartenname
    AddText(0, 400, 5, GAMECLIENT.GetGameName(), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, LargeFont);

    // "Spielername"
    AddText(10, 95, 40, _("Player Name"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, NormalFont);
    // "Einstufung"
    AddText(11, 205, 40, _("Classification"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, NormalFont);
    // "Volk"
    AddText(12, 285, 40, _("Race"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, NormalFont);
    // "Farbe"
    AddText(13, 355, 40, _("Color"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, NormalFont);
    // "Team"
    AddText(14, 405, 40, _("Team"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, NormalFont);

    if (!IsSinglePlayer())
    {
        // "Bereit"
        AddText(15, 465, 40, _("Ready?"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, NormalFont);
        // "Ping"
        AddText(16, 515, 40, _("Ping"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, NormalFont);
    }
    // "Swap"
    if(GAMECLIENT.IsHost() && !GAMECLIENT.IsSavegame())
        AddText(24, 10, 40, _("Swap"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, NormalFont);
    // "Verschieben" (nur bei Savegames!)
    if(GAMECLIENT.IsSavegame())
        AddText(17, 645, 40, _("Past player"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, NormalFont);

    if (!IsSinglePlayer())
    {
        // Enable lobby chat when we are logged in
        if(LOBBYCLIENT.IsLoggedIn())
        {
            ctrlOptionGroup* chatTab = AddOptionGroup(ID_CHAT_TAB, ctrlOptionGroup::CHECK, scale_);
            chatTab->AddTextButton(TAB_GAMECHAT, 20, 320, 178, 22, TC_GREEN2, _("Game Chat"), NormalFont);
            chatTab->AddTextButton(TAB_LOBBYCHAT, 202, 320, 178, 22, TC_GREEN2, _("Lobby Chat"), NormalFont);
            gameChat  = AddChatCtrl(ID_GAME_CHAT,  20, 345, 360, 218 - 25, TC_GREY, NormalFont);
            lobbyChat = AddChatCtrl(ID_LOBBY_CHAT, 20, 345, 360, 218 - 25, TC_GREY, NormalFont);
            chatTab->SetSelection(TAB_GAMECHAT, true);
            AddTimer(ID_UNREAD_ANIM_TIMER, 500);
        } else{
            // Chatfenster
            gameChat = AddChatCtrl(ID_GAME_CHAT, 20, 320, 360, 218, TC_GREY, NormalFont);
        }
        // Edit für Chatfenster
        AddEdit(ID_CHAT_INPUT, 20, 540, 360, 22, TC_GREY, NormalFont);
    }

    // "Spiel starten"
    AddTextButton(2, 600, 560, 180, 22, TC_GREEN2, (GAMECLIENT.IsHost() ? _("Start game") : _("Ready")), NormalFont);

    // "Zurück"
    AddTextButton(3, 400, 560, 180, 22, TC_RED1, _("Return"), NormalFont);

    // "Teams sperren"
    AddCheckBox(20, 400, 460, 180, 26, TC_GREY, _("Lock teams:"), NormalFont, readonlySettings);
    // "Gemeinsame Team-Sicht"
    AddCheckBox(19, 600, 460, 180, 26, TC_GREY, _("Shared team view"), NormalFont, readonlySettings);
    // "Random Start Locations"
    AddCheckBox(23, 600, 430, 180, 26, TC_GREY, _("Random start locations"), NormalFont, readonlySettings);

    // "Enhancements"
    AddText(21, 400, 499, _("Addons:"), COLOR_YELLOW, 0, NormalFont);
    AddTextButton(22, 600, 495, 180, 22, TC_GREEN2, allowAddonChange ? _("Change Settings...") : _("View Settings..."), NormalFont);

    ctrlComboBox* combo;

    // umgedrehte Reihenfolge, damit die Listen nicht dahinter sind

    // "Aufklärung"
    AddText(30, 400, 405, _("Exploration:"), COLOR_YELLOW, 0, NormalFont);
    combo = AddComboBox(40, 600, 400, 180, 20, TC_GREY, NormalFont, 100, readonlySettings);
    combo->AddString(_("Off (all visible)"));
    combo->AddString(_("Classic (Settlers 2)"));
    combo->AddString(_("Fog of War"));
    combo->AddString(_("FoW - all explored"));

    // "Waren zu Beginn"
    AddText(31, 400, 375, _("Goods at start:"), COLOR_YELLOW, 0, NormalFont);
    combo = AddComboBox(41, 600, 370, 180, 20, TC_GREY, NormalFont, 100, readonlySettings);
    combo->AddString(_("Very Low"));
    combo->AddString(_("Low"));
    combo->AddString(_("Normal"));
    combo->AddString(_("A lot"));

    // "Spielziel"
    AddText(32, 400, 345, _("Goals:"), COLOR_YELLOW, 0, NormalFont);
    combo = AddComboBox(42, 600, 340, 180, 20, TC_GREY, NormalFont, 100, readonlySettings);
    combo->AddString(_("None")); // Kein Spielziel
    combo->AddString(_("Conquer 3/4 of map")); // Besitz 3/4 des Landes
    combo->AddString(_("Total domination")); // Alleinherrschaft
    // Lobby game?
    if(LOBBYCLIENT.IsLoggedIn())
    {
        // Then add tournament modes as possible "objectives"
        for(unsigned i = 0; i < TOURNAMENT_MODES_COUNT; ++i)
        {
            char str[512];
            sprintf (str, _("Tournament: %u minutes"), TOURNAMENT_MODES_DURATION[i]);
            combo->AddString(str);
        }
    }

    // "Geschwindigkeit"
    AddText(33, 400, 315, _("Speed:"), COLOR_YELLOW, 0, NormalFont);
    combo = AddComboBox(43, 600, 310, 180, 20, TC_GREY, NormalFont, 100, !GAMECLIENT.IsHost());
    combo->AddString(_("Very slow")); // Sehr Langsam
    combo->AddString(_("Slow")); // Langsam
    combo->AddString(_("Normal")); // Normal
    combo->AddString(_("Fast")); // Schnell
    combo->AddString(_("Very fast")); // Sehr Schnell

    // Karte laden, um Kartenvorschau anzuzeigen
    if(GAMECLIENT.GetMapType() == MAPTYPE_OLDMAP)
    {
        // Map laden
        libsiedler2::ArchivInfo mapArchiv;
        // Karteninformationen laden
        if(libsiedler2::loader::LoadMAP(GAMECLIENT.GetMapPath(), mapArchiv) == 0)
        {
            glArchivItem_Map* map = static_cast<glArchivItem_Map*>(mapArchiv.get(0));
            ctrlPreviewMinimap* preview = AddPreviewMinimap(70, 560, 40, 220, 220, map);

            // Titel der Karte, Y-Position relativ je nach Höhe der Minimap festlegen, daher nochmals danach
            // verschieben, da diese Position sonst skaliert wird!
            ctrlText* text = AddText(71, 670, 0, _("Map: ") +  GAMECLIENT.GetMapTitle(), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, NormalFont);
            text->Move(text->GetX(false), preview->GetY(false) + preview->GetBottom() + 10);
        }
    }

    if (IsSinglePlayer() && !GAMECLIENT.IsSavegame())
    {
        // Setze initial auf KI
        for (unsigned char i = 0; i < gameLobby.GetPlayerCount(); i++)
        {
            if (!gameLobby.GetPlayer(i).isHost)
                GAMESERVER.TogglePlayerState(i);
        }
    }

    // Alle Spielercontrols erstellen
    for(unsigned char i = gameLobby.GetPlayerCount(); i; --i)
        UpdatePlayerRow(i - 1);
    //swap buttons erstellen
    if(GAMECLIENT.IsHost() && !GAMECLIENT.IsSavegame() && (!lua || lua->IsChangeAllowed("swapping")))
    {
        for(unsigned char i = gameLobby.GetPlayerCount(); i; --i)
            AddTextButton(ID_SWAP_BUTTON + i - 1, 5, 80 + (i - 1) * 30, 10, 22, TC_RED1, _("-"), NormalFont);;
    }
    CI_GGSChanged(gameLobby.GetSettings());

    LOBBYCLIENT.SetInterface(this);
    if(serverType == ServerType::LOBBY && LOBBYCLIENT.IsLoggedIn())
    {
        LOBBYCLIENT.SendServerJoinRequest();
        LOBBYCLIENT.SendRankingInfoRequest(gameLobby.GetPlayer(GAMECLIENT.GetPlayerId()).name);
        for(unsigned char i = 0; i < gameLobby.GetPlayerCount(); ++i)
        {
            JoinPlayerInfo& player = gameLobby.GetPlayer(i);
            if(player.ps == PS_OCCUPIED)
                LOBBYCLIENT.SendRankingInfoRequest(player.name);
        }
    }

    GAMECLIENT.SetInterface(this);
}

/**
 *  Größe ändern-Reaktionen die nicht vom Skaling-Mechanismus erfasst werden.
 */
void dskHostGame::Resize(unsigned short width, unsigned short height)
{
    Window::Resize(width, height);

    // Text unter der PreviewMinimap verschieben, dessen Höhe von der Höhe der
    // PreviewMinimap abhängt, welche sich gerade geändert hat.
    ctrlPreviewMinimap* preview = GetCtrl<ctrlPreviewMinimap>(70);
    ctrlText* text = GetCtrl<ctrlText>(71);
    if(preview && text)
        text->Move(text->GetX(false), preview->GetY(false) + preview->GetBottom() + 10);
}

void dskHostGame::SetActive(bool activate /*= true*/)
{
    Desktop::SetActive(activate);
    if(activate && !wasActivated && lua && GAMECLIENT.IsHost())
    {
        wasActivated = true;
        lua->EventSettingsReady();
    }
}

void dskHostGame::UpdatePlayerRow(const unsigned row)
{
    JoinPlayerInfo& player = gameLobby.GetPlayer(row);

    unsigned cy = 80 + row * 30;
    TextureColor tc = (row & 1 ? TC_GREY : TC_GREEN2);

    // Alle Controls erstmal zerstören (die ganze Gruppe)
    DeleteCtrl(58 - row);
    // und neu erzeugen
    ctrlGroup* group = AddGroup(58 - row, scale_);

    std::string name;
    // Name
    switch(player.ps)
    {
        default:
            name.clear();
            break;
        case PS_OCCUPIED:
        case PS_AI:
        {
            name = player.name;
        } break;
        case PS_FREE:
        case PS_RESERVED:
        {
            // Offen
            name = _("Open");
        } break;
        case PS_LOCKED:
        {
            // Geschlossen
            name = _("Closed");
        } break;
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
    ctrlBaseText* text;
    if(GAMECLIENT.IsHost() && !player.isHost && (!lua || lua->IsChangeAllowed("playerState")))
        text = group->AddTextButton(1, 20, cy, 150, 22, tc, name, NormalFont);
    else
        text = group->AddTextDeepening(1, 20, cy, 150, 22, tc, name, NormalFont, COLOR_YELLOW);

    // Is das der Host? Dann farblich markieren
    if(player.isHost)
        text->SetTextColor(0xFF00FF00);

    // Bei geschlossenem nicht sichtbar
    if(player.isUsed())
    {
        /// Einstufung nur bei Lobbyspielen anzeigen @todo Einstufung ( "%d" )
        group->AddVarDeepening(2, 180, cy, 50, 22, tc, (LOBBYCLIENT.IsLoggedIn() || player.ps == PS_AI ? _("%d") : _("n/a")), NormalFont, COLOR_YELLOW, 1, &player.rating); //-V111

        // If not in savegame -> Player can change own row and host can change AIs
        const bool allowPlayerChange = ((GAMECLIENT.IsHost() && player.ps == PS_AI) || GAMECLIENT.GetPlayerId() == row) && !GAMECLIENT.IsSavegame();
        bool allowNationChange = allowPlayerChange;
        bool allowColorChange = allowPlayerChange;
        bool allowTeamChange = allowPlayerChange;
        if(lua)
        {
            if(GAMECLIENT.GetPlayerId() == row)
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
            group->AddTextButton(3, 240, cy, 90, 22, tc, _(NationNames[0]), NormalFont);
        else
            group->AddTextDeepening(3, 240, cy, 90, 22, tc, _(NationNames[0]), NormalFont, COLOR_YELLOW);

        if(allowColorChange)
            group->AddColorButton(4, 340, cy, 30, 22, tc, 0);
        else
            group->AddColorDeepening(4, 340, cy, 30, 22, tc, 0);

        if(allowTeamChange)
            group->AddTextButton(5, 380, cy, 50, 22, tc, _("-"), NormalFont);
        else
            group->AddTextDeepening(5, 380, cy, 50, 22, tc, _("-"), NormalFont, COLOR_YELLOW);

        // Bereit (nicht bei KIs und Host)
        if(player.ps == PS_OCCUPIED && !player.isHost)
            group->AddCheckBox(6, 450, cy, 22, 22, tc, EMPTY_STRING, NULL, (GAMECLIENT.GetPlayerId() != row) );

        // Ping ( "%d" )
        ctrlVarDeepening* ping = group->AddVarDeepening(7, 490, cy, 50, 22, tc, _("%d"), NormalFont, COLOR_YELLOW, 1, &player.ping); //-V111

        // Verschieben (nur bei Savegames und beim Host!)
        if(GAMECLIENT.IsSavegame() && player.ps == PS_OCCUPIED)
        {
            ctrlComboBox* combo = group->AddComboBox(8, 570, cy, 150, 22, tc, NormalFont, 150, !GAMECLIENT.IsHost());

            // Mit den alten Namen füllen
            for(unsigned i = 0; i < gameLobby.GetPlayerCount(); ++i)
            {
                if(gameLobby.GetPlayer(i).originName.length())
                {
                    combo->AddString(gameLobby.GetPlayer(i).originName);
                    if(i == row)
                        combo->SetSelection(combo->GetCount() - 1);
                }
            }
        }

        // Ping bei KI und Host ausblenden
        if(player.ps == PS_AI || player.isHost)
            ping->SetVisible(false);

        // Felder ausfüllen
        ChangeNation(row, player.nation);
        ChangeTeam(row, player.team);
        ChangePing(row);
        ChangeReady(row, player.isReady);
        ChangeColor(row, player.color);
    }
    group->SetActive(this->active_);
}

/**
 *  Methode vor dem Zeichnen
 */
void dskHostGame::Msg_PaintBefore()
{
    // Chatfenster Fokus geben
    if (!IsSinglePlayer())
        GetCtrl<ctrlEdit>(ID_CHAT_INPUT)->SetFocus();
}

void dskHostGame::Msg_Timer(const unsigned timerId)
{
    if(timerId != ID_UNREAD_ANIM_TIMER || !hasUnreadChat)
        return;
    ctrlButton* bt = GetCtrl<Window>(ID_CHAT_TAB)->GetCtrl<ctrlButton>(gameChat->IsVisible() ? TAB_LOBBYCHAT : TAB_GAMECHAT);
    if(bt->GetTexture() != TC_GREEN2)
        bt->SetTexture(TC_GREEN2);
    else
        bt->SetTexture(TC_RED2);

}

void dskHostGame::Msg_Group_ButtonClick(const unsigned group_id, const unsigned ctrl_id)
{
    unsigned playerId = 8 - (group_id - 50);

    switch(ctrl_id)
    {
            // Klick auf Spielername
        case 1:
        {
            if(GAMECLIENT.IsHost())
                GAMESERVER.TogglePlayerState(playerId);
        } break;
        // Volk
        case 3:
        {
            TogglePlayerReady(playerId, false);

            if(GAMECLIENT.IsHost())
                GAMESERVER.ToggleAINation(playerId);
            if(playerId == GAMECLIENT.GetPlayerId())
            {
                JoinPlayerInfo& localPlayer = gameLobby.GetPlayer(playerId);
                localPlayer.nation = Nation((unsigned(localPlayer.nation) + 1) % NAT_COUNT);
                GAMECLIENT.Command_SetNation(localPlayer.nation);
                ChangeNation(playerId, localPlayer.nation);
            }
        } break;

        // Farbe
        case 4:
        {
            TogglePlayerReady(playerId, false);

            if(playerId == GAMECLIENT.GetPlayerId())
            {
                // Get colors used by other players
                std::set<unsigned> takenColors;
                for(unsigned p = 0; p < gameLobby.GetPlayerCount(); ++p)
                {
                    // Skip self
                    if(p == GAMECLIENT.GetPlayerId())
                        continue;

                    JoinPlayerInfo& otherPlayer = gameLobby.GetPlayer(p);
                    if(otherPlayer.isUsed())
                        takenColors.insert(otherPlayer.color);
                }

                // Look for a unique color
                JoinPlayerInfo& player = gameLobby.GetPlayer(playerId); 
                int newColorIdx = player.GetColorIdx(player.color);
                do{
                    player.color = PLAYER_COLORS[(++newColorIdx) % PLAYER_COLORS.size()];
                } while(helpers::contains(takenColors, player.color));

                GAMECLIENT.Command_SetColor(player.color);
                ChangeColor(playerId, player.color);
            } else if(GAMECLIENT.IsHost())
                GAMESERVER.ToggleAIColor(playerId);

            // Start-Farbe der Minimap ändern
        } break;

        // Team
        case 5:
        {
            TogglePlayerReady(playerId, false);

            if(GAMECLIENT.IsHost())
                GAMESERVER.ToggleAITeam(playerId);
            if(playerId == GAMECLIENT.GetPlayerId())
            {
                JoinPlayerInfo& player = gameLobby.GetPlayer(playerId);
                if(player.team >= TM_TEAM1 && player.team < Team(TEAM_COUNT)) // team: 1->2->3->4->0 //-V807
                {
                    player.team = Team((player.team + 1) % TEAM_COUNT);
                }
                else
                {
                    if(player.team == TM_NOTEAM) // 0(noteam)->randomteam(1-4)
                    {
                        int rnd = rand() % 4;
                        if(!rnd)
                            player.team = TM_RANDOMTEAM;
                        else
                            player.team = Team(TM_RANDOMTEAM2 + rnd - 1);
                    }
                    else //any randomteam -> team 1
                    {
                        player.team = TM_TEAM1;
                    }
                }
                GAMECLIENT.Command_SetTeam(player.team);
                ChangeTeam(GAMECLIENT.GetPlayerId(), player.team);
            }
        } break;
    }
}

void dskHostGame::Msg_Group_CheckboxChange(const unsigned group_id, const unsigned  /*ctrl_id*/, const bool checked)
{
    unsigned playerId = 8 - (group_id - 50);

    // Bereit
    if(playerId < 8)
        TogglePlayerReady(playerId, checked);
}

void dskHostGame::Msg_Group_ComboSelectItem(const unsigned group_id, const unsigned  /*ctrl_id*/, const int selection)
{
    unsigned playerId = 8 - (group_id - 50);

    // Spieler wurden vertauscht

    // 2. Player herausfinden (Strings vergleichen
    unsigned player2;
    for(player2 = 0; player2 < gameLobby.GetPlayerCount(); ++player2)
    {
        if(gameLobby.GetPlayer(player2).originName == GetCtrl<ctrlGroup>(group_id)->GetCtrl<ctrlComboBox>(8)->GetText(selection))
            break;
    }

    // Keinen Namen gefunden?
    if(player2 == gameLobby.GetPlayerCount())
    {
        LOG.write("dskHostGame: ERROR: No Origin Name found, stop swapping!\n");
        return;
    }

    GAMESERVER.SwapPlayer(playerId, player2);
}


void dskHostGame::GoBack()
{
    if (IsSinglePlayer())
        WINDOWMANAGER.Switch(new dskSinglePlayer);
    else if (serverType == ServerType::LAN)
        WINDOWMANAGER.Switch(new dskLAN);
    else if (serverType == ServerType::LOBBY && LOBBYCLIENT.IsLoggedIn())
        WINDOWMANAGER.Switch(new dskLobby);
    else
        WINDOWMANAGER.Switch(new dskDirectIP);
}

void dskHostGame::Msg_ButtonClick(const unsigned ctrl_id)
{
    if(ctrl_id >= ID_SWAP_BUTTON && ctrl_id < ID_SWAP_BUTTON + MAX_PLAYERS)
    {
        LOG.write("dskHostGame: swap button pressed\n");
        unsigned char p = 0;
        while(p < MAX_PLAYERS && !gameLobby.GetPlayer(p).isHost)
            p++;

        if(p < MAX_PLAYERS)
        {
            GAMESERVER.SwapPlayer(p, ctrl_id - ID_SWAP_BUTTON);
            CI_PlayersSwapped(p, ctrl_id - ID_SWAP_BUTTON);
        }else
            LOG.write("dskHostGame: could not find host\n");
        return;
    }
    switch(ctrl_id)
    {
        case 3: // Zurück
        {
            if(GAMECLIENT.IsHost())
                GAMESERVER.Stop();
            GAMECLIENT.Stop();

            GoBack();
        } break;

        case 2: // Starten
        {
            ctrlTextButton* ready = GetCtrl<ctrlTextButton>(2);
            if(GAMECLIENT.IsHost())
            {
                if(lua)
                    lua->EventPlayerReady(GAMECLIENT.GetPlayerId());
                if(ready->GetText() == _("Start game"))
                {
                    if(GAMESERVER.StartCountdown())
                    {
                        ready->SetText(_("Cancel start"));
                    }
                    else
                        WINDOWMANAGER.Show(new iwMsgbox(_("Error"), _("Game can only be started as soon as everybody has a unique color,everyone is ready and all free slots are closed."), this, MSB_OK, MSB_EXCLAMATIONRED, 10));
                }
                else
                    GAMESERVER.CancelCountdown();
            }
            else
            {
                if(ready->GetText() == _("Ready"))
                    TogglePlayerReady(GAMECLIENT.GetPlayerId(), true);
                else
                    TogglePlayerReady(GAMECLIENT.GetPlayerId(), false);
            }
        } break;
        case 22: // Addons
        {
            iwAddons* w;
            if(allowAddonChange && (!lua || lua->IsChangeAllowed("addonsAll")))
                w = new iwAddons(gameLobby.GetSettings(), iwAddons::HOSTGAME);
            else if(allowAddonChange)
                w = new iwAddons(gameLobby.GetSettings(), iwAddons::HOSTGAME_WHITELIST, lua->GetAllowedAddons());
            else
                w = new iwAddons(gameLobby.GetSettings(), iwAddons::READONLY);
            w->SetParent(this);
            WINDOWMANAGER.Show(w);
        } break;
    }
}

void dskHostGame::Msg_EditEnter(const unsigned ctrl_id)
{
    if(ctrl_id != ID_CHAT_INPUT)
        return;
    ctrlEdit* edit = GetCtrl<ctrlEdit>(ctrl_id);
    const std::string msg = edit->GetText();
    edit->SetText("");
    if(gameChat->IsVisible())
        GAMECLIENT.Command_Chat(msg, CD_ALL);
    else if(LOBBYCLIENT.IsLoggedIn() && lobbyChat->IsVisible())
        LOBBYCLIENT.SendChat(msg);
}

void dskHostGame::CI_Countdown(unsigned remainingTimeInSec)
{
    if (IsSinglePlayer())
        return;

    if (!hasCountdown_)
    {
        char startMsg[100];
        sprintf(startMsg, _("You have %u seconds until game starts"), remainingTimeInSec);
        gameChat->AddMessage("", "", 0, startMsg, COLOR_RED);
        gameChat->AddMessage("", "", 0, _("Don't forget to check the addon configuration!"), 0xFFFFDD00);
        gameChat->AddMessage("", "", 0, "", 0xFFFFCC00);
        hasCountdown_ = true;
    }

    std::stringstream message;
    if(remainingTimeInSec > 0)
        message << " " << remainingTimeInSec;
    else
        message << _("Starting game, please wait");

    gameChat->AddMessage("", "", 0, message.str(), 0xFFFFBB00);
}

void dskHostGame::CI_CancelCountdown()
{
    if (IsSinglePlayer())
        return;

    gameChat->AddMessage("", "", 0xFFCC2222, _("Start aborted"), 0xFFFFCC00);

    hasCountdown_ = false;

    if(GAMECLIENT.IsHost())
        TogglePlayerReady(GAMECLIENT.GetPlayerId(), false);
}

void dskHostGame::Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult mbr)
{
    switch(msgbox_id)
    {
        case 0: // Verbindung zu Server verloren?
        {
            GAMECLIENT.Stop();

            GoBack();
        } break;
        case CGI_ADDONS: // addon-window applied settings?
        {
            if(mbr == MSR_YES)
                UpdateGGS();
        } break;
    }
}

void dskHostGame::Msg_ComboSelectItem(const unsigned ctrl_id, const int  /*selection*/)
{
    switch(ctrl_id)
    {
        default:
            break;

        case 43: // Geschwindigkeit
        case 42: // Ziel
        case 41: // Waren
        case 40: // Aufklärung
        {
            // GameSettings wurden verändert, resetten
            UpdateGGS();
        } break;
    }
}

void dskHostGame::Msg_CheckboxChange(const unsigned ctrl_id, const bool  /*checked*/)
{

    switch(ctrl_id)
    {
        default:
            break;
        case 19: // Team-Sicht
        case 20: // Teams
        case 23: //random startlocation
        {
            // GameSettings wurden verändert, resetten
            UpdateGGS();
        } break;
    }
}

void dskHostGame::Msg_OptionGroupChange(const unsigned ctrl_id, const int selection)
{
    if(ctrl_id == ID_CHAT_TAB)
    {
        gameChat->SetVisible(selection == TAB_GAMECHAT);
        lobbyChat->SetVisible(selection == TAB_LOBBYCHAT);
        GetCtrl<Window>(ID_CHAT_TAB)->GetCtrl<ctrlButton>(selection)->SetTexture(TC_GREEN2);
        hasUnreadChat = false;
    }
}

void dskHostGame::UpdateGGS()
{
    GlobalGameSettings& ggs = gameLobby.GetSettings();

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
    //random locations
    ggs.randomStartPosition = GetCtrl<ctrlCheck>(23)->GetCheck();

    // An Server übermitteln
    GAMESERVER.ChangeGlobalGameSettings(ggs);
}

void dskHostGame::ChangeTeam(const unsigned i, const unsigned char nr)
{
    const std::string teams[9] =
    { "-", "?", "1", "2", "3", "4", "?", "?", "?"};

    GetCtrl<ctrlGroup>(58 - i)->GetCtrl<ctrlBaseText>(5)->SetText(teams[nr]);
}

void dskHostGame::ChangeReady(const unsigned player, const bool ready)
{
    ctrlCheck* check = GetCtrl<ctrlGroup>(58 - player)->GetCtrl<ctrlCheck>(6);
    if(check)
        check->SetCheck(ready);

    ctrlTextButton* start = GetCtrl<ctrlTextButton>(2);
    if(player == GAMECLIENT.GetPlayerId())
    {
        if(GAMECLIENT.IsHost())
            start->SetText(hasCountdown_ ? _("Cancel start") : _("Start game"));
        else
            start->SetText(ready ? _("Not Ready") : _("Ready"));
    }
}

void dskHostGame::ChangeNation(const unsigned i, const Nation nation)
{
    GetCtrl<ctrlGroup>(58 - i)->GetCtrl<ctrlBaseText>(3)->SetText(_(NationNames[nation]));
}

void dskHostGame::ChangePing(unsigned playerId)
{
    unsigned color = COLOR_RED;

    // Farbe bestimmen
    if(gameLobby.GetPlayer(playerId).ping < 300)
        color = COLOR_GREEN;
    else if(gameLobby.GetPlayer(playerId).ping < 800 )
        color = COLOR_YELLOW;

    // und setzen
    GetCtrl<ctrlGroup>(58 - playerId)->GetCtrl<ctrlVarDeepening>(7)->SetTextColor(color);
}

void dskHostGame::ChangeColor(const unsigned i, const unsigned color)
{
    GetCtrl<ctrlGroup>(58 - i)->GetCtrl<ctrlBaseColor>(4)->SetColor(color);

    // Minimap-Startfarbe ändern
    if(GetCtrl<ctrlPreviewMinimap>(70))
        GetCtrl<ctrlPreviewMinimap>(70)->SetPlayerColor(i, color);
}

void dskHostGame::TogglePlayerReady(unsigned char player, bool ready)
{
    if(player != GAMECLIENT.GetPlayerId())
        return;
    if(GAMECLIENT.IsHost())
        ready = true;
    if(gameLobby.GetPlayer(player).isReady != ready)
    {
        gameLobby.GetPlayer(player).isReady = ready;
        GAMECLIENT.Command_SetReady(ready);
    }
    ChangeReady(player, ready);
}

void dskHostGame::CI_NewPlayer(const unsigned playerId)
{
    // Spielername setzen
    UpdatePlayerRow(playerId);

    // Rankinginfo abrufen
    if(LOBBYCLIENT.IsLoggedIn())
    {
        for(unsigned char i = 0; i < gameLobby.GetPlayerCount(); ++i)
        {
            JoinPlayerInfo& player = gameLobby.GetPlayer(i);
            if(player.ps == PS_OCCUPIED)
                LOBBYCLIENT.SendRankingInfoRequest(player.name);
        }
    }
    if(lua && GAMECLIENT.IsHost())
        lua->EventPlayerJoined(playerId);
}

void dskHostGame::CI_PlayerLeft(const unsigned playerId)
{
    UpdatePlayerRow(playerId);
    if(lua && GAMECLIENT.IsHost())
        lua->EventPlayerLeft(playerId);
}

void dskHostGame::CI_GameStarted(GameWorldBase& world)
{
    // Desktop wechseln
    WINDOWMANAGER.Switch(new dskGameLoader(world));
}

void dskHostGame::CI_PSChanged(const unsigned playerId, const PlayerState ps)
{
    if (IsSinglePlayer() && (ps == PS_FREE))
        GAMESERVER.TogglePlayerState(playerId);

    UpdatePlayerRow(playerId);
}

void dskHostGame::CI_NationChanged(const unsigned playerId, const Nation nation)
{
    ChangeNation(playerId, nation);
}

void dskHostGame::CI_TeamChanged(const unsigned playerId, const unsigned char team)
{
    ChangeTeam(playerId, team);
}

void dskHostGame::CI_ColorChanged(const unsigned playerId, const unsigned color)
{
    ChangeColor(playerId, color);
}

void dskHostGame::CI_PingChanged(const unsigned playerId, const unsigned short  /*ping*/)
{
    ChangePing(playerId);
}

void dskHostGame::CI_ReadyChanged(const unsigned playerId, const bool ready)
{
    ChangeReady(playerId, ready);
    // Event only called for other players (host ready is done in start game)
    // Also only for host and non-savegames
    if(ready && lua && GAMECLIENT.IsHost() && playerId != GAMECLIENT.GetPlayerId())
        lua->EventPlayerReady(playerId);
}

void dskHostGame::CI_PlayersSwapped(const unsigned player1, const unsigned player2)
{
    // Spieler wurden vertauscht, beide Reihen updaten
    UpdatePlayerRow(player1);
    UpdatePlayerRow(player2);
}

void dskHostGame::CI_GGSChanged(const GlobalGameSettings&  /*ggs*/)
{
    const GlobalGameSettings& ggs = gameLobby.GetSettings();

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
    //random location
    GetCtrl<ctrlCheck>(23)->SetCheck(ggs.randomStartPosition);

    TogglePlayerReady(GAMECLIENT.GetPlayerId(), false);
}

void dskHostGame::CI_Chat(const unsigned playerId, const ChatDestination  /*cd*/, const std::string& msg)
{
    if ((playerId != 0xFFFFFFFF) && !IsSinglePlayer())
    {
        std::string time = TIME.FormatTime("(%H:%i:%s)");

        gameChat->AddMessage(time, gameLobby.GetPlayer(playerId).name, gameLobby.GetPlayer(playerId).color, msg, 0xFFFFFF00);
        if(!gameChat->IsVisible())
            hasUnreadChat = true;
    }
}

void dskHostGame::CI_Error(const ClientError ce)
{
    switch(ce)
    {
        case CE_INCOMPLETEMESSAGE:
        case CE_CONNECTIONLOST:
        {
            // Verbindung zu Server abgebrochen
            WINDOWMANAGER.Show(new iwMsgbox(_("Error"), _("Lost connection to server!"), this, MSB_OK, MSB_EXCLAMATIONRED, 0));
        } break;
        default: break;
    }
}

/**
 *  (Lobby-)RankingInfo: Rankinginfo eines bestimmten Benutzers empfangen
 */
void dskHostGame::LC_RankingInfo(const LobbyPlayerInfo& player)
{
    for(unsigned i = 0; i < gameLobby.GetPlayerCount(); ++i)
    {
        if(gameLobby.GetPlayer(i).name == player.getName())
            gameLobby.GetPlayer(i).rating = player.getPunkte();
    }
}

/**
 *  (Lobby-)Status: Benutzerdefinierter Fehler (kann auch Conn-Loss o.ä sein)
 */
void dskHostGame::LC_Status_Error(const std::string& error)
{
    WINDOWMANAGER.Show(new iwMsgbox(_("Error"), error, this, MSB_OK, MSB_EXCLAMATIONRED, 0));
}

void dskHostGame::LC_Chat(const std::string& player, const std::string& text)
{
    if(!lobbyChat)
        return;
    lobbyChat->AddMessage("", player, ctrlChat::CalcUniqueColor(player), text, COLOR_YELLOW);
    if(!lobbyChat->IsVisible())
        hasUnreadChat = true;
}
