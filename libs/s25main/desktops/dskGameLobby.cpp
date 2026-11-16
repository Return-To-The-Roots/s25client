// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dskGameLobby.h"
#include "GameLobby.h"
#include "GameLobbyController.h"
#include "ILobbyClient.hpp"
#include "JoinPlayerInfo.h"
#include "Loader.h"
#include "RTTR_Assert.h"
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
#include <array>
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

std::array NATION_ORDER = {
  Nation::Romans, Nation::Vikings, Nation::Japanese, Nation::Africans, Nation::Babylonians,
};
static_assert(NATION_ORDER.size() == helpers::NumEnumValues_v<Nation>);

Nation nextNation(const Nation value)
{
    // NOLINTNEXTLINE(readability-qualified-auto)
    auto it = helpers::find(NATION_ORDER, value);
    RTTR_Assert(it != NATION_ORDER.end());
    if(++it == NATION_ORDER.end())
        it = NATION_ORDER.begin();
    return *it;
}
} // namespace

dskGameLobby::dskGameLobby(ServerType serverType, std::shared_ptr<GameLobby> gameLobby, unsigned playerId,
                           std::unique_ptr<ILobbyClient> lobbyClient)
    : Desktop(LOADER.getImageN("setup015", 0)), serverType(serverType), gameLobby_(std::move(gameLobby)),
      localPlayerId_(playerId), lobbyClient_(std::move(lobbyClient)), hasCountdown_(false), wasActivated(false),
      gameChat(nullptr), lobbyChat(nullptr), lobbyChatTabAnimId(0), localChatTabAnimId(0)
{
    // If no lobby don't do anything else
    if(!gameLobby_)
        return;

    const bool loadLua = !GAMECLIENT.getLuaFilePath().empty();

    // The lobby controller for clients is only used by lua
    if(gameLobby_->isHost() || loadLua)
        lobbyController = std::make_unique<GameLobbyController>(gameLobby_, GAMECLIENT.getMainPlayer());

    if(loadLua)
    {
        lua = std::make_unique<LuaInterfaceSettings>(*lobbyController, GAMECLIENT);
        if(!lua->loadScript(GAMECLIENT.getLuaFilePath()))
        {
            WINDOWMANAGER.showAfterSwitch(std::make_unique<iwMsgbox>(
              _("Error"), _("Lua script was found but failed to load. Map might not work as expected!"), this,
              MsgboxButton::Ok, MsgboxIcon::ExclamationRed, ID_mbLuaLoadError));
            lua.reset();
        } else if(!lua->checkScriptVersion())
        {
            WINDOWMANAGER.showAfterSwitch(std::make_unique<iwMsgbox>(
              _("Error"), _("Lua script uses a different version and cannot be used. Map might not work as expected!"),
              this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed, ID_mbLuaVersionError));
            lua.reset();
        } else if(!lua->eventSettingsInit(serverType == ServerType::Local, gameLobby_->isSavegame()))
        {
            // This should have been detected for the host so others won't even see the script
            RTTR_Assert(gameLobby_->isHost());
            LOG.write(_("Lua was disabled by the script itself\n"));
            lua.reset();
        }
        if(!lua && gameLobby_->isHost())
            lobbyController->removeLuaScript();
    }

    const bool readonlySettings = !gameLobby_->isHost() || gameLobby_->isSavegame() || !isChangeAllowed("general");
    allowAddonChange = gameLobby_->isHost() && !gameLobby_->isSavegame()
                       && (isChangeAllowed("addonsAll") || isChangeAllowed("addonsSome"));

    addText(ID_txtGameName, DrawPoint(400, 5), GAMECLIENT.getGameName(), COLOR_YELLOW, FontStyle::CENTER, LargeFont);

    addText(ID_txtColName, DrawPoint(125, 40), _("Player Name"), COLOR_YELLOW, FontStyle::CENTER, NormalFont);
    addText(ID_txtColRace, DrawPoint(262, 40), _("Race"), COLOR_YELLOW, FontStyle::CENTER, NormalFont);
    addText(ID_txtColColor, DrawPoint(369, 40), _("Color"), COLOR_YELLOW, FontStyle::CENTER, NormalFont);
    addText(ID_txtColTeam, DrawPoint(419, 40), _("Team"), COLOR_YELLOW, FontStyle::CENTER, NormalFont);

    if(!isSinglePlayer())
    {
        addText(ID_txtColReady, DrawPoint(479, 40), _("Ready?"), COLOR_YELLOW, FontStyle::CENTER, NormalFont);
        addText(ID_txtColPing, DrawPoint(530, 40), _("Ping"), COLOR_YELLOW, FontStyle::CENTER, NormalFont);
    }
    if(gameLobby_->isHost() && !gameLobby_->isSavegame())
        addText(ID_txtColSwap, DrawPoint(0, 40), _("Swap"), COLOR_YELLOW, FontStyle::LEFT, NormalFont);
    if(gameLobby_->isSavegame())
        addText(ID_txtColPastPlayer, DrawPoint(645, 40), _("Past player"), COLOR_YELLOW, FontStyle::CENTER, NormalFont);

    if(!isSinglePlayer())
    {
        // Enable lobby chat when we are logged in
        if(lobbyClient_ && lobbyClient_->IsLoggedIn())
        {
            ctrlOptionGroup* chatTab = addOptionGroup(ID_optChatTab, GroupSelectType::Check);
            chatTab->addTextButton(ID_btChatGame, DrawPoint(20, 320), Extent(178, 22), TextureColor::Green2,
                                   _("Game Chat"), NormalFont);
            chatTab->addTextButton(ID_btChatLobby, DrawPoint(202, 320), Extent(178, 22), TextureColor::Green2,
                                   _("Lobby Chat"), NormalFont);
            gameChat =
              addChatCtrl(ID_chatGame, DrawPoint(20, 345), Extent(360, 218 - 25), TextureColor::Grey, NormalFont);
            lobbyChat =
              addChatCtrl(ID_chatLobby, DrawPoint(20, 345), Extent(360, 218 - 25), TextureColor::Grey, NormalFont);
            chatTab->setSelection(ID_btChatGame, true);
        } else
        {
            gameChat = addChatCtrl(ID_chatGame, DrawPoint(20, 320), Extent(360, 218), TextureColor::Grey, NormalFont);
        }
        addEdit(ID_edtChatMsg, DrawPoint(20, 540), Extent(360, 22), TextureColor::Grey, NormalFont);
    }

    addTextButton(ID_btStartGame, DrawPoint(600, 560), Extent(180, 22), TextureColor::Green2,
                  (gameLobby_->isHost() ? _("Start game") : _("Ready")), NormalFont);

    addTextButton(ID_btReturn, DrawPoint(400, 560), Extent(180, 22), TextureColor::Red1, _("Return"), NormalFont);

    addCheckBox(ID_chkLockTeams, DrawPoint(400, 460), Extent(180, 26), TextureColor::Grey, _("Lock teams:"), NormalFont,
                readonlySettings);
    addCheckBox(ID_chkSharedView, DrawPoint(600, 460), Extent(180, 26), TextureColor::Grey, _("Shared team view"),
                NormalFont, readonlySettings);
    addCheckBox(ID_chkRandomSpawn, DrawPoint(600, 430), Extent(180, 26), TextureColor::Grey,
                _("Random start locations"), NormalFont, readonlySettings);

    addText(ID_txtAddons, DrawPoint(400, 499), _("Addons:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    addTextButton(ID_btSettings, DrawPoint(600, 495), Extent(180, 22), TextureColor::Green2,
                  allowAddonChange ? _("Change Settings...") : _("View Settings..."), NormalFont);

    ctrlComboBox* combo;

    // umgedrehte Reihenfolge, damit die Listen nicht dahinter sind

    addText(ID_txtExploration, DrawPoint(400, 405), _("Exploration:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo = addComboBox(ID_cbExploration, DrawPoint(600, 400), Extent(180, 20), TextureColor::Grey, NormalFont, 100,
                        readonlySettings);
    combo->addItem(_("Off (all visible)"));
    combo->addItem(_("Classic (Settlers 2)"));
    combo->addItem(_("Fog of War"));
    combo->addItem(_("FoW - all explored"));

    addText(ID_txtGoods, DrawPoint(400, 375), _("Goods at start:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo = addComboBox(ID_cbGoods, DrawPoint(600, 370), Extent(180, 20), TextureColor::Grey, NormalFont, 100,
                        readonlySettings);
    combo->addItem(_("Very Low"));
    combo->addItem(_("Low"));
    combo->addItem(_("Normal"));
    combo->addItem(_("A lot"));

    addText(ID_txtGoals, DrawPoint(400, 345), _("Goals:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo = addComboBox(ID_cbGoals, DrawPoint(600, 340), Extent(180, 20), TextureColor::Grey, NormalFont, 100,
                        readonlySettings);
    combo->addItem(_("None"));
    combo->addItem(_("Conquer 3/4 of map"));
    combo->addItem(_("Total domination"));
    combo->addItem(_("Economy mode"));

    // Lobby game?
    if(lobbyClient_ && lobbyClient_->IsLoggedIn())
    {
        // Then add tournament modes as possible "objectives"
        for(const auto duration : TOURNAMENT_MODES_DURATION)
            combo->addItem(helpers::format(_("Tournament: %u minutes"), duration / 1min));
    }

    addText(ID_txtSpeed, DrawPoint(400, 315), _("Speed:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo = addComboBox(ID_cbSpeed, DrawPoint(600, 310), Extent(180, 20), TextureColor::Grey, NormalFont, 100,
                        !gameLobby_->isHost());
    combo->addItem(_("Very slow"));
    combo->addItem(_("Slow"));
    combo->addItem(_("Normal"));
    combo->addItem(_("Fast"));
    combo->addItem(_("Very fast"));

    // Karte laden, um Kartenvorschau anzuzeigen
    if(!gameLobby_->isSavegame())
    {
        const bool isMapPreviewEnabled = !lua || lua->isMapPreviewEnabled();
        if(!isMapPreviewEnabled)
        {
            addTextDeepening(ID_txtNoPreview, DrawPoint(560, 40), Extent(220, 220), TextureColor::Grey, _("No preview"),
                             LargeFont, COLOR_YELLOW);
            addText(ID_txtMapName, DrawPoint(670, 40 + 220 + 10), _("Map: ") + GAMECLIENT.getMapTitle(), COLOR_YELLOW,
                    FontStyle::CENTER, NormalFont);
        } else
        {
            // Map laden
            libsiedler2::Archiv mapArchiv;
            // Karteninformationen laden
            if(int ec = libsiedler2::loader::LoadMAP(GAMECLIENT.getMapPath(), mapArchiv))
            {
                WINDOWMANAGER.showAfterSwitch(
                  std::make_unique<iwMsgbox>(_("Error"), _("Could not load map:\n") + libsiedler2::getErrorString(ec),
                                             this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed, ID_mbMapLoadError));
            } else
            {
                auto* map = static_cast<libsiedler2::ArchivItem_Map*>(mapArchiv.get(0));
                ctrlPreviewMinimap* preview = addPreviewMinimap(ID_miniMap, DrawPoint(560, 40), Extent(220, 220), map);

                // Titel der Karte, Y-Position relativ je nach Höhe der Minimap festlegen, daher nochmals danach
                // verschieben, da diese Position sonst skaliert wird!
                ctrlText* text = addText(ID_txtMapName, DrawPoint(670, 0), _("Map: ") + GAMECLIENT.getMapTitle(),
                                         COLOR_YELLOW, FontStyle::CENTER, NormalFont);
                text->setPos(DrawPoint(text->getPos().x, preview->getPos().y + preview->getMapArea().bottom + 10));
            }
        }
    }

    if(GAMECLIENT.isAIBattleModeOn())
    {
        const auto& aiBattlePlayers = GAMECLIENT.getAIBattlePlayers();

        // Initialize AI battle players
        for(unsigned i = 0; i < gameLobby_->getNumPlayers(); i++)
        {
            if(i < aiBattlePlayers.size())
                lobbyController->setPlayerState(i, PlayerState::AI, aiBattlePlayers[i]);
            else
                lobbyController->closeSlot(i); // Close remaining slots
        }

        // Set name of host to the corresponding AI for local player
        if(localPlayerId_ < aiBattlePlayers.size())
            lobbyController->setName(localPlayerId_,
                                     JoinPlayerInfo::makeAIName(aiBattlePlayers[localPlayerId_], localPlayerId_));
    } else if(isSinglePlayer() && !gameLobby_->isSavegame())
    {
        // Setze initial auf KI
        for(unsigned i = 0; i < gameLobby_->getNumPlayers(); i++)
        {
            if(!gameLobby_->getPlayer(i).isHost)
                lobbyController->setPlayerState(i, PlayerState::AI, AI::Info(AI::Type::Default, AI::Level::Easy));
        }
    }

    // Alle Spielercontrols erstellen
    for(unsigned i = 0; i < gameLobby_->getNumPlayers(); i++)
        updatePlayerRow(i);
    // swap buttons erstellen
    if(gameLobby_->isHost() && !gameLobby_->isSavegame() && isChangeAllowed("swapping"))
    {
        for(unsigned i = 0; i < gameLobby_->getNumPlayers(); i++)
        {
            int rowPos = getCtrl<Window>(ID_grpPlayerStart + i)->getCtrl<Window>(ID_btPlayerState)->getPos().y;
            ctrlButton* bt =
              addTextButton(ID_btSwap + i, DrawPoint(5, 0), Extent(22, 22), TextureColor::Red1, _("-"), NormalFont);
            bt->setPos(DrawPoint(bt->getPos().x, rowPos));
        }
    }
    CI_GGSChanged(gameLobby_->getSettings());

    if(serverType == ServerType::Lobby && lobbyClient_ && lobbyClient_->IsLoggedIn())
    {
        lobbyClient_->addListener(this);
        lobbyClient_->sendServerJoinRequest();
    }

    GAMECLIENT.setInterface(this);
}

dskGameLobby::~dskGameLobby()
{
    if(lobbyClient_)
        lobbyClient_->removeListener(this);
    GAMECLIENT.removeInterface(this);
}

/**
 *  Größe ändern-Reaktionen die nicht vom Skaling-Mechanismus erfasst werden.
 */
void dskGameLobby::resize(const Extent& newSize)
{
    Window::resize(newSize);

    // Text unter der PreviewMinimap verschieben, dessen Höhe von der Höhe der
    // PreviewMinimap abhängt, welche sich gerade geändert hat.
    auto* preview = getCtrl<ctrlPreviewMinimap>(ID_miniMap);
    auto* text = getCtrl<ctrlText>(ID_txtMapName);
    if(preview && text)
    {
        DrawPoint txtPos = text->getPos();
        txtPos.y = preview->getPos().y + preview->getMapArea().bottom + 10;
        text->setPos(txtPos);
    }
}

void dskGameLobby::setActive(bool activate /*= true*/)
{
    Desktop::setActive(activate);
    if(activate && !wasActivated && lua && gameLobby_->isHost())
    {
        wasActivated = true;
        try
        {
            lua->eventSettingsReady();
        } catch(const LuaExecutionError&)
        {
            WINDOWMANAGER.show(std::make_unique<iwMsgbox>(
              _("Error"), _("Lua script was found but failed to load. Map might not work as expected!"), this,
              MsgboxButton::Ok, MsgboxIcon::ExclamationRed, ID_mbLuaLoadError));
            lua.reset();
        }
    }
}

void dskGameLobby::updatePlayerRow(const unsigned row)
{
    const JoinPlayerInfo& player = gameLobby_->getPlayer(row);

    unsigned cy = 80 + row * 30;
    TextureColor tc = (row & 1 ? TextureColor::Grey : TextureColor::Green2);

    // Alle Controls erstmal zerstören (die ganze Gruppe)
    deleteCtrl(ID_grpPlayerStart + row);
    // und neu erzeugen
    ctrlGroup* group = addGroup(ID_grpPlayerStart + row);

    std::string name;
    switch(player.ps)
    {
        default: name.clear(); break;
        case PlayerState::Occupied:
        case PlayerState::AI: name = player.name; break;
        case PlayerState::Free: name = _("Open"); break;
        case PlayerState::Locked: name = _("Closed"); break;
    }

    if(getCtrl<ctrlPreviewMinimap>(ID_miniMap))
    {
        if(player.isUsed())
            // Nur KIs und richtige Spieler haben eine Farbe auf der Karte
            getCtrl<ctrlPreviewMinimap>(ID_miniMap)->setPlayerColor(row, player.color);
        else
            // Keine richtigen Spieler --> Startposition auf der Karte ausblenden
            getCtrl<ctrlPreviewMinimap>(ID_miniMap)->setPlayerColor(row, 0);
    }

    // Spielername, beim Hosts Spielerbuttons, aber nich beim ihm selber, er kann sich ja nich selber kicken!
    if(gameLobby_->isHost() && !player.isHost && isChangeAllowed("playerState"))
        group->addTextButton(ID_btPlayerState, DrawPoint(30, cy), Extent(180, 22), tc, name, NormalFont);
    else
        group->addTextDeepening(ID_btPlayerState, DrawPoint(30, cy), Extent(180, 22), tc, name, NormalFont,
                                COLOR_YELLOW);
    auto* text = group->getCtrl<ctrlBaseText>(ID_btPlayerState);

    // Is das der Host? Dann farblich markieren
    if(player.isHost)
        text->setTextColor(0xFF00FF00);

    // Bei geschlossenem nicht sichtbar
    if(player.isUsed())
    {
        // If not in savegame -> Player can change own row and host can change AIs
        const bool allowPlayerChange = ((gameLobby_->isHost() && player.ps == PlayerState::AI) || localPlayerId_ == row)
                                       && !gameLobby_->isSavegame();
        bool allowNationChange = allowPlayerChange;
        bool allowColorChange = allowPlayerChange;
        bool allowTeamChange = allowPlayerChange;
        bool allowPortraitChange = allowPlayerChange;
        if(lua)
        {
            if(localPlayerId_ == row)
            {
                allowNationChange &= lua->isChangeAllowed("ownNation", true);
                allowColorChange &= lua->isChangeAllowed("ownColor", true);
                allowTeamChange &= lua->isChangeAllowed("ownTeam", true);
                allowPortraitChange &= lua->isChangeAllowed("ownPortrait", true);
            } else
            {
                allowNationChange &= lua->isChangeAllowed("aiNation", true);
                allowColorChange &= lua->isChangeAllowed("aiColor", true);
                allowTeamChange &= lua->isChangeAllowed("aiTeam", true);
                allowPortraitChange &= lua->isChangeAllowed("aiPortrait", true);
            }
        }

        if(allowNationChange)
            group->addTextButton(ID_btNation, DrawPoint(215, cy), Extent(95, 22), tc, _(NationNames[NATION_ORDER[0]]),
                                 NormalFont);
        else
            group->addTextDeepening(ID_btNation, DrawPoint(215, cy), Extent(95, 22), tc,
                                    _(NationNames[NATION_ORDER[0]]), NormalFont, COLOR_YELLOW);

        const auto& portrait = Portraits[player.portraitIndex];
        if(allowPortraitChange)
            group->addImageButton(ID_btPortrait, DrawPoint(315, cy), Extent(34, 22), tc,
                                  LOADER.getImageN(portrait.resourceId, portrait.resourceIndex), _(portrait.name));
        else
            group->addImageDeepening(ID_btPortrait, DrawPoint(315, cy), Extent(34, 22), tc,
                                     LOADER.getImageN(portrait.resourceId, portrait.resourceIndex));

        if(allowColorChange)
            group->addColorButton(ID_btColor, DrawPoint(354, cy), Extent(30, 22), tc, 0);
        else
            group->addColorDeepening(ID_btColor, DrawPoint(354, cy), Extent(30, 22), tc, 0);

        if(allowTeamChange)
            group->addTextButton(ID_btTeam, DrawPoint(394, cy), Extent(50, 22), tc, _("-"), NormalFont);
        else
            group->addTextDeepening(ID_btTeam, DrawPoint(394, cy), Extent(50, 22), tc, _("-"), NormalFont,
                                    COLOR_YELLOW);

        // Ready (not for AIs and Host)
        if(player.ps == PlayerState::Occupied && !player.isHost)
            group->addCheckBox(ID_chkReady, DrawPoint(464, cy), Extent(22, 22), tc, "", nullptr,
                               (localPlayerId_ != row));

        ctrlVarDeepening* ping = group->addVarDeepening(ID_txtPing, DrawPoint(505, cy), Extent(50, 22), tc, _("%d"),
                                                        NormalFont, COLOR_YELLOW, 1, &player.ping); //-V111

        // Move (not for Save games and Host)
        if(gameLobby_->isSavegame() && player.ps == PlayerState::Occupied)
        {
            ctrlComboBox* combo = group->addComboBox(ID_cbMove, DrawPoint(560, cy), Extent(160, 22), tc, NormalFont,
                                                     150, !gameLobby_->isHost());

            // Mit den alten Namen füllen
            for(unsigned i = 0; i < gameLobby_->getNumPlayers(); ++i)
            {
                if(!gameLobby_->getPlayer(i).originName.empty())
                {
                    combo->addItem(gameLobby_->getPlayer(i).originName);
                    if(i == row)
                        combo->setSelection(combo->getNumItems() - 1u);
                }
            }
        }

        // Hide ping for AIs or on single player games
        if(player.ps == PlayerState::AI || isSinglePlayer())
            ping->setVisible(false);

        // Fill fields
        changeNation(row, player.nation);
        changePortrait(row, player.portraitIndex);
        changeTeam(row, player.team);
        changePing(row);
        changeReady(row, player.isReady);
        changeColor(row, player.color);
    }
    group->setActive(IsActive());
}

/**
 *  Methode vor dem Zeichnen
 */
void dskGameLobby::msgPaintBefore()
{
    Desktop::msgPaintBefore();
    // Chatfenster Fokus geben
    if(!isSinglePlayer())
        getCtrl<ctrlEdit>(ID_edtChatMsg)->setFocus();
}

void dskGameLobby::msgGroupButtonClick(const unsigned group_id, const unsigned ctrl_id)
{
    unsigned playerId = group_id - ID_grpPlayerStart;

    switch(ctrl_id)
    {
        case ID_btPlayerState:
        {
            if(gameLobby_->isHost())
                lobbyController->togglePlayerState(playerId);
        }
        break;

        case ID_btNation:
        {
            setPlayerReady(playerId, false);

            if(playerId == localPlayerId_ || gameLobby_->isHost())
            {
                JoinPlayerInfo& player = gameLobby_->getPlayer(playerId);
                player.nation = nextNation(player.nation);
                if(gameLobby_->isHost())
                    lobbyController->setNation(playerId, player.nation);
                else
                    GAMECLIENT.commandSetNation(player.nation);
                changeNation(playerId, player.nation);
            }
        }
        break;

        case ID_btPortrait:
        {
            setPlayerReady(playerId, false);

            if(playerId == localPlayerId_ || gameLobby_->isHost())
            {
                JoinPlayerInfo& player = gameLobby_->getPlayer(playerId);
                player.portraitIndex = (player.portraitIndex + 1) % Portraits.size();
                if(gameLobby_->isHost())
                    lobbyController->setPortrait(playerId, player.portraitIndex);
                else
                    GAMECLIENT.commandSetPortrait(player.portraitIndex);
                changePortrait(playerId, player.portraitIndex);
            }
        }
        break;

        case ID_btColor:
        {
            setPlayerReady(playerId, false);

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
                int newColorIdx = JoinPlayerInfo::getColorIdx(player.color);
                do
                {
                    player.color = PLAYER_COLORS[(++newColorIdx) % PLAYER_COLORS.size()];
                } while(helpers::contains(takenColors, player.color));

                if(gameLobby_->isHost())
                    lobbyController->setColor(playerId, player.color);
                else
                    GAMECLIENT.commandSetColor(player.color);
                changeColor(playerId, player.color);
            }

            // Start-Farbe der Minimap ändern
        }
        break;

        case ID_btTeam:
        {
            setPlayerReady(playerId, false);

            if(playerId == localPlayerId_ || gameLobby_->isHost())
            {
                JoinPlayerInfo& player = gameLobby_->getPlayer(playerId);
                player.team = nextEnumValue(player.team);
                if(gameLobby_->isHost())
                    lobbyController->setTeam(playerId, player.team);
                else
                    GAMECLIENT.commandSetTeam(player.team);
                changeTeam(playerId, player.team);
            }
        }
        break;
    }
}

void dskGameLobby::msgGroupCheckboxChange(const unsigned group_id, const unsigned /*ctrl_id*/, const bool checked)
{
    unsigned playerId = group_id - ID_grpPlayerStart;

    // Bereit
    if(playerId < MAX_PLAYERS)
        setPlayerReady(playerId, checked);
}

void dskGameLobby::msgGroupComboSelectItem(const unsigned group_id, const unsigned /*ctrl_id*/,
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
        lobbyController->swapPlayers(playerId, static_cast<unsigned>(player2));
}

void dskGameLobby::goBack()
{
    if(isSinglePlayer())
        WINDOWMANAGER.Switch(std::make_unique<dskSinglePlayer>());
    else if(serverType == ServerType::LAN)
        WINDOWMANAGER.Switch(std::make_unique<dskLAN>());
    else if(serverType == ServerType::Lobby && lobbyClient_ && lobbyClient_->IsLoggedIn())
        WINDOWMANAGER.Switch(std::make_unique<dskLobby>());
    else
        WINDOWMANAGER.Switch(std::make_unique<dskDirectIP>());
}

bool dskGameLobby::isChangeAllowed(const std::string& setting) const
{
    return !lua || lua->isChangeAllowed(setting);
}

void dskGameLobby::msgButtonClick(const unsigned ctrl_id)
{
    if(ctrl_id >= ID_btSwap && ctrl_id < ID_btSwap + MAX_PLAYERS)
    {
        unsigned targetPlayer = ctrl_id - ID_btSwap;
        if(targetPlayer != localPlayerId_ && gameLobby_->isHost())
            lobbyController->swapPlayers(localPlayerId_, targetPlayer);
        return;
    }
    switch(ctrl_id)
    {
        case ID_btReturn:
            GAMECLIENT.stop();
            goBack();
            break;

        case ID_btStartGame:
        {
            auto* ready = getCtrl<ctrlTextButton>(ID_btStartGame);
            if(gameLobby_->isHost())
            {
                if(!checkOptions())
                    return;

                setPlayerReady(localPlayerId_, true);
                if(lua)
                    lua->eventPlayerReady(localPlayerId_);
                if(ready->getText() == _("Start game"))
                    lobbyController->startCountdown(5);
                else
                    lobbyController->cancelCountdown();
            } else
            {
                if(ready->getText() == _("Ready"))
                    setPlayerReady(localPlayerId_, true);
                else
                    setPlayerReady(localPlayerId_, false);
            }
        }
        break;
        case ID_btSettings: // Addons
        {
            if(auto* wnd = WINDOWMANAGER.findNonModalWindow(CGI_ADDONS))
                wnd->close();
            else
            {
                std::unique_ptr<iwAddons> w;
                if(!allowAddonChange)
                    w = std::make_unique<iwAddons>(gameLobby_->getSettings(), this, AddonChangeAllowed::None);
                else if(isChangeAllowed("addonsAll"))
                    w = std::make_unique<iwAddons>(gameLobby_->getSettings(), this, AddonChangeAllowed::All);
                else
                {
                    RTTR_Assert(lua); // Otherwise all changes would be allowed
                    w = std::make_unique<iwAddons>(gameLobby_->getSettings(), this, AddonChangeAllowed::WhitelistOnly,
                                                   lua->getAllowedAddons());
                }
                WINDOWMANAGER.show(std::move(w));
            }
        }
        break;
    }
}

void dskGameLobby::msgEditEnter(const unsigned ctrl_id)
{
    if(ctrl_id != ID_edtChatMsg)
        return;
    auto* edit = getCtrl<ctrlEdit>(ctrl_id);
    const std::string msg = edit->getText();
    edit->setText("");
    if(gameChat->isVisible())
        GAMECLIENT.commandChat(msg, ChatDestination::All);
    else if(lobbyClient_ && lobbyClient_->IsLoggedIn() && lobbyChat->isVisible())
        lobbyClient_->sendChat(msg);
}

void dskGameLobby::CI_Countdown(unsigned remainingTimeInSec)
{
    if(isSinglePlayer())
        return;

    if(!hasCountdown_)
    {
        const std::string startMsg = helpers::format(_("You have %u seconds until game starts"), remainingTimeInSec);
        gameChat->addMessage("", "", 0, startMsg, COLOR_RED);
        gameChat->addMessage("", "", 0, _("Don't forget to check the addon configuration!"), 0xFFFFDD00);
        gameChat->addMessage("", "", 0, "", 0xFFFFCC00);
        hasCountdown_ = true;
    }

    const std::string message =
      (remainingTimeInSec > 0) ? " " + std::to_string(remainingTimeInSec) : _("Starting game, please wait");

    gameChat->addMessage("", "", 0, message, 0xFFFFBB00);
}

void dskGameLobby::CI_CancelCountdown(bool error)
{
    if(hasCountdown_)
    {
        hasCountdown_ = false;
        gameChat->addMessage("", "", 0xFFCC2222, _("Start aborted"), 0xFFFFCC00);
        flashGameChat();
    }

    if(gameLobby_->isHost())
    {
        if(error)
        {
            WINDOWMANAGER.show(std::make_unique<iwMsgbox>(
              _("Error"),
              _("Game can only be started as soon as everybody has a unique color,everyone is "
                "ready and all free slots are closed."),
              this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed, ID_mbStartErrror));
        }

        changeReady(localPlayerId_, true);
    }
}

void dskGameLobby::flashGameChat()
{
    if(!gameChat->isVisible())
    {
        auto* tab = getCtrl<Window>(ID_optChatTab);
        auto* bt = tab->getCtrl<ctrlButton>(ID_btChatGame);
        if(!localChatTabAnimId)
            localChatTabAnimId = tab->getAnimationManager().addAnimation(new BlinkButtonAnim(bt));
    }
}

void dskGameLobby::msgMsgBoxResult(const unsigned msgbox_id, const MsgboxResult mbr)
{
    switch(msgbox_id)
    {
        case ID_mbMapLoadError:
        case ID_mbError:
        {
            GAMECLIENT.stop();

            goBack();
        }
        break;
        case CGI_ADDONS: // addon-window applied settings?
        {
            if(mbr == MsgboxResult::Yes)
                updateGGS();
        }
        break;
        case ID_mbQuestionEconomy: // Economy Mode - change Addon Setttings
        {
            if(mbr == MsgboxResult::Yes)
            {
                gameLobby_->getSettings().setSelection(AddonId::PEACEFULMODE, true);
                gameLobby_->getSettings().setSelection(AddonId::NO_COINS_DEFAULT, true);
                gameLobby_->getSettings().setSelection(AddonId::NO_ARMOR_DEFAULT, true);
                gameLobby_->getSettings().setSelection(AddonId::LIMIT_CATAPULTS, 2);
                updateGGS();
            } else if(mbr == MsgboxResult::No)
            {
                forceOptions = true;
                msgButtonClick(ID_btStartGame);
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
                msgButtonClick(ID_btStartGame);
            }
        }
        break;
    }
}

void dskGameLobby::msgComboSelectItem(const unsigned ctrl_id, const unsigned /*selection*/)
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
            updateGGS();
        }
        break;
    }
}

void dskGameLobby::msgCheckboxChange(const unsigned ctrl_id, const bool /*checked*/)
{
    switch(ctrl_id)
    {
        default: break;
        case ID_chkSharedView:
        case ID_chkLockTeams:
        case ID_chkRandomSpawn:
        {
            // GameSettings wurden verändert, resetten
            updateGGS();
        }
        break;
    }
}

void dskGameLobby::msgOptionGroupChange(const unsigned ctrl_id, const unsigned selection)
{
    if(ctrl_id == ID_optChatTab)
    {
        gameChat->setVisible(selection == ID_btChatGame);
        lobbyChat->setVisible(selection == ID_btChatLobby);
        auto* tab = getCtrl<Window>(ID_optChatTab);
        tab->getCtrl<ctrlButton>(selection)->setTexture(TextureColor::Green2);
        if(selection == ID_btChatGame)
        {
            tab->getAnimationManager().finishAnimation(localChatTabAnimId, false);
            localChatTabAnimId = 0;
        } else
        {
            tab->getAnimationManager().finishAnimation(lobbyChatTabAnimId, false);
            lobbyChatTabAnimId = 0;
        }
    }
}

void dskGameLobby::updateGGS()
{
    RTTR_Assert(gameLobby_->isHost());

    GlobalGameSettings& ggs = gameLobby_->getSettings();

    ggs.speed = static_cast<GameSpeed>(getCtrl<ctrlComboBox>(ID_cbSpeed)->getSelection().value());
    ggs.objective = static_cast<GameObjective>(getCtrl<ctrlComboBox>(ID_cbGoals)->getSelection().value());
    ggs.startWares = static_cast<StartWares>(getCtrl<ctrlComboBox>(ID_cbGoods)->getSelection().value());
    ggs.exploration = static_cast<Exploration>(getCtrl<ctrlComboBox>(ID_cbExploration)->getSelection().value());
    ggs.lockedTeams = getCtrl<ctrlCheck>(ID_chkLockTeams)->isChecked();
    ggs.teamView = getCtrl<ctrlCheck>(ID_chkSharedView)->isChecked();
    ggs.randomStartPosition = getCtrl<ctrlCheck>(ID_chkRandomSpawn)->isChecked();

    // An Server übermitteln
    lobbyController->changeGlobalGameSettings(ggs);
}

void dskGameLobby::changeTeam(const unsigned player, const Team team)
{
    constexpr helpers::EnumArray<const char*, Team> teams = {"-", "?", "1", "2", "3", "4", "1-2", "1-3", "1-4"};

    getCtrl<ctrlGroup>(ID_grpPlayerStart + player)->getCtrl<ctrlBaseText>(ID_btTeam)->setText(teams[team]);
}

void dskGameLobby::changeReady(const unsigned player, const bool ready)
{
    auto* check = getCtrl<ctrlGroup>(ID_grpPlayerStart + player)->getCtrl<ctrlCheck>(ID_chkReady);
    if(check)
        check->setChecked(ready);

    if(player == localPlayerId_)
    {
        auto* start = getCtrl<ctrlTextButton>(ID_btStartGame);
        if(gameLobby_->isHost())
            start->setText(hasCountdown_ ? _("Cancel start") : _("Start game"));
        else
            start->setText(ready ? _("Not Ready") : _("Ready"));
    }
}

void dskGameLobby::changeNation(const unsigned player, const Nation nation)
{
    getCtrl<ctrlGroup>(ID_grpPlayerStart + player)->getCtrl<ctrlBaseText>(ID_btNation)->setText(_(NationNames[nation]));
}

void dskGameLobby::changePortrait(const unsigned player, const unsigned portraitIndex)
{
    RTTR_Assert(portraitIndex < Portraits.size());
    const auto& portrait = Portraits[portraitIndex];
    auto* ctrl = getCtrl<ctrlGroup>(ID_grpPlayerStart + player)->getCtrl<ctrlBaseImage>(ID_btPortrait);
    ctrl->setImage(LOADER.getImageN(portrait.resourceId, portrait.resourceIndex));
    auto* ctrlButton = getCtrl<ctrlGroup>(ID_grpPlayerStart + player)->getCtrl<ctrlImageButton>(ID_btPortrait);
    if(ctrlButton)
        ctrlButton->setTooltip(_(portrait.name));
}

void dskGameLobby::changePing(unsigned playerId)
{
    unsigned color = COLOR_RED;

    // Farbe bestimmen
    if(gameLobby_->getPlayer(playerId).ping < 300)
        color = COLOR_GREEN;
    else if(gameLobby_->getPlayer(playerId).ping < 800)
        color = COLOR_YELLOW;

    // und setzen
    getCtrl<ctrlGroup>(ID_grpPlayerStart + playerId)->getCtrl<ctrlVarDeepening>(ID_txtPing)->setTextColor(color);
}

void dskGameLobby::changeColor(const unsigned player, const unsigned color)
{
    getCtrl<ctrlGroup>(ID_grpPlayerStart + player)->getCtrl<ctrlBaseColor>(ID_btColor)->setColor(color);

    // Minimap-Startfarbe ändern
    if(getCtrl<ctrlPreviewMinimap>(ID_miniMap))
        getCtrl<ctrlPreviewMinimap>(ID_miniMap)->setPlayerColor(player, color);
}

void dskGameLobby::setPlayerReady(unsigned char player, bool ready)
{
    if(player != localPlayerId_)
        return;
    if(gameLobby_->isHost())
        ready = true;
    if(gameLobby_->getPlayer(player).isReady != ready)
    {
        gameLobby_->getPlayer(player).isReady = ready;
        GAMECLIENT.commandSetReady(ready);
    }
    changeReady(player, ready);
}

void dskGameLobby::CI_NewPlayer(const unsigned playerId)
{
    updatePlayerRow(playerId);

    if(lua && gameLobby_->isHost())
        lua->eventPlayerJoined(playerId);
}

void dskGameLobby::CI_PlayerLeft(const unsigned playerId)
{
    updatePlayerRow(playerId);
    if(lua && gameLobby_->isHost())
        lua->eventPlayerLeft(playerId);
}

void dskGameLobby::CI_GameLoading(std::shared_ptr<Game> game)
{
    WINDOWMANAGER.Switch(std::make_unique<dskGameLoader>(std::move(game)));
}

void dskGameLobby::CI_PlayerDataChanged(unsigned playerId)
{
    updatePlayerRow(playerId);
}

void dskGameLobby::CI_PingChanged(const unsigned playerId, const unsigned short /*ping*/)
{
    changePing(playerId);
}

void dskGameLobby::CI_ReadyChanged(const unsigned playerId, const bool ready)
{
    changeReady(playerId, ready);
    // Event only called for other players (host ready is done in start game)
    // Also only for host and non-savegames
    if(ready && lua && gameLobby_->isHost() && playerId != localPlayerId_)
        lua->eventPlayerReady(playerId);
}

void dskGameLobby::CI_PlayersSwapped(const unsigned player1, const unsigned player2)
{
    if(player1 == localPlayerId_)
        localPlayerId_ = player2;
    else if(localPlayerId_ == player2)
        localPlayerId_ = player1;
    // Spieler wurden vertauscht, beide Reihen updaten
    updatePlayerRow(player1);
    updatePlayerRow(player2);
}

void dskGameLobby::CI_GGSChanged(const GlobalGameSettings& /*ggs*/)
{
    const GlobalGameSettings& ggs = gameLobby_->getSettings();

    getCtrl<ctrlComboBox>(ID_cbSpeed)->setSelection(static_cast<unsigned short>(ggs.speed));
    getCtrl<ctrlComboBox>(ID_cbGoals)->setSelection(static_cast<unsigned short>(ggs.objective));
    getCtrl<ctrlComboBox>(ID_cbGoods)->setSelection(static_cast<unsigned short>(ggs.startWares));
    getCtrl<ctrlComboBox>(ID_cbExploration)->setSelection(static_cast<unsigned short>(ggs.exploration));
    getCtrl<ctrlCheck>(ID_chkLockTeams)->setChecked(ggs.lockedTeams);
    getCtrl<ctrlCheck>(ID_chkSharedView)->setChecked(ggs.teamView);
    getCtrl<ctrlCheck>(ID_chkRandomSpawn)->setChecked(ggs.randomStartPosition);

    setPlayerReady(localPlayerId_, false);
}

void dskGameLobby::CI_Chat(const unsigned playerId, const ChatDestination /*cd*/, const std::string& msg)
{
    if((playerId != 0xFFFFFFFF) && !isSinglePlayer())
    {
        std::string time = s25util::Time::FormatTime("(%H:%i:%s)");

        gameChat->addMessage(time, gameLobby_->getPlayer(playerId).name, gameLobby_->getPlayer(playerId).color, msg,
                             0xFFFFFF00); //-V810
        flashGameChat();
    }
}

void dskGameLobby::CI_Error(const ClientError ce)
{
    WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Error"), clientErrorToStr(ce), this, MsgboxButton::Ok,
                                                  MsgboxIcon::ExclamationRed, ID_mbError));
}

