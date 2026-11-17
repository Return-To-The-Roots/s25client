// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dskGameInterface.h"
#include "CollisionDetection.h"
#include "EventManager.h"
#include "Game.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "NWFInfo.h"
#include "Settings.h"
#include "SoundManager.h"
#include "WindowManager.h"
#include "addons/AddonMaxWaterwayLength.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobHQ.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobStorehouse.h"
#include "buildings/nobTemple.h"
#include "buildings/nobUsual.h"
#include "controls/ctrlImageButton.h"
#include "controls/ctrlText.h"
#include "driver/MouseCoords.h"
#include "drivers/VideoDriverWrapper.h"
#include "helpers/format.hpp"
#include "helpers/strUtils.h"
#include "helpers/toString.h"
#include "ingameWindows/iwAIDebug.h"
#include "ingameWindows/iwAction.h"
#include "ingameWindows/iwBaseWarehouse.h"
#include "ingameWindows/iwBuildOrder.h"
#include "ingameWindows/iwBuilding.h"
#include "ingameWindows/iwBuildingProductivities.h"
#include "ingameWindows/iwBuildingSite.h"
#include "ingameWindows/iwBuildings.h"
#include "ingameWindows/iwDiplomacy.h"
#include "ingameWindows/iwDistribution.h"
#include "ingameWindows/iwEconomicProgress.h"
#include "ingameWindows/iwEndgame.h"
#include "ingameWindows/iwHQ.h"
#include "ingameWindows/iwHarborBuilding.h"
#include "ingameWindows/iwInventory.h"
#include "ingameWindows/iwMainMenu.h"
#include "ingameWindows/iwMapDebug.h"
#include "ingameWindows/iwMerchandiseStatistics.h"
#include "ingameWindows/iwMilitary.h"
#include "ingameWindows/iwMilitaryBuilding.h"
#include "ingameWindows/iwMinimap.h"
#include "ingameWindows/iwMusicPlayer.h"
#include "ingameWindows/iwOptionsWindow.h"
#include "ingameWindows/iwPostWindow.h"
#include "ingameWindows/iwRoadWindow.h"
#include "ingameWindows/iwSave.h"
#include "ingameWindows/iwSettings.h"
#include "ingameWindows/iwShip.h"
#include "ingameWindows/iwSkipGFs.h"
#include "ingameWindows/iwStatistics.h"
#include "ingameWindows/iwTempleBuilding.h"
#include "ingameWindows/iwTextfile.h"
#include "ingameWindows/iwTools.h"
#include "ingameWindows/iwTrade.h"
#include "ingameWindows/iwTransport.h"
#include "ingameWindows/iwVictory.h"
#include "lua/GameDataLoader.h"
#include "network/GameClient.h"
#include "notifications/BuildingNote.h"
#include "notifications/NotificationManager.h"
#include "ogl/FontStyle.h"
#include "ogl/SoundEffectItem.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "ogl/glFont.h"
#include "pathfinding/FindPathForRoad.h"
#include "postSystem/PostBox.h"
#include "postSystem/PostMsg.h"
#include "random/Random.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldViewer.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noTree.h"
#include "gameData/BuildingProperties.h"
#include "gameData/GameConsts.h"
#include "gameData/GuiConsts.h"
#include "gameData/TerrainDesc.h"
#include "gameData/const_gui_ids.h"
#include "liblobby/LobbyClient.h"
#include <algorithm>
#include <cstdio>
#include <utility>

namespace {
enum
{
    ID_btMap,
    ID_btOptions,
    ID_btConstructionAid,
    ID_btPost,
    ID_txtNumMsg
};

/// Size of buttons on lower bar
constexpr Extent btSize = Extent(37, 32);
/// Offsets of the buttons relative to the "border" graphics on the lower bar
constexpr DrawPoint btOffset(44, 4);

float getNextZoomLevel(const float currentZoom)
{
    // Get first level bigger than current zoom
    // NOLINTNEXTLINE(readability-qualified-auto)
    auto it = std::upper_bound(ZOOM_FACTORS.begin(), ZOOM_FACTORS.end(), currentZoom);
    return (it == ZOOM_FACTORS.end()) ? ZOOM_FACTORS.front() : *it;
}

float getPreviousZoomLevel(const float currentZoom)
{
    // Get last level bigger or equal than current zoom
    // NOLINTNEXTLINE(readability-qualified-auto)
    auto it = std::lower_bound(ZOOM_FACTORS.begin(), ZOOM_FACTORS.end(), currentZoom);
    return (it == ZOOM_FACTORS.begin()) ? ZOOM_FACTORS.back() : *(--it);
}
} // namespace

dskGameInterface::dskGameInterface(std::shared_ptr<Game> game, std::shared_ptr<const NWFInfo> nwfInfo,
                                   unsigned playerIdx, bool initOGL)
    : Desktop(nullptr), game_(std::move(game)), nwfInfo_(std::move(nwfInfo)),
      worldViewer(playerIdx, const_cast<Game&>(*game_).world_),
      gwv(worldViewer, Position(0, 0), VIDEODRIVER.getRenderSize()), cbb(*LOADER.getPaletteN("pal5")),
      actionwindow(nullptr), roadwindow(nullptr), minimap(worldViewer), isScrolling(false),
      cheats_(const_cast<Game&>(*game_).world_, GAMECLIENT), cheatCommandTracker_(cheats_)
{
    road.mode = RoadBuildMode::Disabled;
    road.point = MapPoint(0, 0);
    road.start = MapPoint(0, 0);

    setScale(false);

    const glArchivItem_Bitmap& imgButtonBar = *LOADER.getImageN("resource", 29);

    auto barPos =
      DrawPoint((getSize().x - imgButtonBar.getWidth()) / 2, getSize().y - imgButtonBar.getHeight()) + btOffset;

    addImageButton(ID_btMap, barPos, btSize, TextureColor::Green1, LOADER.getImageN("io", 50), _("Map"))
      ->setBorder(false);
    barPos.x += btSize.x;
    addImageButton(ID_btOptions, barPos, btSize, TextureColor::Green1, LOADER.getImageN("io", 192), _("Main selection"))
      ->setBorder(false);
    barPos.x += btSize.x;
    addImageButton(ID_btConstructionAid, barPos, btSize, TextureColor::Green1, LOADER.getImageN("io", 83),
                   _("Construction aid mode"))
      ->setBorder(false);
    barPos.x += btSize.x;
    addImageButton(ID_btPost, barPos, btSize, TextureColor::Green1, LOADER.getImageN("io", 62), _("Post office"))
      ->setBorder(false);
    barPos += DrawPoint(18, 24);

    addText(ID_txtNumMsg, barPos, "", COLOR_YELLOW, FontStyle::CENTER | FontStyle::VCENTER, SmallFont);

    const_cast<Game&>(*game_).world_.setGameInterface(this);

    std::fill(borders.begin(), borders.end(), (glArchivItem_Bitmap*)(nullptr));
    cbb.loadEdges(LOADER.getArchive("resource"));
    cbb.buildBorder(VIDEODRIVER.getRenderSize(), borders);

    initPlayer();
    if(initOGL)
        worldViewer.initTerrainRenderer();

    VIDEODRIVER.setTargetFramerate(SETTINGS.video.framerate); // Use requested setting for ingame
}

