// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dskGameLobby.h"
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
#include "gameData/GameConsts.h"
#include "gameData/const_gui_ids.h"
#include "liblobby/LobbyPlayerInfo.h"
#include "libsiedler2/ArchivItem_Map.h"
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
template<typename T>
constexpr T nextEnumValue(T value)
{
    return T((rttr::enum_cast(value) + 1) % helpers::NumEnumValues_v<T>);
}
} // namespace

dskGameLobby::dskGameLobby(ServerType serverType, std::shared_ptr<GameLobby> gameLobby, unsigned playerId,
                           std::unique_ptr<ILobbyClient> lobbyClient)
    : Desktop(LOADER.GetImageN("setup015", 0)), serverType(serverType), gameLobby_(std::move(gameLobby)),
      localPlayerId_(playerId), lobbyClient_(std::move(lobbyClient)), hasCountdown_(false), wasActivated(false),
      gameChat(nullptr), lobbyChat(nullptr), lobbyChatTabAnimId(0), localChatTabAnimId(0)
{
    // If no lobby don't do anything else
    if(!gameLobby_)
        return;

    if(gameLobby_->isHost())
        lobbyHostController = std::make_unique<GameLobbyController>(gameLobby_, GAMECLIENT.GetMainPlayer());

    if(!GAMECLIENT.GetLuaFilePath().empty() && gameLobby_->isHost())
    {
        lua = std::make_unique<LuaInterfaceSettings>(*lobbyHostController, GAMECLIENT);
        if(!lua->loadScript(GAMECLIENT.GetLuaFilePath()))
        {
            WINDOWMANAGER.ShowAfterSwitch(std::make_unique<iwMsgbox>(
              _("Error"), _("Lua script was found but failed to load. Map might not work as expected!"), this,
              MsgboxButton::Ok, MsgboxIcon::ExclamationRed, 1));
            lua.reset();
        } else if(!lua->CheckScriptVersion())
        {
            WINDOWMANAGER.ShowAfterSwitch(std::make_unique<iwMsgbox>(
              _("Error"), _("Lua script uses a different version and cannot be used. Map might not work as expected!"),
              this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed, 1));
            lua.reset();
        } else if(!lua->EventSettingsInit(serverType == ServerType::Local, gameLobby_->isSavegame()))
        {
            RTTR_Assert(
              gameLobby_->isHost()); // This should be done first for the host so others won't even see the script
            LOG.write(_("Lua was disabled by the script itself\n"));
            lua.reset();
        }
        if(!lua)
            lobbyHostController->RemoveLuaScript();
    }

    const bool readonlySettings = !gameLobby_->isHost() || gameLobby_->isSavegame() || !IsChangeAllowed("general");
    allowAddonChange = gameLobby_->isHost() && !gameLobby_->isSavegame()
                       && (IsChangeAllowed("addonsAll") || IsChangeAllowed("addonsSome"));

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
    if(gameLobby_->isHost() && !gameLobby_->isSavegame())
        AddText(24, DrawPoint(0, 40), _("Swap"), COLOR_YELLOW, FontStyle::LEFT, NormalFont);
    // "Verschieben" (nur bei Savegames!)
    if(gameLobby_->isSavegame())
        AddText(17, DrawPoint(645, 40), _("Past player"), COLOR_YELLOW, FontStyle::CENTER, NormalFont);

    if(!IsSinglePlayer())
    {
        // Enable lobby chat when we are logged in
        if(lobbyClient_ && lobbyClient_->IsLoggedIn())
        {
            ctrlOptionGroup* chatTab = AddOptionGroup(ID_CHAT_TAB, GroupSelectType::Check);
            chatTab->AddTextButton(TAB_GAMECHAT, DrawPoint(20, 320), Extent(178, 22), TextureColor::Green2,
                                   _("Game Chat"), NormalFont);
            chatTab->AddTextButton(TAB_LOBBYCHAT, DrawPoint(202, 320), Extent(178, 22), TextureColor::Green2,
                                   _("Lobby Chat"), NormalFont);
            gameChat =
              AddChatCtrl(ID_GAME_CHAT, DrawPoint(20, 345), Extent(360, 218 - 25), TextureColor::Grey, NormalFont);
            lobbyChat =
              AddChatCtrl(ID_LOBBY_CHAT, DrawPoint(20, 345), Extent(360, 218 - 25), TextureColor::Grey, NormalFont);
            chatTab->SetSelection(TAB_GAMECHAT, true);
        } else
        {
            // Chatfenster
            gameChat = AddChatCtrl(ID_GAME_CHAT, DrawPoint(20, 320), Extent(360, 218), TextureColor::Grey, NormalFont);
        }
        // Edit für Chatfenster
        AddEdit(ID_CHAT_INPUT, DrawPoint(20, 540), Extent(360, 22), TextureColor::Grey, NormalFont);
    }

    // "Spiel starten"
    AddTextButton(2, DrawPoint(600, 560), Extent(180, 22), TextureColor::Green2,
                  (gameLobby_->isHost() ? _("Start game") : _("Ready")), NormalFont);

    // "Zurück"
    AddTextButton(3, DrawPoint(400, 560), Extent(180, 22), TextureColor::Red1, _("Return"), NormalFont);

    // "Teams sperren"
    AddCheckBox(20, DrawPoint(400, 460), Extent(180, 26), TextureColor::Grey, _("Lock teams:"), NormalFont,
                readonlySettings);
    // "Gemeinsame Team-Sicht"
    AddCheckBox(19, DrawPoint(600, 460), Extent(180, 26), TextureColor::Grey, _("Shared team view"), NormalFont,
                readonlySettings);
    // "Random Start Locations"
    AddCheckBox(23, DrawPoint(600, 430), Extent(180, 26), TextureColor::Grey, _("Random start locations"), NormalFont,
                readonlySettings);

    // "Enhancements"
    AddText(21, DrawPoint(400, 499), _("Addons:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddTextButton(22, DrawPoint(600, 495), Extent(180, 22), TextureColor::Green2,
                  allowAddonChange ? _("Change Settings...") : _("View Settings..."), NormalFont);

    ctrlComboBox* combo;

    // umgedrehte Reihenfolge, damit die Listen nicht dahinter sind

    // "Aufklärung"
    AddText(30, DrawPoint(400, 405), _("Exploration:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo =
      AddComboBox(40, DrawPoint(600, 400), Extent(180, 20), TextureColor::Grey, NormalFont, 100, readonlySettings);
    combo->AddString(_("Off (all visible)"));
    combo->AddString(_("Classic (Settlers 2)"));
    combo->AddString(_("Fog of War"));
    combo->AddString(_("FoW - all explored"));

    // "Waren zu Beginn"
    AddText(31, DrawPoint(400, 375), _("Goods at start:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo =
      AddComboBox(41, DrawPoint(600, 370), Extent(180, 20), TextureColor::Grey, NormalFont, 100, readonlySettings);
    combo->AddString(_("Very Low"));
    combo->AddString(_("Low"));
    combo->AddString(_("Normal"));
    combo->AddString(_("A lot"));

    // "Spielziel"
    AddText(32, DrawPoint(400, 345), _("Goals:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo =
      AddComboBox(42, DrawPoint(600, 340), Extent(180, 20), TextureColor::Grey, NormalFont, 100, readonlySettings);
    combo->AddString(_("None"));               // Kein Spielziel
    combo->AddString(_("Conquer 3/4 of map")); // Besitz 3/4 des Landes
    combo->AddString(_("Total domination"));   // Alleinherrschaft
    combo->AddString(_("Economy mode"));       // Wirtschaftsmodus

    // Lobby game?
    if(lobbyClient_ && lobbyClient_->IsLoggedIn())
    {
        // Then add tournament modes as possible "objectives"
        for(const unsigned duration : TOURNAMENT_MODES_DURATION)
            combo->AddString(helpers::format(_("Tournament: %u minutes"), duration));
    }

    // "Geschwindigkeit"
    AddText(33, DrawPoint(400, 315), _("Speed:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo =
      AddComboBox(43, DrawPoint(600, 310), Extent(180, 20), TextureColor::Grey, NormalFont, 100, !gameLobby_->isHost());
    combo->AddString(_("Very slow")); // Sehr Langsam
    combo->AddString(_("Slow"));      // Langsam
    combo->AddString(_("Normal"));    // Normal
    combo->AddString(_("Fast"));      // Schnell
    combo->AddString(_("Very fast")); // Sehr Schnell

    // Karte laden, um Kartenvorschau anzuzeigen
    if(!gameLobby_->isSavegame())
    {
        bool isMapPreviewEnabled = !lua || lua->IsMapPreviewEnabled();
        if(!isMapPreviewEnabled)
        {
            AddTextDeepening(70, DrawPoint(560, 40), Extent(220, 220), TextureColor::Grey, _("No preview"), LargeFont,
                             COLOR_YELLOW);
            AddText(71, DrawPoint(670, 40 + 220 + 10), _("Map: ") + GAMECLIENT.GetMapTitle(), COLOR_YELLOW,
                    FontStyle::CENTER, NormalFont);
        } else
        {
            // Map laden
            libsiedler2::Archiv mapArchiv;
            // Karteninformationen laden
            if(int ec = libsiedler2::loader::LoadMAP(GAMECLIENT.GetMapPath(), mapArchiv))
            {
                WINDOWMANAGER.ShowAfterSwitch(
                  std::make_unique<iwMsgbox>(_("Error"), _("Could not load map:\n") + libsiedler2::getErrorString(ec),
                                             this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed, 0));
            } else
            {
                auto* map = static_cast<libsiedler2::ArchivItem_Map*>(mapArchiv.get(0));
                ctrlPreviewMinimap* preview = AddPreviewMinimap(70, DrawPoint(560, 40), Extent(220, 220), map);

                // Titel der Karte, Y-Position relativ je nach Höhe der Minimap festlegen, daher nochmals danach
                // verschieben, da diese Position sonst skaliert wird!
                ctrlText* text = AddText(71, DrawPoint(670, 0), _("Map: ") + GAMECLIENT.GetMapTitle(), COLOR_YELLOW,
                                         FontStyle::CENTER, NormalFont);
                text->SetPos(DrawPoint(text->GetPos().x, preview->GetPos().y + preview->GetMapArea().bottom + 10));
            }
        }
    }

    if(GAMECLIENT.IsAIBattleModeOn())
    {
        const auto& aiBattlePlayers = GAMECLIENT.GetAIBattlePlayers();

        // Initialize AI battle players
        for(unsigned i = 0; i < gameLobby_->getNumPlayers(); i++)
        {
            if(i < aiBattlePlayers.size())
                lobbyHostController->SetPlayerState(i, PlayerState::AI, aiBattlePlayers[i]);
            else
                lobbyHostController->CloseSlot(i); // Close remaining slots
        }

        // Set name of host to the corresponding AI for local player
        if(localPlayerId_ < aiBattlePlayers.size())
            lobbyHostController->SetName(localPlayerId_,
                                         JoinPlayerInfo::MakeAIName(aiBattlePlayers[localPlayerId_], localPlayerId_));
    } else if(IsSinglePlayer() && !gameLobby_->isSavegame())
    {
        // Setze initial auf KI
        for(unsigned i = 0; i < gameLobby_->getNumPlayers(); i++)
        {
            if(!gameLobby_->getPlayer(i).isHost)
                lobbyHostController->SetPlayerState(i, PlayerState::AI, AI::Info(AI::Type::Default, AI::Level::Easy));
        }
    }

    // Alle Spielercontrols erstellen
    for(unsigned i = 0; i < gameLobby_->getNumPlayers(); i++)
        UpdatePlayerRow(i);
    // swap buttons erstellen
    if(gameLobby_->isHost() && !gameLobby_->isSavegame() && IsChangeAllowed("swapping"))
    {
        for(unsigned i = 0; i < gameLobby_->getNumPlayers(); i++)
        {
            DrawPoint rowPos = GetCtrl<Window>(ID_PLAYER_GROUP_START + i)->GetCtrl<Window>(1)->GetPos();
            ctrlButton* bt = AddTextButton(ID_SWAP_BUTTON + i, DrawPoint(5, 0), Extent(22, 22), TextureColor::Red1,
                                           _("-"), NormalFont);
            bt->SetPos(DrawPoint(bt->GetPos().x, rowPos.y));
        }
    }
    CI_GGSChanged(gameLobby_->getSettings());

    if(serverType == ServerType::Lobby && lobbyClient_ && lobbyClient_->IsLoggedIn())
    {
        lobbyClient_->AddListener(this);
        lobbyClient_->SendServerJoinRequest();
    }

    GAMECLIENT.SetInterface(this);
}

dskGameLobby::~dskGameLobby()
{
    if(lobbyClient_)
        lobbyClient_->RemoveListener(this);
    GAMECLIENT.RemoveInterface(this);
}

/**
 *  Größe ändern-Reaktionen die nicht vom Skaling-Mechanismus erfasst werden.
 */
void dskGameLobby::Resize(const Extent& newSize)
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

void dskGameLobby::SetActive(bool activate /*= true*/)
{
    Desktop::SetActive(activate);
    if(activate && !wasActivated && lua && gameLobby_->isHost())
    {
        wasActivated = true;
        try
        {
            lua->EventSettingsReady();
        } catch(LuaExecutionError&)
        {
            WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(
              _("Error"), _("Lua script was found but failed to load. Map might not work as expected!"), this,
              MsgboxButton::Ok, MsgboxIcon::ExclamationRed, 1));
            lua.reset();
        }
    }
}

void dskGameLobby::UpdatePlayerRow(const unsigned row)
{
    const JoinPlayerInfo& player = gameLobby_->getPlayer(row);

    unsigned cy = 80 + row * 30;
    TextureColor tc = (row & 1 ? TextureColor::Grey : TextureColor::Green2);

    // Alle Controls erstmal zerstören (die ganze Gruppe)
    DeleteCtrl(ID_PLAYER_GROUP_START + row);
    // und neu erzeugen
    ctrlGroup* group = AddGroup(ID_PLAYER_GROUP_START + row);

    std::string name;
    // Name
    switch(player.ps)
    {
        default: name.clear(); break;
        case PlayerState::Occupied:
        case PlayerState::AI: name = player.name; break;
        case PlayerState::Free: name = _("Open"); break;
        case PlayerState::Locked: name = _("Closed"); break;
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
    if(gameLobby_->isHost() && !player.isHost && IsChangeAllowed("playerState"))
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
        const bool allowPlayerChange = ((gameLobby_->isHost() && player.ps == PlayerState::AI) || localPlayerId_ == row)
                                       && !gameLobby_->isSavegame();
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
            group->AddTextButton(3, DrawPoint(240, cy), Extent(90, 22), tc, _(NationNames[Nation::Romans]), NormalFont);
        else
            group->AddTextDeepening(3, DrawPoint(240, cy), Extent(90, 22), tc, _(NationNames[Nation::Romans]),
                                    NormalFont, COLOR_YELLOW);

        if(allowColorChange)
            group->AddColorButton(4, DrawPoint(340, cy), Extent(30, 22), tc, 0);
        else
            group->AddColorDeepening(4, DrawPoint(340, cy), Extent(30, 22), tc, 0);

        if(allowTeamChange)
            group->AddTextButton(5, DrawPoint(380, cy), Extent(50, 22), tc, _("-"), NormalFont);
        else
            group->AddTextDeepening(5, DrawPoint(380, cy), Extent(50, 22), tc, _("-"), NormalFont, COLOR_YELLOW);

        // Bereit (nicht bei KIs und Host)
        if(player.ps == PlayerState::Occupied && !player.isHost)
            group->AddCheckBox(6, DrawPoint(450, cy), Extent(22, 22), tc, "", nullptr, (localPlayerId_ != row));

        // Ping ( "%d" )
        ctrlVarDeepening* ping = group->AddVarDeepening(7, DrawPoint(490, cy), Extent(50, 22), tc, _("%d"), NormalFont,
                                                        COLOR_YELLOW, 1, &player.ping); //-V111

        // Verschieben (nur bei Savegames und beim Host!)
        if(gameLobby_->isSavegame() && player.ps == PlayerState::Occupied)
        {
            ctrlComboBox* combo =
              group->AddComboBox(8, DrawPoint(560, cy), Extent(160, 22), tc, NormalFont, 150, !gameLobby_->isHost());

            // Mit den alten Namen füllen
            for(unsigned i = 0; i < gameLobby_->getNumPlayers(); ++i)
            {
                if(!gameLobby_->getPlayer(i).originName.empty())
                {
                    combo->AddString(gameLobby_->getPlayer(i).originName);
                    if(i == row)
                        combo->SetSelection(combo->GetNumItems() - 1u);
                }
            }
        }

        // Hide ping for AIs or on single player games
        if(player.ps == PlayerState::AI || IsSinglePlayer())
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
void dskGameLobby::Msg_PaintBefore()
{
    Desktop::Msg_PaintBefore();
    // Chatfenster Fokus geben
    if(!IsSinglePlayer())
        GetCtrl<ctrlEdit>(ID_CHAT_INPUT)->SetFocus();
}

void dskGameLobby::Msg_Group_ButtonClick(const unsigned group_id, const unsigned ctrl_id)
{
    unsigned playerId = group_id - ID_PLAYER_GROUP_START;

    switch(ctrl_id)
    {
        // Klick auf Spielername
        case 1:
        {
            if(gameLobby_->isHost())
                lobbyHostController->TogglePlayerState(playerId);
        }
        break;
        // Volk
        case 3:
        {
            SetPlayerReady(playerId, false);

            if(playerId == localPlayerId_ || gameLobby_->isHost())
            {
                JoinPlayerInfo& player = gameLobby_->getPlayer(playerId);
                player.nation = nextEnumValue(player.nation);
                if(gameLobby_->isHost())
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

            if(playerId == localPlayerId_ || gameLobby_->isHost())
            {
                // Get colors used by other players
                std::set<unsigned> takenColors;
                for(unsigned p = 0; p < gameLobby_->getNumPlayers(); ++p)
                {
                    // Skip self
                    if(p == playerId)
                        continue;

                    const JoinPlayerInfo& otherPlayer = gameLobby_->getPlayer(p);
                    if(otherPlayer.isUsed())
                        takenColors.insert(otherPlayer.color);
                }

                // Look for a unique color
                JoinPlayerInfo& player = gameLobby_->getPlayer(playerId);
                int newColorIdx = JoinPlayerInfo::GetColorIdx(player.color);
                do
                {
                    player.color = PLAYER_COLORS[(++newColorIdx) % PLAYER_COLORS.size()];
                } while(helpers::contains(takenColors, player.color));

                if(gameLobby_->isHost())
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

            if(playerId == localPlayerId_ || gameLobby_->isHost())
            {
                JoinPlayerInfo& player = gameLobby_->getPlayer(playerId);
                player.team = nextEnumValue(player.team);
                if(gameLobby_->isHost())
                    lobbyHostController->SetTeam(playerId, player.team);
                else
                    GAMECLIENT.Command_SetTeam(player.team);
                ChangeTeam(playerId, player.team);
            }
        }
        break;
    }
}

void dskGameLobby::Msg_Group_CheckboxChange(const unsigned group_id, const unsigned /*ctrl_id*/, const bool checked)
{
    unsigned playerId = group_id - ID_PLAYER_GROUP_START;

    // Bereit
    if(playerId < MAX_PLAYERS)
        SetPlayerReady(playerId, checked);
}

void dskGameLobby::Msg_Group_ComboSelectItem(const unsigned group_id, const unsigned /*ctrl_id*/,
                                             const unsigned selection)
{
    if(!gameLobby_->isHost())
        return;
    // Swap players
    const unsigned playerId = group_id - ID_PLAYER_GROUP_START;

    int player2 = -1;
    for(unsigned i = 0, playerCtr = 0; i < gameLobby_->getNumPlayers(); ++i)
    {
        if(!gameLobby_->getPlayer(i).originName.empty() && playerCtr++ == selection)
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

void dskGameLobby::GoBack()
{
    if(IsSinglePlayer())
        WINDOWMANAGER.Switch(std::make_unique<dskSinglePlayer>());
    else if(serverType == ServerType::LAN)
        WINDOWMANAGER.Switch(std::make_unique<dskLAN>());
    else if(serverType == ServerType::Lobby && lobbyClient_ && lobbyClient_->IsLoggedIn())
        WINDOWMANAGER.Switch(std::make_unique<dskLobby>());
    else
        WINDOWMANAGER.Switch(std::make_unique<dskDirectIP>());
}

bool dskGameLobby::IsChangeAllowed(const std::string& setting) const
{
    return !lua || lua->IsChangeAllowed(setting);
}

void dskGameLobby::Msg_ButtonClick(const unsigned ctrl_id)
{
    if(ctrl_id >= ID_SWAP_BUTTON && ctrl_id < ID_SWAP_BUTTON + MAX_PLAYERS)
    {
        unsigned targetPlayer = ctrl_id - ID_SWAP_BUTTON;
        if(targetPlayer != localPlayerId_ && gameLobby_->isHost())
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
            if(gameLobby_->isHost())
            {
                if(!checkOptions())
                    return;

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
                if(allowAddonChange && lua->IsChangeAllowed("addonsAll"))
                    w = std::make_unique<iwAddons>(gameLobby_->getSettings(), this, AddonChangeAllowed::All);
                else if(allowAddonChange)
                    w = std::make_unique<iwAddons>(gameLobby_->getSettings(), this, AddonChangeAllowed::WhitelistOnly,
                                                   lua->GetAllowedAddons());
                else
                    w = std::make_unique<iwAddons>(gameLobby_->getSettings(), this, AddonChangeAllowed::None);
                WINDOWMANAGER.Show(std::move(w));
            }
        }
        break;
    }
}

void dskGameLobby::Msg_EditEnter(const unsigned ctrl_id)
{
    if(ctrl_id != ID_CHAT_INPUT)
        return;
    auto* edit = GetCtrl<ctrlEdit>(ctrl_id);
    const std::string msg = edit->GetText();
    edit->SetText("");
    if(gameChat->IsVisible())
        GAMECLIENT.Command_Chat(msg, ChatDestination::All);
    else if(lobbyClient_ && lobbyClient_->IsLoggedIn() && lobbyChat->IsVisible())
        lobbyClient_->SendChat(msg);
}

void dskGameLobby::CI_Countdown(unsigned remainingTimeInSec)
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

    const std::string message =
      (remainingTimeInSec > 0) ? " " + std::to_string(remainingTimeInSec) : _("Starting game, please wait");

    gameChat->AddMessage("", "", 0, message, 0xFFFFBB00);
}

void dskGameLobby::CI_CancelCountdown(bool error)
{
    if(hasCountdown_)
    {
        hasCountdown_ = false;
        gameChat->AddMessage("", "", 0xFFCC2222, _("Start aborted"), 0xFFFFCC00);
        FlashGameChat();
    }

    if(gameLobby_->isHost())
    {
        if(error)
        {
            WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(
              _("Error"),
              _("Game can only be started as soon as everybody has a unique color,everyone is "
                "ready and all free slots are closed."),
              this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed, 10));
        }

        ChangeReady(localPlayerId_, true);
    }
}

void dskGameLobby::FlashGameChat()
{
    if(!gameChat->IsVisible())
    {
        auto* tab = GetCtrl<Window>(ID_CHAT_TAB);
        auto* bt = tab->GetCtrl<ctrlButton>(TAB_GAMECHAT);
        if(!localChatTabAnimId)
            localChatTabAnimId = tab->GetAnimationManager().addAnimation(new BlinkButtonAnim(bt));
    }
}

void dskGameLobby::Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult mbr)
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
            if(mbr == MsgboxResult::Yes)
                UpdateGGS();
        }
        break;
        case 10: // Economy Mode - change Addon Setttings
        {
            if(mbr == MsgboxResult::Yes)
            {
                gameLobby_->getSettings().setSelection(AddonId::PEACEFULMODE, true);
                gameLobby_->getSettings().setSelection(AddonId::NO_COINS_DEFAULT, true);
                gameLobby_->getSettings().setSelection(AddonId::LIMIT_CATAPULTS, 2);
                UpdateGGS();
            } else if(mbr == MsgboxResult::No)
            {
                forceOptions = true;
                Msg_ButtonClick(2);
            }
        }
        break;
        case 11: // Peaceful mode still active but we have an attack based victory condition
        {
            if(mbr == MsgboxResult::Yes)
            {
                gameLobby_->getSettings().setSelection(AddonId::PEACEFULMODE, false);
            } else if(mbr == MsgboxResult::No)
            {
                forceOptions = true;
                Msg_ButtonClick(2);
            }
        }
        break;
    }
}

void dskGameLobby::Msg_ComboSelectItem(const unsigned ctrl_id, const unsigned /*selection*/)
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

void dskGameLobby::Msg_CheckboxChange(const unsigned ctrl_id, const bool /*checked*/)
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

void dskGameLobby::Msg_OptionGroupChange(const unsigned ctrl_id, const unsigned selection)
{
    if(ctrl_id == ID_CHAT_TAB)
    {
        gameChat->SetVisible(selection == TAB_GAMECHAT);
        lobbyChat->SetVisible(selection == TAB_LOBBYCHAT);
        auto* tab = GetCtrl<Window>(ID_CHAT_TAB);
        tab->GetCtrl<ctrlButton>(selection)->SetTexture(TextureColor::Green2);
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

void dskGameLobby::UpdateGGS()
{
    RTTR_Assert(gameLobby_->isHost());

    GlobalGameSettings& ggs = gameLobby_->getSettings();

    // Geschwindigkeit
    ggs.speed = static_cast<GameSpeed>(GetCtrl<ctrlComboBox>(43)->GetSelection().get());
    // Spielziel
    ggs.objective = static_cast<GameObjective>(GetCtrl<ctrlComboBox>(42)->GetSelection().get());
    // Waren zu Beginn
    ggs.startWares = static_cast<StartWares>(GetCtrl<ctrlComboBox>(41)->GetSelection().get());
    // Aufklärung
    ggs.exploration = static_cast<Exploration>(GetCtrl<ctrlComboBox>(40)->GetSelection().get());
    // Teams gesperrt
    ggs.lockedTeams = GetCtrl<ctrlCheck>(20)->isChecked();
    // Team sicht
    ggs.teamView = GetCtrl<ctrlCheck>(19)->isChecked();
    // random locations
    ggs.randomStartPosition = GetCtrl<ctrlCheck>(23)->isChecked();

    // An Server übermitteln
    lobbyHostController->ChangeGlobalGameSettings(ggs);
}

void dskGameLobby::ChangeTeam(const unsigned player, const Team team)
{
    constexpr helpers::EnumArray<const char*, Team> teams = {"-", "?", "1", "2", "3", "4", "1-2", "1-3", "1-4"};

    GetCtrl<ctrlGroup>(ID_PLAYER_GROUP_START + player)->GetCtrl<ctrlBaseText>(5)->SetText(teams[team]);
}

void dskGameLobby::ChangeReady(const unsigned player, const bool ready)
{
    auto* check = GetCtrl<ctrlGroup>(ID_PLAYER_GROUP_START + player)->GetCtrl<ctrlCheck>(6);
    if(check)
        check->setChecked(ready);

    if(player == localPlayerId_)
    {
        auto* start = GetCtrl<ctrlTextButton>(2);
        if(gameLobby_->isHost())
            start->SetText(hasCountdown_ ? _("Cancel start") : _("Start game"));
        else
            start->SetText(ready ? _("Not Ready") : _("Ready"));
    }
}

void dskGameLobby::ChangeNation(const unsigned player, const Nation nation)
{
    GetCtrl<ctrlGroup>(ID_PLAYER_GROUP_START + player)->GetCtrl<ctrlBaseText>(3)->SetText(_(NationNames[nation]));
}

void dskGameLobby::ChangePing(unsigned playerId)
{
    unsigned color = COLOR_RED;

    // Farbe bestimmen
    if(gameLobby_->getPlayer(playerId).ping < 300)
        color = COLOR_GREEN;
    else if(gameLobby_->getPlayer(playerId).ping < 800)
        color = COLOR_YELLOW;

    // und setzen
    GetCtrl<ctrlGroup>(ID_PLAYER_GROUP_START + playerId)->GetCtrl<ctrlVarDeepening>(7)->SetTextColor(color);
}

void dskGameLobby::ChangeColor(const unsigned player, const unsigned color)
{
    GetCtrl<ctrlGroup>(ID_PLAYER_GROUP_START + player)->GetCtrl<ctrlBaseColor>(4)->SetColor(color);

    // Minimap-Startfarbe ändern
    if(GetCtrl<ctrlPreviewMinimap>(70))
        GetCtrl<ctrlPreviewMinimap>(70)->SetPlayerColor(player, color);
}

void dskGameLobby::SetPlayerReady(unsigned char player, bool ready)
{
    if(player != localPlayerId_)
        return;
    if(gameLobby_->isHost())
        ready = true;
    if(gameLobby_->getPlayer(player).isReady != ready)
    {
        gameLobby_->getPlayer(player).isReady = ready;
        GAMECLIENT.Command_SetReady(ready);
    }
    ChangeReady(player, ready);
}

void dskGameLobby::CI_NewPlayer(const unsigned playerId)
{
    // Spielername setzen
    UpdatePlayerRow(playerId);

    if(lua && gameLobby_->isHost())
        lua->EventPlayerJoined(playerId);
}

void dskGameLobby::CI_PlayerLeft(const unsigned playerId)
{
    UpdatePlayerRow(playerId);
    if(lua && gameLobby_->isHost())
        lua->EventPlayerLeft(playerId);
}

void dskGameLobby::CI_GameLoading(std::shared_ptr<Game> game)
{
    // Desktop wechseln
    WINDOWMANAGER.Switch(std::make_unique<dskGameLoader>(std::move(game)));
}

void dskGameLobby::CI_PlayerDataChanged(unsigned playerId)
{
    UpdatePlayerRow(playerId);
}

void dskGameLobby::CI_PingChanged(const unsigned playerId, const unsigned short /*ping*/)
{
    ChangePing(playerId);
}

void dskGameLobby::CI_ReadyChanged(const unsigned playerId, const bool ready)
{
    ChangeReady(playerId, ready);
    // Event only called for other players (host ready is done in start game)
    // Also only for host and non-savegames
    if(ready && lua && gameLobby_->isHost() && playerId != localPlayerId_)
        lua->EventPlayerReady(playerId);
}

void dskGameLobby::CI_PlayersSwapped(const unsigned player1, const unsigned player2)
{
    if(player1 == localPlayerId_)
        localPlayerId_ = player2;
    else if(localPlayerId_ == player2)
        localPlayerId_ = player1;
    // Spieler wurden vertauscht, beide Reihen updaten
    UpdatePlayerRow(player1);
    UpdatePlayerRow(player2);
}

void dskGameLobby::CI_GGSChanged(const GlobalGameSettings& /*ggs*/)
{
    const GlobalGameSettings& ggs = gameLobby_->getSettings();

    // Geschwindigkeit
    GetCtrl<ctrlComboBox>(43)->SetSelection(static_cast<unsigned short>(ggs.speed));
    // Ziel
    GetCtrl<ctrlComboBox>(42)->SetSelection(static_cast<unsigned short>(ggs.objective));
    // Waren
    GetCtrl<ctrlComboBox>(41)->SetSelection(static_cast<unsigned short>(ggs.startWares));
    // Aufklärung
    GetCtrl<ctrlComboBox>(40)->SetSelection(static_cast<unsigned short>(ggs.exploration));
    // Teams
    GetCtrl<ctrlCheck>(20)->setChecked(ggs.lockedTeams);
    // Team-Sicht
    GetCtrl<ctrlCheck>(19)->setChecked(ggs.teamView);
    // random location
    GetCtrl<ctrlCheck>(23)->setChecked(ggs.randomStartPosition);

    SetPlayerReady(localPlayerId_, false);
}

void dskGameLobby::CI_Chat(const unsigned playerId, const ChatDestination /*cd*/, const std::string& msg)
{
    if((playerId != 0xFFFFFFFF) && !IsSinglePlayer())
    {
        std::string time = s25util::Time::FormatTime("(%H:%i:%s)");

        gameChat->AddMessage(time, gameLobby_->getPlayer(playerId).name, gameLobby_->getPlayer(playerId).color, msg,
                             0xFFFFFF00); //-V810
        FlashGameChat();
    }
}

void dskGameLobby::CI_Error(const ClientError ce)
{
    WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Error"), ClientErrorToStr(ce), this, MsgboxButton::Ok,
                                                  MsgboxIcon::ExclamationRed, 0));
}

/**
 *  (Lobby-)Status: Benutzerdefinierter Fehler (kann auch Conn-Loss o.ä sein)
 */
void dskGameLobby::LC_Status_Error(const std::string& error)
{
    WINDOWMANAGER.Show(
      std::make_unique<iwMsgbox>(_("Error"), error, this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed, 0));
}

void dskGameLobby::LC_Chat(const std::string& player, const std::string& text)
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

bool dskGameLobby::checkOptions()
{
    if(forceOptions)
        return true;
    const GlobalGameSettings& ggs = gameLobby_->getSettings();
    if(ggs.objective == GameObjective::EconomyMode && !ggs.isEnabled(AddonId::PEACEFULMODE))
    {
        WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(
          _("Economy mode"),
          _("You chose the economy mode. In economy mode the player or team that collects the most of certain goods "
            "wins (check the Economic progress window in game).\n\n"
            "Some players like to play this objective in peaceful mode. Would you like to adjust settings for a "
            "peaceful game?\n"
            "Choosing Yes will activate peaceful mode, ban catapults and disable buildings receiving coins by default. "
            "After clicking Yes you will be able to review the changes and then start the game by clicking the Start "
            "game button again.\n"
            "Choosing No will start the game without any changes."),
          this, MsgboxButton::YesNoCancel, MsgboxIcon::QuestionGreen, 10));
        return false;
    } else if(ggs.isEnabled(AddonId::PEACEFULMODE)
              && (ggs.objective == GameObjective::Conquer3_4 || ggs.objective == GameObjective::TotalDomination))
    {
        WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(
          _("Peaceful mode"),
          _("You chose a war based victory condition but peaceful mode is still active. Would you like to deactivate "
            "peaceful mode before you start? Choosing No will start the game, Yes will let you review the changes."),
          this, MsgboxButton::YesNoCancel, MsgboxIcon::QuestionRed, 11));
        return false;
    }
    return true;
}