/**
 *  (Lobby-)Status: Benutzerdefinierter Fehler (kann auch Conn-Loss o.ä sein)
 */
void dskGameLobby::LC_Status_Error(const std::string& error)
{
    WINDOWMANAGER.show(
      std::make_unique<iwMsgbox>(_("Error"), error, this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed, ID_mbError));
}

void dskGameLobby::LC_Chat(const std::string& player, const std::string& text)
{
    if(!lobbyChat)
        return;
    lobbyChat->addMessage("", player, ctrlChat::calcUniqueColor(player), text, COLOR_YELLOW);
    if(!lobbyChat->isVisible())
    {
        auto* tab = getCtrl<Window>(ID_optChatTab);
        auto* bt = tab->getCtrl<ctrlButton>(ID_btChatLobby);
        if(!lobbyChatTabAnimId)
            lobbyChatTabAnimId = tab->getAnimationManager().addAnimation(new BlinkButtonAnim(bt));
    }
}

bool dskGameLobby::checkOptions()
{
    if(forceOptions)
        return true;
    const GlobalGameSettings& ggs = gameLobby_->getSettings();
    if(ggs.objective == GameObjective::EconomyMode && !ggs.isEnabled(AddonId::PEACEFULMODE))
    {
        WINDOWMANAGER.show(std::make_unique<iwMsgbox>(
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
        WINDOWMANAGER.show(std::make_unique<iwMsgbox>(
          _("Peaceful mode"),
          _("You chose a war based victory condition but peaceful mode is still active. Would you like to deactivate "
            "peaceful mode before you start? Choosing No will start the game, Yes will let you review the changes."),
          this, MsgboxButton::YesNoCancel, MsgboxIcon::QuestionRed, ID_mbQuestionPeaceful));
        return false;
    }
    return true;
}