void dskGameInterface::initPlayer()
{
    // Jump to players HQ if it exists
    if(worldViewer.getPlayer().getHQPos().isValid())
        gwv.moveToMapPt(worldViewer.getPlayer().getHQPos());

    evBld = worldViewer.getWorld().getNotifications().subscribe<BuildingNote>([this](const auto& note) {
        if(note.player == worldViewer.getPlayerId())
            this->onBuildingNote(note);
    });
    PostBox& postBox = getPostBox();
    postBox.observeNewMsg([this](const auto& msg, auto msgCt) { this->newPostMessage(msg, msgCt); });
    postBox.observeDeletedMsg([this](auto msgCt) { this->postMessageDeleted(msgCt); });
    updatePostIcon(postBox.getNumMsgs(), true);
}

PostBox& dskGameInterface::getPostBox()
{
    PostBox* postBox = worldViewer.getWorld().getPostMgr().getPostBox(worldViewer.getPlayerId());
    if(!postBox)
        postBox = &worldViewer.getWorldNonConst().getPostMgr().addPostBox(worldViewer.getPlayerId());
    RTTR_Assert(postBox != nullptr);
    return *postBox;
}

dskGameInterface::~dskGameInterface()
{
    for(auto& border : borders)
        deletePtr(border);
    GAMECLIENT.removeInterface(this);
    LOBBYCLIENT.RemoveListener(this);
}

void dskGameInterface::setActive(bool activate)
{
    if(activate == isActive())
        return;
    if(!activate && isScrolling)
    {
        // Stay active if scrolling and no modal window is open
        const IngameWindow* wnd = WINDOWMANAGER.getTopMostWindow();
        if(wnd && wnd->isModal())
            stopScrolling();
        else
            return;
    }
    Desktop::setActive(activate);
    // Do this here to allow previous screen to keep control
    if(activate)
    {
        GAMECLIENT.setInterface(this);
        LOBBYCLIENT.AddListener(this);
        if(!game_->isStarted())
        {
            GAMECLIENT.onGameStart();

            showPersistentWindowsAfterSwitch();
        }
    }
}

void dskGameInterface::stopScrolling()
{
    isScrolling = false;
    WINDOWMANAGER.setCursor(road.mode == RoadBuildMode::Disabled ? Cursor::Hand : Cursor::remove);
}

void dskGameInterface::startScrolling(const Position& mousePos)
{
    startScrollPt = mousePos;
    isScrolling = true;
    WINDOWMANAGER.setCursor(Cursor::Scroll);
}

void dskGameInterface::toggleFoW()
{
    disableFoW(!GAMECLIENT.isReplayFOWDisabled());
}

void dskGameInterface::disableFoW(const bool hideFOW)
{
    GAMECLIENT.setReplayFOW(hideFOW);
    // Notify viewer and minimap to recalculate the visibility
    worldViewer.recalcAllColors();
    minimap.updateAll();
}

void dskGameInterface::showPersistentWindowsAfterSwitch()
{
    auto& windows = SETTINGS.windows.persistentSettings;

    if(windows[CGI_CHAT].isOpen)
        WINDOWMANAGER.showAfterSwitch(std::make_unique<iwChat>(this));
    if(windows[CGI_POSTOFFICE].isOpen)
        WINDOWMANAGER.showAfterSwitch(std::make_unique<iwPostWindow>(gwv, getPostBox()));
    if(windows[CGI_DISTRIBUTION].isOpen)
        WINDOWMANAGER.showAfterSwitch(std::make_unique<iwDistribution>(gwv.getViewer(), GAMECLIENT));
    if(windows[CGI_BUILDORDER].isOpen && gwv.getWorld().getGGS().isEnabled(AddonId::CUSTOM_BUILD_SEQUENCE))
        WINDOWMANAGER.showAfterSwitch(std::make_unique<iwBuildOrder>(gwv.getViewer()));
    if(windows[CGI_TRANSPORT].isOpen)
        WINDOWMANAGER.showAfterSwitch(std::make_unique<iwTransport>(gwv.getViewer(), GAMECLIENT));
    if(windows[CGI_MILITARY].isOpen)
        WINDOWMANAGER.showAfterSwitch(std::make_unique<iwMilitary>(gwv.getViewer(), GAMECLIENT));
    if(windows[CGI_TOOLS].isOpen)
        WINDOWMANAGER.showAfterSwitch(std::make_unique<iwTools>(gwv.getViewer(), GAMECLIENT));
    if(windows[CGI_INVENTORY].isOpen)
        WINDOWMANAGER.showAfterSwitch(std::make_unique<iwInventory>(gwv.getViewer().getPlayer()));
    if(windows[CGI_MINIMAP].isOpen)
        WINDOWMANAGER.showAfterSwitch(std::make_unique<iwMinimap>(minimap, gwv));
    if(windows[CGI_BUILDINGS].isOpen)
        WINDOWMANAGER.showAfterSwitch(std::make_unique<iwBuildings>(gwv, GAMECLIENT));
    if(windows[CGI_BUILDINGSPRODUCTIVITY].isOpen)
        WINDOWMANAGER.showAfterSwitch(std::make_unique<iwBuildingProductivities>(gwv.getViewer().getPlayer()));
    if(windows[CGI_MUSICPLAYER].isOpen)
        WINDOWMANAGER.showAfterSwitch(std::make_unique<iwMusicPlayer>());
    if(windows[CGI_STATISTICS].isOpen)
        WINDOWMANAGER.showAfterSwitch(std::make_unique<iwStatistics>(gwv.getViewer()));
    if(windows[CGI_ECONOMICPROGRESS].isOpen && gwv.getWorld().getEconHandler())
        WINDOWMANAGER.showAfterSwitch(std::make_unique<iwEconomicProgress>(gwv.getViewer()));
    if(windows[CGI_DIPLOMACY].isOpen)
        WINDOWMANAGER.showAfterSwitch(std::make_unique<iwDiplomacy>(gwv.getViewer(), GAMECLIENT));
    if(windows[CGI_SHIP].isOpen)
        WINDOWMANAGER.showAfterSwitch(
          std::make_unique<iwShip>(gwv, GAMECLIENT, gwv.getViewer().getPlayer().getShipByID(0)));
    if(windows[CGI_MERCHANDISE_STATISTICS].isOpen)
        WINDOWMANAGER.showAfterSwitch(std::make_unique<iwMerchandiseStatistics>(gwv.getViewer().getPlayer()));
}

void dskGameInterface::resize(const Extent& newSize)
{
    Window::resize(newSize);

    // recreate borders
    for(auto& border : borders)
        deletePtr(border);
    cbb.buildBorder(newSize, borders);

    // move buttons
    // Get real renderer size as newSize may get capped but we want to keep the manually drawn borders intact
    const Extent realNewSize = VIDEODRIVER.getRenderSize();
    const glArchivItem_Bitmap& imgButtonBar = *LOADER.getImageN("resource", 29);
    DrawPoint barPos =
      DrawPoint((realNewSize.x - imgButtonBar.getWidth()) / 2, realNewSize.y - imgButtonBar.getHeight()) + btOffset;

    auto* button = getCtrl<ctrlButton>(ID_btMap);
    button->setPos(barPos);

    barPos.x += button->getSize().x;
    button = getCtrl<ctrlButton>(ID_btOptions);
    button->setPos(barPos);

    barPos.x += button->getSize().x;
    button = getCtrl<ctrlButton>(ID_btConstructionAid);
    button->setPos(barPos);

    barPos.x += button->getSize().x;
    button = getCtrl<ctrlButton>(ID_btPost);
    button->setPos(barPos);

    barPos += DrawPoint(18, 24);
    auto* text = getCtrl<ctrlText>(ID_txtNumMsg);
    text->setPos(barPos);

    gwv.resize(newSize);
}

