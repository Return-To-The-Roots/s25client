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
#include "controls/ctrlImageButton.h"
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
#include "gameData/PortraitConsts.h"
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
    ID_btStartGame,
    ID_btReturn,
    ID_chkLockTeams,
    ID_chkSharedView,
    ID_chkRandomSpawn,
    ID_txtAddons,
    ID_btSettings,
    ID_txtColPastPlayer,
    ID_txtColSwap,
    ID_txtColName,
    ID_txtColRace,
    ID_txtColColor,
    ID_txtColTeam,
    ID_txtColReady,
    ID_txtColPing,
    ID_txtGameName,
    ID_txtExploration,
    ID_cbExploration,
    ID_txtGoods,
    ID_cbGoods,
    ID_txtGoals,
    ID_cbGoals,
    ID_txtSpeed,
    ID_cbSpeed,
    ID_txtNoPreview,
    ID_txtMapName,
    ID_miniMap,
    ID_btPlayerState,
    ID_btNation,
    ID_btPortrait,
    ID_btColor,
    ID_btTeam,
    ID_chkReady,
    ID_txtPing,
    ID_cbMove,
    ID_mbLuaLoadError,
    ID_mbLuaVersionError,
    ID_mbMapLoadError,
    ID_mbError,
    ID_mbStartErrror,
    ID_mbQuestionEconomy,
    ID_mbQuestionPeaceful,
    ID_chatGame,
    ID_chatLobby,
    ID_edtChatMsg,
    ID_optChatTab,
    ID_btChatGame,
    ID_btChatLobby,
    ID_grpPlayerStart,                           // up to and including ID_grpPlayerStart + MAX_PLAYERS - 1
    ID_btSwap = ID_grpPlayerStart + MAX_PLAYERS, // up to and including ID_btSwap + MAX_PLAYERS - 1
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

    const bool loadLua = !GAMECLIENT.GetLuaFilePath().empty();

    // The lobby controller for clients is only used by lua
    if(gameLobby_->isHost() || loadLua)
        lobbyController = std::make_unique<GameLobbyController>(gameLobby_, GAMECLIENT.GetMainPlayer());

    if(loadLua)
    {
        lua = std::make_unique<LuaInterfaceSettings>(*lobbyController, GAMECLIENT);
        if(!lua->loadScript(GAMECLIENT.GetLuaFilePath()))
        {
            WINDOWMANAGER.ShowAfterSwitch(std::make_unique<iwMsgbox>(
              _("Error"), _("Lua script was found but failed to load. Map might not work as expected!"), this,
              MsgboxButton::Ok, MsgboxIcon::ExclamationRed, ID_mbLuaLoadError));
            lua.reset();
        } else if(!lua->CheckScriptVersion())
        {
            WINDOWMANAGER.ShowAfterSwitch(std::make_unique<iwMsgbox>(
              _("Error"), _("Lua script uses a different version and cannot be used. Map might not work as expected!"),
              this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed, ID_mbLuaVersionError));
            lua.reset();
        } else if(!lua->EventSettingsInit(serverType == ServerType::Local, gameLobby_->isSavegame()))
        {
            // This should have been detected for the host so others won't even see the script
            RTTR_Assert(gameLobby_->isHost());
            LOG.write(_("Lua was disabled by the script itself\n"));
            lua.reset();
        }
        if(!lua && gameLobby_->isHost())
            lobbyController->RemoveLuaScript();
    }

    const bool readonlySettings = !gameLobby_->isHost() || gameLobby_->isSavegame() || !IsChangeAllowed("general");
    allowAddonChange = gameLobby_->isHost() && !gameLobby_->isSavegame()
                       && (IsChangeAllowed("addonsAll") || IsChangeAllowed("addonsSome"));

    AddText(ID_txtGameName, DrawPoint(400, 5), GAMECLIENT.GetGameName(), COLOR_YELLOW, FontStyle::CENTER, LargeFont);

    AddText(ID_txtColName, DrawPoint(125, 40), _("Player Name"), COLOR_YELLOW, FontStyle::CENTER, NormalFont);
    AddText(ID_txtColRace, DrawPoint(262, 40), _("Race"), COLOR_YELLOW, FontStyle::CENTER, NormalFont);
    AddText(ID_txtColColor, DrawPoint(369, 40), _("Color"), COLOR_YELLOW, FontStyle::CENTER, NormalFont);
    AddText(ID_txtColTeam, DrawPoint(419, 40), _("Team"), COLOR_YELLOW, FontStyle::CENTER, NormalFont);

    if(!IsSinglePlayer())
    {
        AddText(ID_txtColReady, DrawPoint(479, 40), _("Ready?"), COLOR_YELLOW, FontStyle::CENTER, NormalFont);
        AddText(ID_txtColPing, DrawPoint(530, 40), _("Ping"), COLOR_YELLOW, FontStyle::CENTER, NormalFont);
    }
    if(gameLobby_->isHost() && !gameLobby_->isSavegame())
        AddText(ID_txtColSwap, DrawPoint(0, 40), _("Swap"), COLOR_YELLOW, FontStyle::LEFT, NormalFont);
    if(gameLobby_->isSavegame())
        AddText(ID_txtColPastPlayer, DrawPoint(645, 40), _("Past player"), COLOR_YELLOW, FontStyle::CENTER, NormalFont);

    if(!IsSinglePlayer())
    {
        // Enable lobby chat when we are logged in
        if(lobbyClient_ && lobbyClient_->IsLoggedIn())
        {
            ctrlOptionGroup* chatTab = AddOptionGroup(ID_optChatTab, GroupSelectType::Check);
            chatTab->AddTextButton(ID_btChatGame, DrawPoint(20, 320), Extent(178, 22), TextureColor::Green2,
                                   _("Game Chat"), NormalFont);
            chatTab->AddTextButton(ID_btChatLobby, DrawPoint(202, 320), Extent(178, 22), TextureColor::Green2,
                                   _("Lobby Chat"), NormalFont);
            gameChat =
              AddChatCtrl(ID_chatGame, DrawPoint(20, 345), Extent(360, 218 - 25), TextureColor::Grey, NormalFont);
            lobbyChat =
              AddChatCtrl(ID_chatLobby, DrawPoint(20, 345), Extent(360, 218 - 25), TextureColor::Grey, NormalFont);
            chatTab->SetSelection(ID_btChatGame, true);
        } else
        {
            gameChat = AddChatCtrl(ID_chatGame, DrawPoint(20, 320), Extent(360, 218), TextureColor::Grey, NormalFont);
        }
        AddEdit(ID_edtChatMsg, DrawPoint(20, 540), Extent(360, 22), TextureColor::Grey, NormalFont);
    }

    AddTextButton(ID_btStartGame, DrawPoint(600, 560), Extent(180, 22), TextureColor::Green2,
                  (gameLobby_->isHost() ? _("Start game") : _("Ready")), NormalFont);

    AddTextButton(ID_btReturn, DrawPoint(400, 560), Extent(180, 22), TextureColor::Red1, _("Return"), NormalFont);

    AddCheckBox(ID_chkLockTeams, DrawPoint(400, 460), Extent(180, 26), TextureColor::Grey, _("Lock teams:"), NormalFont,
                readonlySettings);
    AddCheckBox(ID_chkSharedView, DrawPoint(600, 460), Extent(180, 26), TextureColor::Grey, _("Shared team view"),
                NormalFont, readonlySettings);
    AddCheckBox(ID_chkRandomSpawn, DrawPoint(600, 430), Extent(180, 26), TextureColor::Grey,
                _("Random start locations"), NormalFont, readonlySettings);

    AddText(ID_txtAddons, DrawPoint(400, 499), _("Addons:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddTextButton(ID_btSettings, DrawPoint(600, 495), Extent(180, 22), TextureColor::Green2,
                  allowAddonChange ? _("Change Settings...") : _("View Settings..."), NormalFont);

    ctrlComboBox* combo;

    // umgedrehte Reihenfolge, damit die Listen nicht dahinter sind

    AddText(ID_txtExploration, DrawPoint(400, 405), _("Exploration:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo = AddComboBox(ID_cbExploration, DrawPoint(600, 400), Extent(180, 20), TextureColor::Grey, NormalFont, 100,
                        readonlySettings);
    combo->AddString(_("Off (all visible)"));
    combo->AddString(_("Classic (Settlers 2)"));
    combo->AddString(_("Fog of War"));
    combo->AddString(_("FoW - all explored"));

    AddText(ID_txtGoods, DrawPoint(400, 375), _("Goods at start:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo = AddComboBox(ID_cbGoods, DrawPoint(600, 370), Extent(180, 20), TextureColor::Grey, NormalFont, 100,
                        readonlySettings);
    combo->AddString(_("Very Low"));
    combo->AddString(_("Low"));
    combo->AddString(_("Normal"));
    combo->AddString(_("A lot"));

    AddText(ID_txtGoals, DrawPoint(400, 345), _("Goals:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo = AddComboBox(ID_cbGoals, DrawPoint(600, 340), Extent(180, 20), TextureColor::Grey, NormalFont, 100,
                        readonlySettings);
    combo->AddString(_("None"));
    combo->AddString(_("Conquer 3/4 of map"));
    combo->AddString(_("Total domination"));
    combo->AddString(_("Economy mode"));

    // Lobby game?
    if(lobbyClient_ && lobbyClient_->IsLoggedIn())
    {
        // Then add tournament modes as possible "objectives"
        for(const unsigned duration : TOURNAMENT_MODES_DURATION)
            combo->AddString(helpers::format(_("Tournament: %u minutes"), duration));
    }

    AddText(ID_txtSpeed, DrawPoint(400, 315), _("Speed:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo = AddComboBox(ID_cbSpeed, DrawPoint(600, 310), Extent(180, 20), TextureColor::Grey, NormalFont, 100,
                        !gameLobby_->isHost());
    combo->AddString(_("Very slow"));
    combo->AddString(_("Slow"));
    combo->AddString(_("Normal"));
    combo->AddString(_("Fast"));
    combo->AddString(_("Very fast"));

    // Karte laden, um Kartenvorschau anzuzeigen
    if(!gameLobby_->isSavegame())
    {
        const bool isMapPreviewEnabled = !lua || lua->IsMapPreviewEnabled();
        if(!isMapPreviewEnabled)
        {
            AddTextDeepening(ID_txtNoPreview, DrawPoint(560, 40), Extent(220, 220), TextureColor::Grey, _("No preview"),
                             LargeFont, COLOR_YELLOW);
            AddText(ID_txtMapName, DrawPoint(670, 40 + 220 + 10), _("Map: ") + GAMECLIENT.GetMapTitle(), COLOR_YELLOW,
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
                                             this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed, ID_mbMapLoadError));
            } else
            {
                auto* map = static_cast<libsiedler2::ArchivItem_Map*>(mapArchiv.get(0));
                ctrlPreviewMinimap* preview = AddPreviewMinimap(ID_miniMap, DrawPoint(560, 40), Extent(220, 220), map);

                // Titel der Karte, Y-Position relativ je nach Höhe der Minimap festlegen, daher nochmals danach
                // verschieben, da diese Position sonst skaliert wird!
                ctrlText* text = AddText(ID_txtMapName, DrawPoint(670, 0), _("Map: ") + GAMECLIENT.GetMapTitle(),
                                         COLOR_YELLOW, FontStyle::CENTER, NormalFont);
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
                lobbyController->SetPlayerState(i, PlayerState::AI, aiBattlePlayers[i]);
            else
                lobbyController->CloseSlot(i); // Close remaining slots
        }

        // Set name of host to the corresponding AI for local player
        if(localPlayerId_ < aiBattlePlayers.size())
            lobbyController->SetName(localPlayerId_,
                                     JoinPlayerInfo::MakeAIName(aiBattlePlayers[localPlayerId_], localPlayerId_));
    } else if(IsSinglePlayer() && !gameLobby_->isSavegame())
    {
        // Setze initial auf KI
        for(unsigned i = 0; i < gameLobby_->getNumPlayers(); i++)
        {
            if(!gameLobby_->getPlayer(i).isHost)
                lobbyController->SetPlayerState(i, PlayerState::AI, AI::Info(AI::Type::Default, AI::Level::Easy));
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
            int rowPos = GetCtrl<Window>(ID_grpPlayerStart + i)->GetCtrl<Window>(ID_btPlayerState)->GetPos().y;
            ctrlButton* bt =
              AddTextButton(ID_btSwap + i, DrawPoint(5, 0), Extent(22, 22), TextureColor::Red1, _("-"), NormalFont);
            bt->SetPos(DrawPoint(bt->GetPos().x, rowPos));
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
    auto* preview = GetCtrl<ctrlPreviewMinimap>(ID_miniMap);
    auto* text = GetCtrl<ctrlText>(ID_txtMapName);
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
        } catch(const LuaExecutionError&)
        {
            WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(
              _("Error"), _("Lua script was found but failed to load. Map might not work as expected!"), this,
              MsgboxButton::Ok, MsgboxIcon::ExclamationRed, ID_mbLuaLoadError));
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
    DeleteCtrl(ID_grpPlayerStart + row);
    // und neu erzeugen
    ctrlGroup* group = AddGroup(ID_grpPlayerStart + row);

    std::string name;
    switch(player.ps)
    {
        default: name.clear(); break;
        case PlayerState::Occupied:
        case PlayerState::AI: name = player.name; break;
        case PlayerState::Free: name = _("Open"); break;
        case PlayerState::Locked: name = _("Closed"); break;
    }

    if(GetCtrl<ctrlPreviewMinimap>(ID_miniMap))
    {
        if(player.isUsed())
            // Nur KIs und richtige Spieler haben eine Farbe auf der Karte
            GetCtrl<ctrlPreviewMinimap>(ID_miniMap)->SetPlayerColor(row, player.color);
        else
            // Keine richtigen Spieler --> Startposition auf der Karte ausblenden
            GetCtrl<ctrlPreviewMinimap>(ID_miniMap)->SetPlayerColor(row, 0);
    }

    // Spielername, beim Hosts Spielerbuttons, aber nich beim ihm selber, er kann sich ja nich selber kicken!
    if(gameLobby_->isHost() && !player.isHost && IsChangeAllowed("playerState"))
        group->AddTextButton(ID_btPlayerState, DrawPoint(30, cy), Extent(180, 22), tc, name, NormalFont);
    else
        group->AddTextDeepening(ID_btPlayerState, DrawPoint(30, cy), Extent(180, 22), tc, name, NormalFont,
                                COLOR_YELLOW);
    auto* text = group->GetCtrl<ctrlBaseText>(ID_btPlayerState);

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
            group->AddTextButton(ID_btNation, DrawPoint(215, cy), Extent(95, 22), tc, _(NationNames[Nation::Romans]),
                                 NormalFont);
        else
            group->AddTextDeepening(ID_btNation, DrawPoint(215, cy), Extent(95, 22), tc, _(NationNames[Nation::Romans]),
                                    NormalFont, COLOR_YELLOW);

        const auto& portrait = Portraits[player.portraitIndex];
        group->AddImageButton(ID_btPortrait, DrawPoint(315, cy), Extent(34, 22), tc,
                              LOADER.GetImageN(portrait.resourceId, portrait.resourceIndex), _(portrait.name));

        if(allowColorChange)
            group->AddColorButton(ID_btColor, DrawPoint(354, cy), Extent(30, 22), tc, 0);
        else
            group->AddColorDeepening(ID_btColor, DrawPoint(354, cy), Extent(30, 22), tc, 0);

        if(allowTeamChange)
            group->AddTextButton(ID_btTeam, DrawPoint(394, cy), Extent(50, 22), tc, _("-"), NormalFont);
        else
            group->AddTextDeepening(ID_btTeam, DrawPoint(394, cy), Extent(50, 22), tc, _("-"), NormalFont,
                                    COLOR_YELLOW);

        // Ready (not for AIs and Host)
        if(player.ps == PlayerState::Occupied && !player.isHost)
            group->AddCheckBox(ID_chkReady, DrawPoint(464, cy), Extent(22, 22), tc, "", nullptr,
                               (localPlayerId_ != row));

        ctrlVarDeepening* ping = group->AddVarDeepening(ID_txtPing, DrawPoint(505, cy), Extent(50, 22), tc, _("%d"),
                                                        NormalFont, COLOR_YELLOW, 1, &player.ping); //-V111

        // Move (not for Save games and Host)
        if(gameLobby_->isSavegame() && player.ps == PlayerState::Occupied)
        {
            ctrlComboBox* combo = group->AddComboBox(ID_cbMove, DrawPoint(560, cy), Extent(160, 22), tc, NormalFont,
                                                     150, !gameLobby_->isHost());

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

        // Fill fields
        ChangeNation(row, player.nation);
        ChangePortrait(row, player.portraitIndex);
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
        GetCtrl<ctrlEdit>(ID_edtChatMsg)->SetFocus();
}

void dskGameLobby::Msg_Group_ButtonClick(const unsigned group_id, const unsigned ctrl_id)
{
    unsigned playerId = group_id - ID_grpPlayerStart;

    switch(ctrl_id)
    {
        case ID_btPlayerState:
        {
            if(gameLobby_->isHost())
                lobbyController->TogglePlayerState(playerId);
        }
        break;

        case ID_btNation:
        {
            SetPlayerReady(playerId, false);

            if(playerId == localPlayerId_ || gameLobby_->isHost())
            {
                JoinPlayerInfo& player = gameLobby_->getPlayer(playerId);
                player.nation = nextEnumValue(player.nation);
                if(gameLobby_->isHost())
                    lobbyController->SetNation(playerId, player.nation);
                else
                    GAMECLIENT.Command_SetNation(player.nation);
                ChangeNation(playerId, player.nation);
            }
        }
        break;

        case ID_btPortrait:
        {
            SetPlayerReady(playerId, false);

            if(playerId == localPlayerId_ || gameLobby_->isHost())
            {
                JoinPlayerInfo& player = gameLobby_->getPlayer(playerId);
                player.portraitIndex = (player.portraitIndex + 1) % Portraits.size();
                if(gameLobby_->isHost())
                    lobbyController->SetPortrait(playerId, player.portraitIndex);
                else
                    GAMECLIENT.Command_SetPortrait(player.portraitIndex);
                ChangePortrait(playerId, player.portraitIndex);
            }
        }
        break;

        case ID_btColor:
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
                    lobbyController->SetColor(playerId, player.color);
                else
                    GAMECLIENT.Command_SetColor(player.color);
                ChangeColor(playerId, player.color);
            }

            // Start-Farbe der Minimap ändern
        }
        break;

        case ID_btTeam:
        {
            SetPlayerReady(playerId, false);

            if(playerId == localPlayerId_ || gameLobby_->isHost())
            {
                JoinPlayerInfo& player = gameLobby_->getPlayer(playerId);
                player.team = nextEnumValue(player.team);
                if(gameLobby_->isHost())
                    lobbyController->SetTeam(playerId, player.team);
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
    unsigned playerId = group_id - ID_grpPlayerStart;

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
    const unsigned playerId = group_id - ID_grpPlayerStart;

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
        lobbyController->SwapPlayers(playerId, static_cast<unsigned>(player2));
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
    if(ctrl_id >= ID_btSwap && ctrl_id < ID_btSwap + MAX_PLAYERS)
    {
        unsigned targetPlayer = ctrl_id - ID_btSwap;
        if(targetPlayer != localPlayerId_ && gameLobby_->isHost())
            lobbyController->SwapPlayers(localPlayerId_, targetPlayer);
        return;
    }
    switch(ctrl_id)
    {
        case ID_btReturn:
            GAMECLIENT.Stop();
            GoBack();
            break;

        case ID_btStartGame:
        {
            auto* ready = GetCtrl<ctrlTextButton>(ID_btStartGame);
            if(gameLobby_->isHost())
            {
                if(!checkOptions())
                    return;

                SetPlayerReady(localPlayerId_, true);
                if(lua)
                    lua->EventPlayerReady(localPlayerId_);
                if(ready->GetText() == _("Start game"))
                    lobbyController->StartCountdown(5);
                else
                    lobbyController->CancelCountdown();
            } else
            {
                if(ready->GetText() == _("Ready"))
                    SetPlayerReady(localPlayerId_, true);
                else
                    SetPlayerReady(localPlayerId_, false);
            }
        }
        break;
        case ID_btSettings: // Addons
        {
            if(auto* wnd = WINDOWMANAGER.FindNonModalWindow(CGI_ADDONS))
                wnd->Close();
            else
            {
                std::unique_ptr<iwAddons> w;
                if(!allowAddonChange)
                    w = std::make_unique<iwAddons>(gameLobby_->getSettings(), this, AddonChangeAllowed::None);
                else if(IsChangeAllowed("addonsAll"))
                    w = std::make_unique<iwAddons>(gameLobby_->getSettings(), this, AddonChangeAllowed::All);
                else
                {
                    RTTR_Assert(lua); // Otherwise all changes would be allowed
                    w = std::make_unique<iwAddons>(gameLobby_->getSettings(), this, AddonChangeAllowed::WhitelistOnly,
                                                   lua->GetAllowedAddons());
                }
                WINDOWMANAGER.Show(std::move(w));
            }
        }
        break;
    }
}

void dskGameLobby::Msg_EditEnter(const unsigned ctrl_id)
{
    if(ctrl_id != ID_edtChatMsg)
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
              this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed, ID_mbStartErrror));
        }

        ChangeReady(localPlayerId_, true);
    }
}

void dskGameLobby::FlashGameChat()
{
    if(!gameChat->IsVisible())
    {
        auto* tab = GetCtrl<Window>(ID_optChatTab);
        auto* bt = tab->GetCtrl<ctrlButton>(ID_btChatGame);
        if(!localChatTabAnimId)
            localChatTabAnimId = tab->GetAnimationManager().addAnimation(new BlinkButtonAnim(bt));
    }
}

void dskGameLobby::Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult mbr)
{
    switch(msgbox_id)
    {
        case ID_mbMapLoadError:
        case ID_mbError:
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
        case ID_mbQuestionEconomy: // Economy Mode - change Addon Setttings
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
                Msg_ButtonClick(ID_btStartGame);
            }
        }
        break;
        case ID_mbQuestionPeaceful: // Peaceful mode still active but we have an attack based victory condition
        {
            if(mbr == MsgboxResult::Yes)
            {
                gameLobby_->getSettings().setSelection(AddonId::PEACEFULMODE, false);
            } else if(mbr == MsgboxResult::No)
            {
                forceOptions = true;
                Msg_ButtonClick(ID_btStartGame);
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

        case ID_cbSpeed:
        case ID_cbGoals:
        case ID_cbGoods:
        case ID_cbExploration:
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
        case ID_chkSharedView:
        case ID_chkLockTeams:
        case ID_chkRandomSpawn:
        {
            // GameSettings wurden verändert, resetten
            UpdateGGS();
        }
        break;
    }
}

void dskGameLobby::Msg_OptionGroupChange(const unsigned ctrl_id, const unsigned selection)
{
    if(ctrl_id == ID_optChatTab)
    {
        gameChat->SetVisible(selection == ID_btChatGame);
        lobbyChat->SetVisible(selection == ID_btChatLobby);
        auto* tab = GetCtrl<Window>(ID_optChatTab);
        tab->GetCtrl<ctrlButton>(selection)->SetTexture(TextureColor::Green2);
        if(selection == ID_btChatGame)
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

    ggs.speed = static_cast<GameSpeed>(GetCtrl<ctrlComboBox>(ID_cbSpeed)->GetSelection().get());
    ggs.objective = static_cast<GameObjective>(GetCtrl<ctrlComboBox>(ID_cbGoals)->GetSelection().get());
    ggs.startWares = static_cast<StartWares>(GetCtrl<ctrlComboBox>(ID_cbGoods)->GetSelection().get());
    ggs.exploration = static_cast<Exploration>(GetCtrl<ctrlComboBox>(ID_cbExploration)->GetSelection().get());
    ggs.lockedTeams = GetCtrl<ctrlCheck>(ID_chkLockTeams)->isChecked();
    ggs.teamView = GetCtrl<ctrlCheck>(ID_chkSharedView)->isChecked();
    ggs.randomStartPosition = GetCtrl<ctrlCheck>(ID_chkRandomSpawn)->isChecked();

    // An Server übermitteln
    lobbyController->ChangeGlobalGameSettings(ggs);
}

void dskGameLobby::ChangeTeam(const unsigned player, const Team team)
{
    constexpr helpers::EnumArray<const char*, Team> teams = {"-", "?", "1", "2", "3", "4", "1-2", "1-3", "1-4"};

    GetCtrl<ctrlGroup>(ID_grpPlayerStart + player)->GetCtrl<ctrlBaseText>(ID_btTeam)->SetText(teams[team]);
}

void dskGameLobby::ChangeReady(const unsigned player, const bool ready)
{
    auto* check = GetCtrl<ctrlGroup>(ID_grpPlayerStart + player)->GetCtrl<ctrlCheck>(ID_chkReady);
    if(check)
        check->setChecked(ready);

    if(player == localPlayerId_)
    {
        auto* start = GetCtrl<ctrlTextButton>(ID_btStartGame);
        if(gameLobby_->isHost())
            start->SetText(hasCountdown_ ? _("Cancel start") : _("Start game"));
        else
            start->SetText(ready ? _("Not Ready") : _("Ready"));
    }
}

void dskGameLobby::ChangeNation(const unsigned player, const Nation nation)
{
    GetCtrl<ctrlGroup>(ID_grpPlayerStart + player)->GetCtrl<ctrlBaseText>(ID_btNation)->SetText(_(NationNames[nation]));
}

void dskGameLobby::ChangePortrait(const unsigned player, const unsigned portraitIndex)
{
    RTTR_Assert(portraitIndex < Portraits.size());
    const auto& portrait = Portraits[portraitIndex];
    auto* ctrl = GetCtrl<ctrlGroup>(ID_grpPlayerStart + player)->GetCtrl<ctrlImageButton>(ID_btPortrait);
    ctrl->SetImage(LOADER.GetImageN(portrait.resourceId, portrait.resourceIndex));
    ctrl->SetTooltip(_(portrait.name));
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
    GetCtrl<ctrlGroup>(ID_grpPlayerStart + playerId)->GetCtrl<ctrlVarDeepening>(ID_txtPing)->SetTextColor(color);
}

void dskGameLobby::ChangeColor(const unsigned player, const unsigned color)
{
    GetCtrl<ctrlGroup>(ID_grpPlayerStart + player)->GetCtrl<ctrlBaseColor>(ID_btColor)->SetColor(color);

    // Minimap-Startfarbe ändern
    if(GetCtrl<ctrlPreviewMinimap>(ID_miniMap))
        GetCtrl<ctrlPreviewMinimap>(ID_miniMap)->SetPlayerColor(player, color);
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

    GetCtrl<ctrlComboBox>(ID_cbSpeed)->SetSelection(static_cast<unsigned short>(ggs.speed));
    GetCtrl<ctrlComboBox>(ID_cbGoals)->SetSelection(static_cast<unsigned short>(ggs.objective));
    GetCtrl<ctrlComboBox>(ID_cbGoods)->SetSelection(static_cast<unsigned short>(ggs.startWares));
    GetCtrl<ctrlComboBox>(ID_cbExploration)->SetSelection(static_cast<unsigned short>(ggs.exploration));
    GetCtrl<ctrlCheck>(ID_chkLockTeams)->setChecked(ggs.lockedTeams);
    GetCtrl<ctrlCheck>(ID_chkSharedView)->setChecked(ggs.teamView);
    GetCtrl<ctrlCheck>(ID_chkRandomSpawn)->setChecked(ggs.randomStartPosition);

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
                                                  MsgboxIcon::ExclamationRed, ID_mbError));
}