void dskGameInterface::msgButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_btMap: WINDOWMANAGER.toggleWindow(std::make_unique<iwMinimap>(minimap, gwv)); break;
        case ID_btOptions: WINDOWMANAGER.toggleWindow(std::make_unique<iwMainMenu>(gwv, GAMECLIENT)); break;
        case ID_btConstructionAid:
            if(WINDOWMANAGER.isDesktopActive())
                gwv.toggleShowBQ();
            break;
        case ID_btPost:
            WINDOWMANAGER.toggleWindow(std::make_unique<iwPostWindow>(gwv, getPostBox()));
            updatePostIcon(getPostBox().getNumMsgs(), false);
            break;
    }
}

void dskGameInterface::msgPaintBefore()
{
    Desktop::msgPaintBefore();

    // Spiel ausführen
    run();

    /// Padding of the figures
    const DrawPoint figPadding(12, 12);
    const DrawPoint screenSize(VIDEODRIVER.getRenderSize());
    // Rahmen zeichnen
    borders[0]->drawFull(DrawPoint(0, 0));                                      // oben (mit Ecken)
    borders[1]->drawFull(DrawPoint(0, screenSize.y - figPadding.y));            // unten (mit Ecken)
    borders[2]->drawFull(DrawPoint(0, figPadding.y));                           // links
    borders[3]->drawFull(DrawPoint(screenSize.x - figPadding.x, figPadding.y)); // rechts

    // The figure/statues and the button bar
    glArchivItem_Bitmap& imgFigLeftTop = *LOADER.getImageN("resource", 17);
    glArchivItem_Bitmap& imgFigRightTop = *LOADER.getImageN("resource", 18);
    glArchivItem_Bitmap& imgFigLeftBot = *LOADER.getImageN("resource", 19);
    glArchivItem_Bitmap& imgFigRightBot = *LOADER.getImageN("resource", 20);
    imgFigLeftTop.drawFull(figPadding);
    imgFigRightTop.drawFull(DrawPoint(screenSize.x - figPadding.x - imgFigRightTop.getWidth(), figPadding.y));
    imgFigLeftBot.drawFull(DrawPoint(figPadding.x, screenSize.y - figPadding.y - imgFigLeftBot.getHeight()));
    imgFigRightBot.drawFull(screenSize - figPadding - imgFigRightBot.getSize());

    glArchivItem_Bitmap& imgButtonBar = *LOADER.getImageN("resource", 29);
    imgButtonBar.drawFull(
      DrawPoint((screenSize.x - imgButtonBar.getWidth()) / 2, screenSize.y - imgButtonBar.getHeight()));
}

void dskGameInterface::msgPaintAfter()
{
    Desktop::msgPaintAfter();

    const GameWorldBase& world = worldViewer.getWorld();

    if(SETTINGS.global.showGFInfo)
    {
        std::array<char, 256> nwf_string;
        const auto gfLenMs = static_cast<unsigned>(GAMECLIENT.getGFLength() / 1ms);
        const auto nwfLenMs = static_cast<unsigned>((GAMECLIENT.getNWFLength() * GAMECLIENT.getGFLength()) / 1ms);
        if(GAMECLIENT.isReplayModeOn())
        {
            snprintf(nwf_string.data(), nwf_string.size(),
                     _("(Replay-Mode) Current GF: %u (End at: %u) / GF length: %u ms / NWF length: %u gf (%u ms)"),
                     world.getEvMgr().getCurrentGF(), GAMECLIENT.getLastReplayGF(), gfLenMs, GAMECLIENT.getNWFLength(),
                     nwfLenMs);
        } else
        {
            snprintf(nwf_string.data(), nwf_string.size(),
                     _("Current GF: %u / GF length: %u ms / NWF length: %u gf (%u ms) /  Ping: %u ms"),
                     world.getEvMgr().getCurrentGF(), gfLenMs, GAMECLIENT.getNWFLength(), nwfLenMs,
                     worldViewer.getPlayer().ping);
        }
        NormalFont->draw(DrawPoint(30, 1), nwf_string.data(), FontStyle{}, COLOR_YELLOW);
    }

    // tournament mode?
    const unsigned tournamentDuration = GAMECLIENT.getTournamentModeDuration();
    if(tournamentDuration)
    {
        unsigned curGF = world.getEvMgr().getCurrentGF();
        std::string tournamentNotice;
        if(curGF >= tournamentDuration)
            tournamentNotice = _("Tournament finished");
        else
        {
            tournamentNotice =
              helpers::format("Tournament mode: %1% remaining", GAMECLIENT.formatGFTime(tournamentDuration - curGF));
        }
        NormalFont->draw(DrawPoint(VIDEODRIVER.getRenderSize().x - 30, 1), tournamentNotice, FontStyle::AlignH::RIGHT,
                         COLOR_YELLOW);
    }

    // Replaydateianzeige in der linken unteren Ecke
    if(GAMECLIENT.isReplayModeOn())
    {
        NormalFont->draw(DrawPoint(0, VIDEODRIVER.getRenderSize().y), GAMECLIENT.getReplayFilename().string(),
                         FontStyle::BOTTOM, COLOR_YELLOW);
    } else
    {
        // Laggende Spieler anzeigen in Form von Schnecken
        DrawPoint snailPos(VIDEODRIVER.getRenderSize().x - 70, 35);
        for(const NWFPlayerInfo& player : nwfInfo_->getPlayerInfos())
        {
            if(player.isLagging)
            {
                LOADER.getPlayerImage("rttr", 0)->drawFull(Rect(snailPos, 30, 30), COLOR_WHITE,
                                                           game_->world_.getPlayer(player.id).color);
                snailPos.x -= 40;
            }
        }
    }

    // Show icons in the upper right corner of the game interface
    DrawPoint iconPos(VIDEODRIVER.getRenderSize().x - 56, 32);

    // Draw cheating indicator icon (WINTER)
    if(cheats_.isCheatModeOn())
    {
        glArchivItem_Bitmap* cheatingImg = LOADER.getImageN("io", 75);
        cheatingImg->drawFull(iconPos);
        iconPos -= DrawPoint(cheatingImg->getWidth() + 6, 0);
    }

    // Draw speed indicator icon
    const int speedStep = static_cast<int>(REFERENCE_SPEED / 10ms) - static_cast<int>(GAMECLIENT.getGFLength() / 10ms);

    if(speedStep != 0)
    {
        glArchivItem_Bitmap* runnerImg = LOADER.getImageN("io", 164);

        runnerImg->drawFull(iconPos);

        if(speedStep != 1)
        {
            std::string multiplier = helpers::toString(std::abs(speedStep));
            NormalFont->draw(iconPos - runnerImg->getOrigin() + DrawPoint(19, 6), multiplier, FontStyle::LEFT,
                             speedStep > 0 ? COLOR_YELLOW : COLOR_RED);
        }
        iconPos -= DrawPoint(runnerImg->getWidth() + 4, 0);
    }

    // Draw zoom level indicator icon
    if(gwv.getCurrentTargetZoomFactor() != 1.f) //-V550
    {
        glArchivItem_Bitmap* magnifierImg = LOADER.getImageN("io", 36);

        magnifierImg->drawFull(iconPos);

        std::string zoom_percent = helpers::toString((int)(gwv.getCurrentTargetZoomFactor() * 100)) + "%";
        NormalFont->draw(iconPos - magnifierImg->getOrigin() + DrawPoint(9, 7), zoom_percent, FontStyle::CENTER,
                         COLOR_YELLOW);
        iconPos -= DrawPoint(magnifierImg->getWidth() + 4, 0);
    }
}

bool dskGameInterface::contextClick(const MouseCoords& mc)
{
    // Handle road building mode if active
    if(road.mode != RoadBuildMode::Disabled)
    {
        // in "richtige" Map-Koordinaten Konvertieren, den aktuellen selektierten Punkt
        const MapPoint selPt = gwv.getSelectedPt();

        if(selPt == road.point)
        {
            // Selektierter Punkt ist der gleiche wie der Straßenpunkt --> Fenster mit Wegbau abbrechen
            showRoadWindow(mc.pos);
        } else
        {
            // altes Roadwindow schließen
            WINDOWMANAGER.close((unsigned)CGI_ROADWINDOW);

            // Ist das ein gültiger neuer Wegpunkt?
            if(worldViewer.isRoadAvailable(road.mode == RoadBuildMode::Boat, selPt)
               && worldViewer.isPlayerTerritory(selPt))
            {
                MapPoint targetPt = selPt;
                if(!buildRoadPart(targetPt))
                    showRoadWindow(mc.pos);
            } else if(worldViewer.getBQ(selPt) != BuildingQuality::Nothing)
            {
                // Wurde bereits auf das gebaute Stück geklickt?
                unsigned idOnRoad = getIdInCurBuildRoad(selPt);
                if(idOnRoad)
                    demolishRoad(idOnRoad);
                else
                {
                    MapPoint targetPt = selPt;
                    if(buildRoadPart(targetPt))
                    {
                        // Ist der Zielpunkt der gleiche geblieben?
                        if(selPt == targetPt)
                            GI_BuildRoad();
                    } else if(selPt == targetPt)
                        showRoadWindow(mc.pos);
                }
            }
            // Wurde auf eine Flagge geklickt und ist diese Flagge nicht der Weganfangspunkt?
            else if(worldViewer.getWorld().getNO(selPt)->getType() == NodalObjectType::Flag && selPt != road.start)
            {
                MapPoint targetPt = selPt;
                if(buildRoadPart(targetPt))
                {
                    if(selPt == targetPt)
                        GI_BuildRoad();
                } else if(selPt == targetPt)
                    showRoadWindow(mc.pos);
            } else
            {
                unsigned tbr = getIdInCurBuildRoad(selPt);
                // Wurde bereits auf das gebaute Stück geklickt?
                if(tbr)
                    demolishRoad(tbr);
                else
                    showRoadWindow(mc.pos);
            }
        }
    } else // Not in road building mode
    {
        bool enable_military_buildings = false;

        iwAction::Tabs action_tabs;

        const MapPoint cSel = gwv.getSelectedPt();

        // Vielleicht steht hier auch ein Schiff?
        if(const noShip* ship = worldViewer.getShip(cSel))
        {
            WINDOWMANAGER.show(std::make_unique<iwShip>(gwv, GAMECLIENT, ship));
            return true;
        }

        // Evtl ists nen Haus? (unser Haus)
        const noBase& selObj = *worldViewer.getWorld().getNO(cSel);
        if(selObj.getType() == NodalObjectType::Building && worldViewer.isOwner(cSel))
        {
            if(auto* wnd = WINDOWMANAGER.findNonModalWindow(CGI_BUILDING + MapBase::createGUIID(cSel)))
            {
                WINDOWMANAGER.setActiveWindow(*wnd);
                return true;
            }
            BuildingType bt = static_cast<const noBuilding&>(selObj).getBuildingType();
            // HQ
            if(bt == BuildingType::Headquarters)
                WINDOWMANAGER.show(
                  std::make_unique<iwHQ>(gwv, GAMECLIENT, worldViewer.getWorldNonConst().getSpecObj<nobHQ>(cSel)));
            // Lagerhäuser
            else if(bt == BuildingType::Storehouse)
                WINDOWMANAGER.show(std::make_unique<iwBaseWarehouse>(
                  gwv, GAMECLIENT, worldViewer.getWorldNonConst().getSpecObj<nobStorehouse>(cSel)));
            // Hafengebäude
            else if(bt == BuildingType::HarborBuilding)
                WINDOWMANAGER.show(std::make_unique<iwHarborBuilding>(
                  gwv, GAMECLIENT, worldViewer.getWorldNonConst().getSpecObj<nobHarborBuilding>(cSel)));
            // Militärgebäude
            else if(BuildingProperties::isMilitary(bt))
                WINDOWMANAGER.show(std::make_unique<iwMilitaryBuilding>(
                  gwv, GAMECLIENT, worldViewer.getWorldNonConst().getSpecObj<nobMilitary>(cSel)));
            else if(bt == BuildingType::Temple)
                WINDOWMANAGER.show(std::make_unique<iwTempleBuilding>(
                  gwv, GAMECLIENT, worldViewer.getWorldNonConst().getSpecObj<nobTemple>(cSel)));
            else
                WINDOWMANAGER.show(std::make_unique<iwBuilding>(
                  gwv, GAMECLIENT, worldViewer.getWorldNonConst().getSpecObj<nobUsual>(cSel)));
            return true;
        }
        // oder vielleicht eine Baustelle?
        else if(selObj.getType() == NodalObjectType::Buildingsite && worldViewer.isOwner(cSel))
        {
            if(!WINDOWMANAGER.findNonModalWindow(CGI_BUILDING + MapBase::createGUIID(cSel)))
                WINDOWMANAGER.show(
                  std::make_unique<iwBuildingSite>(gwv, worldViewer.getWorld().getSpecObj<noBuildingSite>(cSel)));
            return true;
        }

        action_tabs.watch = true;
        // Unser Land
        if(worldViewer.isOwner(cSel))
        {
            const BuildingQuality bq = worldViewer.getBQ(cSel);
            // Kann hier was gebaut werden?
            if(bq >= BuildingQuality::Mine)
            {
                action_tabs.build = true;

                // Welches Gebäude kann gebaut werden?
                switch(bq)
                {
                    case BuildingQuality::Mine: action_tabs.build_tabs = iwAction::BuildTab::Mine; break;
                    case BuildingQuality::Hut: action_tabs.build_tabs = iwAction::BuildTab::Hut; break;
                    case BuildingQuality::House: action_tabs.build_tabs = iwAction::BuildTab::House; break;
                    case BuildingQuality::Castle: action_tabs.build_tabs = iwAction::BuildTab::Castle; break;
                    case BuildingQuality::Harbor: action_tabs.build_tabs = iwAction::BuildTab::Harbor; break;
                    default: break;
                }

                if(!worldViewer.getWorld().isFlagAround(cSel))
                    action_tabs.setflag = true;

                // Prüfen, ob sich Militärgebäude in der Nähe befinden, wenn nein, können auch eigene
                // Militärgebäude gebaut werden
                enable_military_buildings =
                  !worldViewer.getWorld().isMilitaryBuildingNearNode(cSel, worldViewer.getPlayerId());
            } else if(bq == BuildingQuality::Flag)
                action_tabs.setflag = true;
            else if(selObj.getType() == NodalObjectType::Flag)
                action_tabs.flag = true;

            if(selObj.getType() != NodalObjectType::Flag && selObj.getType() != NodalObjectType::Building)
            {
                // Check if there are roads
                for(const Direction dir : helpers::EnumRange<Direction>{})
                {
                    const PointRoad curRoad = worldViewer.getVisiblePointRoad(cSel, dir);
                    if(curRoad != PointRoad::None)
                    {
                        action_tabs.cutroad = true;
                        action_tabs.upgradeRoad |= (curRoad == PointRoad::Normal);
                    }
                }
            }
        }
        // evtl ists ein feindliches Militärgebäude, welches NICHT im Nebel liegt?
        else if(worldViewer.getVisibility(cSel) == Visibility::Visible)
        {
            if(selObj.getType() == NodalObjectType::Building)
            {
                const auto* building = worldViewer.getWorld().getSpecObj<noBuilding>(cSel); //-V807
                BuildingType bt = building->getBuildingType();

                // Only if trade is enabled
                if(worldViewer.getWorld().getGGS().isEnabled(AddonId::TRADE))
                {
                    // Allied warehouse? -> Show trade window
                    if(BuildingProperties::isWareHouse(bt) && worldViewer.getPlayer().isAlly(building->getPlayer()))
                    {
                        WINDOWMANAGER.show(std::make_unique<iwTrade>(*static_cast<const nobBaseWarehouse*>(building),
                                                                     worldViewer, GAMECLIENT));
                        return true;
                    }
                }

                // Ist es ein gewöhnliches Militärgebäude?
                if(BuildingProperties::isMilitary(bt))
                {
                    // Dann darf es nicht neu gebaut sein!
                    if(!static_cast<const nobMilitary*>(building)->isNewBuilt())
                        action_tabs.attack = true;
                }
                // oder ein HQ oder Hafen?
                else if(bt == BuildingType::Headquarters || bt == BuildingType::HarborBuilding)
                    action_tabs.attack = true;
                action_tabs.sea_attack =
                  action_tabs.attack && worldViewer.getWorld().getGGS().isEnabled(AddonId::SEA_ATTACK);
            }
        }

        // Bisheriges Actionfenster schließen, falls es eins gab
        // aktuelle Mausposition merken, da diese durch das Schließen verändert werden kann
        if(actionwindow)
            actionwindow->close();
        VIDEODRIVER.setMousePos(mc.pos);

        showActionWindow(action_tabs, cSel, mc.pos, enable_military_buildings);
    }

    return true;
}