/**
 *  (Lobby-)Status: Benutzerdefinierter Fehler (kann auch Conn-Loss o.ä sein)
 */
void dskGameLobby::LC_Status_Error(const std::string& error)
{
    WINDOWMANAGER.Show(
      std::make_unique<iwMsgbox>(_("Error"), error, this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed, ID_mbError));
}

void dskGameLobby::LC_Chat(const std::string& player, const std::string& text)
{
    if(!lobbyChat)
        return;
    lobbyChat->AddMessage("", player, ctrlChat::CalcUniqueColor(player), text, COLOR_YELLOW);
    if(!lobbyChat->IsVisible())
    {
        auto* tab = GetCtrl<Window>(ID_optChatTab);
        auto* bt = tab->GetCtrl<ctrlButton>(ID_btChatLobby);
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
          this, MsgboxButton::YesNoCancel, MsgboxIcon::QuestionGreen, ID_mbQuestionEconomy));
        return false;
    } else if(ggs.isEnabled(AddonId::PEACEFULMODE)
              && (ggs.objective == GameObjective::Conquer3_4 || ggs.objective == GameObjective::TotalDomination))
    {
        WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(
          _("Peaceful mode"),
          _("You chose a war based victory condition but peaceful mode is still active. Would you like to deactivate "
            "peaceful mode before you start? Choosing No will start the game, Yes will let you review the changes."),
          this, MsgboxButton::YesNoCancel, MsgboxIcon::QuestionRed, ID_mbQuestionPeaceful));
        return false;
    }
    return true;
}