bool dskGameInterface::msgLeftDown(const MouseCoords& mc)
{
    const glArchivItem_Bitmap& imgButtonBar = *LOADER.getImageN("resource", 29);
    const auto btOrig = DrawPoint(VIDEODRIVER.getRenderSize().x / 2 - imgButtonBar.getWidth() / 2,
                                  VIDEODRIVER.getRenderSize().y - imgButtonBar.getHeight())
                        + btOffset;
    if(isPointInRect(mc.pos, Rect(btOrig, btSize * 4u)))
        return false;

    if(!VIDEODRIVER.isTouch())
    {
        // Start scrolling also on Ctrl + left click
        if(VIDEODRIVER.getModKeyState().ctrl)
        {
            msgRightDown(mc);
            return true;
        } else if(isScrolling)
            stopScrolling();

        return contextClick(mc);

    } else if(mc.num_tfingers < 2)
        touchDuration = VIDEODRIVER.getTickCount();
    else if(isScrolling) // 2 fingers down -> zoom mode. Do not click or scroll map
        stopScrolling();

    return true;
}

bool dskGameInterface::msgLeftUp(const MouseCoords& mc)
{
    if(isScrolling)
    {
        stopScrolling();
        return true;
    }

    // num_tfingers is reduced after this function to check if it's still a touch event
    // Was touch duration short enough to trigger conext click?
    if(mc.num_tfingers == 1 && (VIDEODRIVER.getTickCount() - touchDuration) < TOUCH_MAX_CLICK_INTERVAL)
        return contextClick(mc);

    return false;
}

bool dskGameInterface::msgMouseMove(const MouseCoords& mc)
{
    if(!isScrolling)
    {
        if(mc.num_tfingers == 1)
            msgRightDown(mc);
        else
            return false;
    }

    if(SETTINGS.interface.mapScrollMode == MapScrollMode::GrabAndDrag)
    {
        const Position mapPos = gwv.viewPosToMap(mc.pos);
        gwv.moveBy(-(mapPos - startScrollPt));
        startScrollPt = mapPos;
    } else
    {
        int acceleration = SETTINGS.global.smartCursor ? 2 : 3;

        if(SETTINGS.interface.mapScrollMode == MapScrollMode::ScrollSame)
            acceleration = -acceleration;

        gwv.moveBy((mc.pos - startScrollPt) * acceleration);
        VIDEODRIVER.setMousePos(startScrollPt);

        if(!SETTINGS.global.smartCursor)
            startScrollPt = mc.pos;
    }

    return true;
}

bool dskGameInterface::msgRightDown(const MouseCoords& mc)
{
    if(SETTINGS.interface.mapScrollMode == MapScrollMode::GrabAndDrag)
        startScrolling(gwv.viewPosToMap(mc.pos));
    else
        startScrolling(mc.pos);
    return true;
}

bool dskGameInterface::msgRightUp(const MouseCoords& /*mc*/) //-V524
{
    if(isScrolling)
        stopScrolling();
    return false;
}

bool dskGameInterface::msgKeyDown(const KeyEvent& ke)
{
    cheatCommandTracker_.onKeyEvent(ke);

    switch(ke.kt)
    {
        default: break;
        case KeyType::Return: // Open chat
            WINDOWMANAGER.show(std::make_unique<iwChat>(this));
            return true;

        case KeyType::Left: // Scroll left
            gwv.moveBy({-30, 0});
            return true;
        case KeyType::Right: // Scroll right
            gwv.moveBy({30, 0});
            return true;
        case KeyType::Up: // Scroll up
            gwv.moveBy({0, -30});
            return true;
        case KeyType::Down: // Scroll down
            gwv.moveBy({0, 30});
            return true;

        case KeyType::F2: // Open save game window
            WINDOWMANAGER.toggleWindow(std::make_unique<iwSave>());
            return true;
        case KeyType::F3: // Map debug window
        {
            const bool replayMode = GAMECLIENT.isReplayModeOn();
            if(replayMode)
                disableFoW(true);
            WINDOWMANAGER.toggleWindow(std::make_unique<iwMapDebug>(gwv, game_->world_.isSinglePlayer() || replayMode));
            return true;
        }
        case KeyType::F8:
            WINDOWMANAGER.toggleWindow(std::make_unique<iwTextfile>("keyboardlayout.txt", _("Keyboard layout")));
            return true;
        case KeyType::F9:
            WINDOWMANAGER.toggleWindow(std::make_unique<iwTextfile>("readme.txt", _("Readme!")));
            return true;
        case KeyType::F10: WINDOWMANAGER.toggleWindow(std::make_unique<iwSettings>()); return true;
        case KeyType::F11: // Music player (midi files)
            WINDOWMANAGER.toggleWindow(std::make_unique<iwMusicPlayer>());
            return true;
        case KeyType::F12: // Ingame options
            WINDOWMANAGER.toggleWindow(std::make_unique<iwOptionsWindow>(gwv.getSoundMgr()));
            return true;
    }

    switch(ke.c)
    {
        case '+':
            if(GAMECLIENT.isReplayModeOn() || game_->world_.isSinglePlayer())
                GAMECLIENT.increaseSpeed();
            return true;
        case '-':
            if(GAMECLIENT.isReplayModeOn() || game_->world_.isSinglePlayer())
                GAMECLIENT.decreaseSpeed();
            return true;

        // Switch to specific player
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        {
            unsigned playerIdx = ke.c - '1';
            if(GAMECLIENT.isReplayModeOn())
            {
                unsigned oldPlayerId = worldViewer.getPlayerId();
                GAMECLIENT.changePlayerIngame(worldViewer.getPlayerId(), playerIdx);
                RTTR_Assert(worldViewer.getPlayerId() == oldPlayerId || worldViewer.getPlayerId() == playerIdx);
            } else if(playerIdx < worldViewer.getWorld().getNumPlayers())
            {
                // On multiplayer this currently asyncs, but as this is a debug feature anyway just disable it.
                // If this should be enabled again, look into the handling/clearing of accumulated GCs
                if(game_->world_.isSinglePlayer())
                {
                    const GamePlayer& player = worldViewer.getWorld().getPlayer(playerIdx);
                    if(player.ps == PlayerState::AI && player.aiInfo.type == AI::Type::Dummy)
                        GAMECLIENT.requestSwapToPlayer(playerIdx);
                }
            }
            return true;
        }

        case 'b': gwv.moveToLastPosition(); return true;
        case 'v':
            if(game_->world_.isSinglePlayer())
                GAMECLIENT.increaseSpeed(true);
            return true;
        case 'c': // Show/hide building names
            gwv.toggleShowNames();
            return true;
        case 'd': // Enable/Disable fog of war (in replay mode)
            toggleFoW();
            return true;
        case 'h': // Go to HQ
        {
            const GamePlayer& player = worldViewer.getPlayer();
            // HQ might not exist anymore
            if(player.getHQPos().isValid())
                gwv.moveToMapPt(player.getHQPos());
        }
            return true;
        case 'i': // Show inventory
            WINDOWMANAGER.toggleWindow(std::make_unique<iwInventory>(worldViewer.getPlayer()));
            return true;
        case 'j': // Skip GFs (fast forward)
            if(game_->world_.isSinglePlayer() || GAMECLIENT.isReplayModeOn())
                WINDOWMANAGER.toggleWindow(std::make_unique<iwSkipGFs>(gwv));
            return true;
        case 'l': // Show minimap
            WINDOWMANAGER.toggleWindow(std::make_unique<iwMinimap>(minimap, gwv));
            return true;
        case 'm': // Show main menu
            WINDOWMANAGER.toggleWindow(std::make_unique<iwMainMenu>(gwv, GAMECLIENT));
            return true;
        case 'n': // Show message window
            WINDOWMANAGER.toggleWindow(std::make_unique<iwPostWindow>(gwv, getPostBox()));
            updatePostIcon(getPostBox().getNumMsgs(), false);
            return true;
        case 'p': // Pause
            GAMECLIENT.togglePause();
            return true;
        case 'q': // Quit game (dialog)
            if(ke.alt)
                WINDOWMANAGER.toggleWindow(std::make_unique<iwEndgame>());
            return true;
        case 's': // Show/hide productivity overlay
            gwv.toggleShowProductivity();
            return true;
        case ' ': // Show/hide construction aid
            // workaround for Wayland which does not capture SDLK_SPACE when SDL_StartTextInput() was called
            gwv.toggleShowBQ();
            return true;
        case 26: // ctrl+z
            gwv.setZoomFactor(ZOOM_FACTORS[ZOOM_DEFAULT_INDEX]);
            return true;
        case 'z':
            if(ke.ctrl) // Reset zoom
                gwv.setZoomFactor(ZOOM_FACTORS[ZOOM_DEFAULT_INDEX]);
            else // zoom in
                gwv.setZoomFactor(getNextZoomLevel(gwv.getCurrentTargetZoomFactor()));
            return true;
        case 'Z': // shift-z, zoom out
            gwv.setZoomFactor(getPreviousZoomLevel(gwv.getCurrentTargetZoomFactor()));
            return true;
    }

    return false;
}

bool dskGameInterface::msgWheelUp(const MouseCoords&)
{
    wheelZoom(ZOOM_WHEEL_INCREMENT);
    return true;
}
bool dskGameInterface::msgWheelDown(const MouseCoords&)
{
    wheelZoom(-ZOOM_WHEEL_INCREMENT);
    return true;
}

void dskGameInterface::wheelZoom(const float step)
{
    auto targetZoomFactor = gwv.getCurrentTargetZoomFactor() * (1 + step);
    targetZoomFactor = std::clamp(targetZoomFactor, ZOOM_FACTORS.front(), ZOOM_FACTORS.back());
    if(targetZoomFactor > 1 - ZOOM_WHEEL_INCREMENT && targetZoomFactor < 1 + ZOOM_WHEEL_INCREMENT)
        targetZoomFactor = 1.f; // Snap to 100%

    gwv.setZoomFactor(targetZoomFactor);
}

void dskGameInterface::onBuildingNote(const BuildingNote& note)
{
    switch(note.type)
    {
        case BuildingNote::Constructed:
        case BuildingNote::Destroyed:
        case BuildingNote::Lost:
            // Close the related window as the building does not exist anymore
            // In "Constructed" this means the buildingsite
            WINDOWMANAGER.close(CGI_BUILDING + MapBase::createGUIID(note.pos));
            break;
        default: break;
    }
}

void dskGameInterface::run()
{
    // Reset draw counter of the trees before drawing
    noTree::resetDrawCounter();

    unsigned water_percent;
    // Draw mouse only if not on window
    bool drawMouse = WINDOWMANAGER.findWindowAtPos(VIDEODRIVER.getMousePos()) == nullptr;
    gwv.draw(road, actionwindow != nullptr ? actionwindow->getSelectedPt() : MapPoint::invalid(), drawMouse,
             &water_percent);

    // Indicate that the game is paused by darkening the screen (dark semi-transparent overlay)
    if(GAMECLIENT.isPaused())
        drawRectangle(Rect(DrawPoint(0, 0), VIDEODRIVER.getRenderSize()), COLOR_SHADOW);
    else
    {
        // Play ambient sounds if game is not paused
        worldViewer.getSoundMgr().playOceanBrawling(water_percent);
        worldViewer.getSoundMgr().playBirdSounds(noTree::queryDrawCounter());
    }

    messenger.draw();
}

void dskGameInterface::GI_StartRoadBuilding(const MapPoint startPt, bool waterRoad)
{
    // Im Replay keine Straßen bauen
    if(GAMECLIENT.isReplayModeOn())
        return;

    road.mode = waterRoad ? RoadBuildMode::Boat : RoadBuildMode::Normal;
    road.route.clear();
    road.start = road.point = startPt;
    WINDOWMANAGER.setCursor(Cursor::remove);
}

void dskGameInterface::GI_CancelRoadBuilding()
{
    if(road.mode == RoadBuildMode::Disabled)
        return;
    road.mode = RoadBuildMode::Disabled;
    worldViewer.removeVisualRoad(road.start, road.route);
    WINDOWMANAGER.setCursor(isScrolling ? Cursor::Scroll : Cursor::Hand);
}

bool dskGameInterface::buildRoadPart(MapPoint& cSel)
{
    std::vector<Direction> new_route =
      findPathForRoad(worldViewer, road.point, cSel, road.mode == RoadBuildMode::Boat, 100);
    // Weg gefunden?
    if(new_route.empty())
        return false;

    // Test on water way length
    if(road.mode == RoadBuildMode::Boat)
    {
        unsigned char index = worldViewer.getWorld().getGGS().getSelection(AddonId::MAX_WATERWAY_LENGTH);

        RTTR_Assert(index < waterwayLengths.size());
        const unsigned max_length = waterwayLengths[index];

        unsigned length = road.route.size() + new_route.size();

        // max_length == 0 heißt beliebig lang, ansonsten
        // Weg zurechtstutzen.
        if(max_length > 0)
        {
            while(length > max_length)
            {
                new_route.pop_back();
                --length;
            }
        }
    }

    // Weg (visuell) bauen
    for(const auto dir : new_route)
    {
        worldViewer.setVisiblePointRoad(road.point, dir,
                                        (road.mode == RoadBuildMode::Boat) ? PointRoad::Boat : PointRoad::Normal);
        worldViewer.recalcBQForRoad(road.point);
        road.point = worldViewer.getWorld().getNeighbour(road.point, dir);
    }
    worldViewer.recalcBQForRoad(road.point);

    // Zielpunkt updaten (für Wasserweg)
    cSel = road.point;

    road.route.insert(road.route.end(), new_route.begin(), new_route.end());

    return true;
}

unsigned dskGameInterface::getIdInCurBuildRoad(const MapPoint pt)
{
    MapPoint curPt = road.start;
    for(unsigned i = 0; i < road.route.size(); ++i)
    {
        if(curPt == pt)
            return i + 1;

        curPt = worldViewer.getNeighbour(curPt, road.route[i]);
    }
    return 0;
}

void dskGameInterface::showRoadWindow(const Position& mousePos)
{
    roadwindow = &WINDOWMANAGER.show(
      std::make_unique<iwRoadWindow>(*this, worldViewer.getBQ(road.point) != BuildingQuality::Nothing, mousePos), true);
}

void dskGameInterface::showActionWindow(const iwAction::Tabs& action_tabs, MapPoint cSel, const DrawPoint& mousePos,
                                        const bool enable_military_buildings)
{
    const GameWorldBase& world = worldViewer.getWorld();

    iwAction::Params params;

    // Sind wir am Wasser?
    if(action_tabs.setflag)
    {
        auto isWater = [](const auto& desc) { return desc.kind == TerrainKind::Water; };
        if(world.hasTerrain(cSel, isWater))
            params = iwAction::FlagType::WaterFlag;
    }

    // Wenn es einen Flaggen-Tab gibt, dann den Flaggentyp herausfinden und die Art des Fensters entsprechende setzen
    if(action_tabs.flag)
    {
        if(world.getNO(world.getNeighbour(cSel, Direction::NorthWest))->getGOT() == GO_Type::NobHq)
            params = iwAction::FlagType::HQ;
        else if(world.getNO(cSel)->getType() == NodalObjectType::Flag)
        {
            if(world.getSpecObj<noFlag>(cSel)->getFlagType() == FlagType::Water)
                params = iwAction::FlagType::WaterFlag;
        }
    }

    // Angriffstab muss wissen, wieviel Soldaten maximal entsendet werden können
    if(action_tabs.attack)
    {
        params = worldViewer.getNumSoldiersForAttack(cSel);
    }

    actionwindow = &WINDOWMANAGER.show(
      std::make_unique<iwAction>(*this, gwv, action_tabs, cSel, mousePos, params, enable_military_buildings), true);
}

void dskGameInterface::onChatCommand(const std::string& cmd)
{
    cheatCommandTracker_.onChatCommand(cmd);

    if(cmd == "surrender")
        GAMECLIENT.Surrender();
    else if(cmd == "async")
        (void)RANDOM.rand(RANDOM_CONTEXT2(0), 255);
    else if(cmd == "segfault")
    {
        char* x = nullptr;
        *x = 1; //-V522 // NOLINT
    } else if(cmd == "reload")
    {
        WorldDescription newDesc;
        GameDataLoader gdLoader(newDesc);
        if(gdLoader.load())
        {
            const_cast<GameWorld&>(game_->world_).getDescriptionWriteable() = newDesc;
            worldViewer.initTerrainRenderer();
        }
    }
}

void dskGameInterface::GI_BuildRoad()
{
    if(GAMECLIENT.BuildRoad(road.start, road.mode == RoadBuildMode::Boat, road.route))
    {
        road.mode = RoadBuildMode::Disabled;
        WINDOWMANAGER.setCursor(Cursor::Hand);
    }
}

void dskGameInterface::msgWindowClosed(IngameWindow& wnd)
{
    if(actionwindow == &wnd)
        actionwindow = nullptr;
    else if(roadwindow == &wnd)
        roadwindow = nullptr;
}

void dskGameInterface::GI_FlagDestroyed(const MapPoint pt)
{
    // Im Wegbaumodus und haben wir von hier eine Flagge gebaut?
    if(road.mode != RoadBuildMode::Disabled && road.start == pt)
    {
        GI_CancelRoadBuilding();
    }

    // Evtl Actionfenster schließen, da sich das ja auch auf diese Flagge bezieht
    if(actionwindow)
    {
        if(actionwindow->getSelectedPt() == pt)
            actionwindow->close();
    }
}

void dskGameInterface::CI_PlayerLeft(const unsigned playerId)
{
    // Info-Meldung ausgeben
    std::string text =
      helpers::format(_("Player '%s' left the game!"), worldViewer.getWorld().getPlayer(playerId).name);
    messenger.addMessage("", 0, ChatDestination::System, text, COLOR_RED);
    // Im Spiel anzeigen, dass die KI das Spiel betreten hat
    text = helpers::format(_("Player '%s' joined the game!"), "KI");
    messenger.addMessage("", 0, ChatDestination::System, text, COLOR_GREEN);
}

void dskGameInterface::CI_GGSChanged(const GlobalGameSettings& /*ggs*/)
{
    // TODO: print what has changed
    const std::string text = helpers::format(_("Note: Game settings changed by the server%s"), "");
    messenger.addMessage("", 0, ChatDestination::System, text);
}

void dskGameInterface::CI_Chat(const unsigned playerId, const ChatDestination cd, const std::string& msg)
{
    messenger.addMessage(worldViewer.getWorld().getPlayer(playerId).name,
                         worldViewer.getWorld().getPlayer(playerId).color, cd, msg);
}

void dskGameInterface::CI_Async(const std::string& checksums_list)
{
    messenger.addMessage("", 0, ChatDestination::System,
                         _("The Game is not in sync. Checksums of some players don't match."), COLOR_RED);
    messenger.addMessage("", 0, ChatDestination::System, checksums_list, COLOR_YELLOW);
    messenger.addMessage("", 0, ChatDestination::System, _("A auto-savegame is created..."), COLOR_RED);
}

void dskGameInterface::CI_ReplayAsync(const std::string& msg)
{
    messenger.addMessage("", 0, ChatDestination::System, msg, COLOR_RED);
}

void dskGameInterface::CI_ReplayEndReached(const std::string& msg)
{
    messenger.addMessage("", 0, ChatDestination::System, msg, COLOR_BLUE);
}

void dskGameInterface::CI_GamePaused()
{
    messenger.addMessage(_("SYSTEM"), COLOR_GREY, ChatDestination::System, _("Game was paused."));
}

void dskGameInterface::CI_GameResumed()
{
    messenger.addMessage(_("SYSTEM"), COLOR_GREY, ChatDestination::System, _("Game was resumed."));
}

void dskGameInterface::CI_Error(const ClientError ce)
{
    messenger.addMessage("", 0, ChatDestination::System, clientErrorToStr(ce), COLOR_RED);
    GAMECLIENT.setPause(true);
}

/**
 *  Status: Verbindung verloren.
 */
void dskGameInterface::LC_Status_ConnectionLost()
{
    messenger.addMessage("", 0, ChatDestination::System, _("Lost connection to lobby!"), COLOR_RED);
}

/**
 *  (Lobby-)Status: Benutzerdefinierter Fehler
 */
void dskGameInterface::LC_Status_Error(const std::string& error)
{
    messenger.addMessage("", 0, ChatDestination::System, error, COLOR_RED);
}

void dskGameInterface::CI_PlayersSwapped(const unsigned player1, const unsigned player2)
{
    // Meldung anzeigen
    std::string text = "Player '" + worldViewer.getWorld().getPlayer(player1).name + "' switched to player '"
                       + worldViewer.getWorld().getPlayer(player2).name + "'";
    messenger.addMessage("", 0, ChatDestination::System, text, COLOR_YELLOW);

    // Sichtbarkeiten und Minimap neu berechnen, wenn wir ein von den beiden Spielern sind
    const unsigned localPlayerId = worldViewer.getPlayerId();
    if(player1 == localPlayerId || player2 == localPlayerId)
    {
        worldViewer.changePlayer(player1 == localPlayerId ? player2 : player1);
        // Set visual settings back to the actual ones
        GAMECLIENT.resetVisualSettings();
        minimap.updateAll();
        initPlayer();
    }
}

/**
 *  Wenn ein Spieler verloren hat
 */
void dskGameInterface::GI_PlayerDefeated(const unsigned playerId)
{
    const std::string text =
      helpers::format(_("Player '%s' was defeated!"), worldViewer.getWorld().getPlayer(playerId).name);
    messenger.addMessage("", 0, ChatDestination::System, text, COLOR_ORANGE);

    /// Lokaler Spieler?
    if(playerId == worldViewer.getPlayerId())
    {
        /// Sichtbarkeiten neu berechnen
        worldViewer.recalcAllColors();
        // Minimap updaten
        minimap.updateAll();
    }
}

void dskGameInterface::GI_UpdateMinimap(const MapPoint pt)
{
    // Minimap Bescheid sagen
    minimap.updateNode(pt);
}

void dskGameInterface::GI_UpdateMapVisibility()
{
    // recalculate visibility
    worldViewer.recalcAllColors();
    // update minimap
    minimap.updateAll();
}

/**
 *  Bündnisvertrag wurde abgeschlossen oder abgebrochen --> Minimap updaten
 */
void dskGameInterface::GI_TreatyOfAllianceChanged(unsigned playerId)
{
    // Nur wenn Team-Sicht aktiviert ist, können sihc die Sichtbarkeiten auch ändern
    if(playerId == worldViewer.getPlayerId() && worldViewer.getWorld().getGGS().teamView)
    {
        /// Sichtbarkeiten neu berechnen
        worldViewer.recalcAllColors();
        // Minimap updaten
        minimap.updateAll();
    }
}

/**
 *  Baut Weg zurück von Ende bis zu start_id
 */
void dskGameInterface::demolishRoad(const unsigned start_id)
{
    RTTR_Assert(start_id > 0);
    for(unsigned i = road.route.size(); i >= start_id; --i)
    {
        MapPoint t = road.point;
        road.point = worldViewer.getWorld().getNeighbour(road.point, road.route[i - 1] + 3u);
        worldViewer.setVisiblePointRoad(road.point, road.route[i - 1], PointRoad::None);
        worldViewer.recalcBQForRoad(t);
    }

    road.route.resize(start_id - 1);
}

/**
 *  Updatet das Post-Icon mit der Nachrichtenanzahl und der Taube
 */
void dskGameInterface::updatePostIcon(const unsigned postmessages_count, bool showPigeon)
{
    // Taube setzen oder nicht (Post)
    if(postmessages_count == 0 || !showPigeon)
        getCtrl<ctrlImageButton>(3)->setImage(LOADER.getImageN("io", 62));
    else
        getCtrl<ctrlImageButton>(3)->setImage(LOADER.getImageN("io", 59));

    // und Anzahl der Postnachrichten aktualisieren
    if(postmessages_count > 0)
    {
        getCtrl<ctrlText>(ID_txtNumMsg)->setText(std::to_string(postmessages_count));
    } else
        getCtrl<ctrlText>(ID_txtNumMsg)->setText("");
}

/**
 *  Neue Post-Nachricht eingetroffen
 */
void dskGameInterface::newPostMessage(const PostMsg& msg, const unsigned msgCt)
{
    updatePostIcon(msgCt, true);
    SoundEffect soundEffect = msg.getSoundEffect();
    switch(soundEffect)
    {
        case SoundEffect::Pidgeon: LOADER.getSoundN("sound", 114)->play(100, false); break;
        case SoundEffect::Fanfare: LOADER.getSoundN("sound", 110)->play(100, false);
    }
}

/**
 *  Es wurde eine Postnachricht vom Spieler gelöscht
 */
void dskGameInterface::postMessageDeleted(const unsigned msgCt)
{
    updatePostIcon(msgCt, false);
}

/**
 *  Ein Spieler hat das Spiel gewonnen.
 */
void dskGameInterface::GI_Winner(const unsigned playerId)
{
    const std::string name = worldViewer.getWorld().getPlayer(playerId).name;
    const std::string text = (boost::format(_("Player '%s' is the winner!")) % name).str();
    messenger.addMessage("", 0, ChatDestination::System, text, COLOR_ORANGE);
    WINDOWMANAGER.show(std::make_unique<iwVictory>(std::vector<std::string>(1, name)));
}

/**
 *  Ein Team hat das Spiel gewonnen.
 */
void dskGameInterface::GI_TeamWinner(const unsigned playerMask)
{
    std::vector<std::string> winners;
    const GameWorldBase& world = worldViewer.getWorld();
    for(unsigned i = 0; i < world.getNumPlayers(); i++)
    {
        if(playerMask & (1 << i))
            winners.push_back(world.getPlayer(i).name);
    }
    const std::string text =
      (boost::format(_("%1% are the winners!")) % helpers::join(winners, ", ", _(" and "))).str();
    messenger.addMessage("", 0, ChatDestination::System, text, COLOR_ORANGE);
    WINDOWMANAGER.show(std::make_unique<iwVictory>(winners));
}
