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
#include "dskGameInterface.h"

#include "CollisionDetection.h"
#include "EventManager.h"
#include "Game.h"
#include "GameClient.h"
#include "GameManager.h"
#include "GamePlayer.h"
#include "GameServer.h"
#include "GlobalVars.h"
#include "Loader.h"
#include "Settings.h"
#include "SoundManager.h"
#include "WindowManager.h"
#include "addons/AddonMaxWaterwayLength.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobHQ.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobStorehouse.h"
#include "buildings/nobUsual.h"
#include "controls/ctrlImageButton.h"
#include "controls/ctrlText.h"
#include "driver/MouseCoords.h"
#include "drivers/VideoDriverWrapper.h"
#include "helpers/converters.h"
#include "ingameWindows/iwAction.h"
#include "ingameWindows/iwBaseWarehouse.h"
#include "ingameWindows/iwBuilding.h"
#include "ingameWindows/iwBuildingSite.h"
#include "ingameWindows/iwChat.h"
#include "ingameWindows/iwEndgame.h"
#include "ingameWindows/iwHQ.h"
#include "ingameWindows/iwHarborBuilding.h"
#include "ingameWindows/iwInventory.h"
#include "ingameWindows/iwMainMenu.h"
#include "ingameWindows/iwMapDebug.h"
#include "ingameWindows/iwMilitaryBuilding.h"
#include "ingameWindows/iwMinimap.h"
#include "ingameWindows/iwMusicPlayer.h"
#include "ingameWindows/iwOptionsWindow.h"
#include "ingameWindows/iwPostWindow.h"
#include "ingameWindows/iwRoadWindow.h"
#include "ingameWindows/iwSave.h"
#include "ingameWindows/iwShip.h"
#include "ingameWindows/iwSkipGFs.h"
#include "ingameWindows/iwTextfile.h"
#include "ingameWindows/iwTrade.h"
#include "notifications/BuildingNote.h"
#include "notifications/NotificationManager.h"
#include "ogl/SoundEffectItem.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "ogl/glArchivItem_Font.h"
#include "pathfinding/FindPathForRoad.h"
#include "postSystem/PostBox.h"
#include "postSystem/PostMsg.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldViewer.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noTree.h"
#include "gameData/BuildingProperties.h"
#include "gameData/GameConsts.h"
#include "gameData/GuiConsts.h"
#include "gameData/TerrainData.h"
#include "gameData/const_gui_ids.h"
#include "liblobby/LobbyClient.h"
#include "libutil/Log.h"
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/if.hpp>
#include <boost/lambda/lambda.hpp>
#include <algorithm>
#include <sstream>

namespace {
enum
{
    ID_btMap,
    ID_btOptions,
    ID_btConstructionAid,
    ID_btPost,
    ID_txtNumMsg
};
}

dskGameInterface::dskGameInterface(boost::shared_ptr<Game> game)
    : Desktop(NULL), game_(game), worldViewer(GAMECLIENT.GetPlayerId(), game->world),
      gwv(worldViewer, Point<int>(0, 0), VIDEODRIVER.GetScreenSize()), cbb(*LOADER.GetPaletteN("pal5")), actionwindow(NULL),
      roadwindow(NULL), minimap(worldViewer), isScrolling(false), zoomLvl(ZOOM_DEFAULT_INDEX), isCheatModeOn(false)
{
    road.mode = RM_DISABLED;
    road.point = MapPoint(0, 0);
    road.start = MapPoint(0, 0);

    SetScale(false);

    DrawPoint barPos((GetSize().x - LOADER.GetImageN("resource", 29)->getWidth()) / 2 + 44,
                     GetSize().y - LOADER.GetImageN("resource", 29)->getHeight() + 4);

    Extent btSize = Extent(37, 32);
    AddImageButton(ID_btMap, barPos, btSize, TC_GREEN1, LOADER.GetImageN("io", 50), _("Map"))->SetBorder(false);
    barPos.x += btSize.x;
    AddImageButton(ID_btOptions, barPos, btSize, TC_GREEN1, LOADER.GetImageN("io", 192), _("Main selection"))->SetBorder(false);
    barPos.x += btSize.x;
    AddImageButton(ID_btConstructionAid, barPos, btSize, TC_GREEN1, LOADER.GetImageN("io", 83), _("Construction aid mode"))
      ->SetBorder(false);
    barPos.x += btSize.x;
    AddImageButton(ID_btPost, barPos, btSize, TC_GREEN1, LOADER.GetImageN("io", 62), _("Post office"))->SetBorder(false);
    barPos += DrawPoint(18, 24);

    AddText(ID_txtNumMsg, barPos, "", COLOR_YELLOW, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER, SmallFont);

    LOBBYCLIENT.SetInterface(this);
    GAMECLIENT.SetInterface(this);
    game->world.SetGameInterface(this);

    std::fill(borders.begin(), borders.end(), (glArchivItem_Bitmap*)(NULL));
    cbb.loadEdges(*LOADER.GetInfoN("resource"));
    cbb.buildBorder(VIDEODRIVER.GetScreenSize(), borders);

    InitPlayer();
    worldViewer.InitTerrainRenderer();
}

void dskGameInterface::InitPlayer()
{
    // Jump to players HQ if it exists
    if(worldViewer.GetPlayer().GetHQPos().isValid())
        gwv.MoveToMapPt(worldViewer.GetPlayer().GetHQPos());

    namespace bl = boost::lambda;
    using bl::_1;
    evBld = worldViewer.GetWorld().GetNotifications().subscribe<BuildingNote>(
      bl::if_(bl::bind(&BuildingNote::player, _1) == worldViewer.GetPlayerId())[bl::bind(&dskGameInterface::OnBuildingNote, this, _1)]);
    PostBox& postBox = GetPostBox();
    postBox.ObserveNewMsg(boost::bind(&dskGameInterface::NewPostMessage, this, _1, _2));
    postBox.ObserveDeletedMsg(boost::bind(&dskGameInterface::PostMessageDeleted, this, _1));
    UpdatePostIcon(postBox.GetNumMsgs(), true);
}

PostBox& dskGameInterface::GetPostBox()
{
    PostBox* postBox = worldViewer.GetWorld().GetPostMgr().GetPostBox(worldViewer.GetPlayerId());
    if(!postBox)
        postBox = worldViewer.GetWorldNonConst().GetPostMgr().AddPostBox(worldViewer.GetPlayerId());
    RTTR_Assert(postBox != NULL);
    return *postBox;
}

dskGameInterface::~dskGameInterface()
{
    for(unsigned i = 0; i < borders.size(); i++)
        deletePtr(borders[i]);
    GAMECLIENT.RemoveInterface(this);
    LOBBYCLIENT.RemoveInterface(this);
}

void dskGameInterface::SetActive(bool activate)
{
    if(activate == IsActive())
        return;
    Desktop::SetActive(activate);
    if(activate && GAMECLIENT.GetState() == GameClient::CS_LOADING)
    {
        // Wir sind nun ingame
        GAMECLIENT.GameStarted();
    }
    if(!activate)
    {
        isScrolling = false;
        GAMEMANAGER.SetCursor(CURSOR_HAND);
    } else if(road.mode != RM_DISABLED)
        GAMEMANAGER.SetCursor(CURSOR_RM);
}

void dskGameInterface::SettingsChanged() {}

void dskGameInterface::Resize(const Extent& newSize)
{
    Window::Resize(newSize);

    // recreate borders
    for(unsigned i = 0; i < borders.size(); i++)
        deletePtr(borders[i]);
    cbb.buildBorder(newSize, borders);

    // move buttons
    DrawPoint barPos((newSize.x - LOADER.GetImageN("resource", 29)->getWidth()) / 2 + 44,
                     newSize.y - LOADER.GetImageN("resource", 29)->getHeight() + 4);

    ctrlButton* button = GetCtrl<ctrlButton>(ID_btMap);
    button->SetPos(barPos);

    barPos.x += button->GetSize().x;
    button = GetCtrl<ctrlButton>(ID_btOptions);
    button->SetPos(barPos);

    barPos.x += button->GetSize().x;
    button = GetCtrl<ctrlButton>(ID_btConstructionAid);
    button->SetPos(barPos);

    barPos.x += button->GetSize().x;
    button = GetCtrl<ctrlButton>(ID_btPost);
    button->SetPos(barPos);

    barPos += DrawPoint(18, 24);
    ctrlText* text = GetCtrl<ctrlText>(ID_txtNumMsg);
    text->SetPos(barPos);

    gwv.Resize(newSize);
}

void dskGameInterface::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_btMap: WINDOWMANAGER.Show(new iwMinimap(minimap, gwv)); break;
        case ID_btOptions: WINDOWMANAGER.Show(new iwMainMenu(gwv, GAMECLIENT)); break;
        case ID_btConstructionAid:
            if(WINDOWMANAGER.IsDesktopActive())
                gwv.ToggleShowBQ();
            break;
        case ID_btPost:
            WINDOWMANAGER.Show(new iwPostWindow(gwv, GetPostBox()));
            UpdatePostIcon(GetPostBox().GetNumMsgs(), false);
            break;
    }
}

void dskGameInterface::Msg_PaintBefore()
{
    Desktop::Msg_PaintBefore();

    // Spiel ausführen
    Run();

    /// Padding of the figures
    const DrawPoint figPadding(12, 12);
    const DrawPoint screenSize(VIDEODRIVER.GetScreenSize());
    // Rahmen zeichnen
    borders[0]->DrawFull(DrawPoint(0, 0));                                      // oben (mit Ecken)
    borders[1]->DrawFull(DrawPoint(0, screenSize.y - figPadding.y));            // unten (mit Ecken)
    borders[2]->DrawFull(DrawPoint(0, figPadding.y));                           // links
    borders[3]->DrawFull(DrawPoint(screenSize.x - figPadding.x, figPadding.y)); // rechts

    // The figure/statues and the button bar
    glArchivItem_Bitmap& imgFigLeftTop = *LOADER.GetImageN("resource", 17);
    glArchivItem_Bitmap& imgFigRightTop = *LOADER.GetImageN("resource", 18);
    glArchivItem_Bitmap& imgFigLeftBot = *LOADER.GetImageN("resource", 19);
    glArchivItem_Bitmap& imgFigRightBot = *LOADER.GetImageN("resource", 20);
    imgFigLeftTop.DrawFull(figPadding);
    imgFigRightTop.DrawFull(DrawPoint(screenSize.x - figPadding.x - imgFigRightTop.getWidth(), figPadding.y));
    imgFigLeftBot.DrawFull(DrawPoint(figPadding.x, screenSize.y - figPadding.y - imgFigLeftBot.getHeight()));
    imgFigRightBot.DrawFull(screenSize - figPadding - imgFigRightBot.GetSize());

    glArchivItem_Bitmap& imgButtonBar = *LOADER.GetImageN("resource", 29);
    imgButtonBar.DrawFull(DrawPoint((screenSize.x - imgButtonBar.getWidth()) / 2, screenSize.y - imgButtonBar.getHeight()));
}

void dskGameInterface::Msg_PaintAfter()
{
    /* NWF-Anzeige (vorläufig)*/
    char nwf_string[256];

    const GameWorldBase& world = worldViewer.GetWorld();
    if(GAMECLIENT.IsReplayModeOn())
    {
        snprintf(nwf_string, 255, _("(Replay-Mode) Current GF: %u (End at: %u) / GF length: %u ms / NWF length: %u gf (%u ms)"),
                 world.GetEvMgr().GetCurrentGF(), GAMECLIENT.GetLastReplayGF(), GAMECLIENT.GetGFLength(), GAMECLIENT.GetNWFLength(),
                 GAMECLIENT.GetNWFLength() * GAMECLIENT.GetGFLength());
    } else
        snprintf(nwf_string, 255, _("Current GF: %u / GF length: %u ms / NWF length: %u gf (%u ms) /  Ping: %u ms"),
                 world.GetEvMgr().GetCurrentGF(), GAMECLIENT.GetGFLength(), GAMECLIENT.GetNWFLength(),
                 GAMECLIENT.GetNWFLength() * GAMECLIENT.GetGFLength(), worldViewer.GetPlayer().ping);

    // tournament mode?
    unsigned tmd = GAMECLIENT.GetTournamentModeDuration();

    if(tmd)
    {
        // Convert gf to seconds
        unsigned sec = (tmd - world.GetEvMgr().GetCurrentGF()) * GAMECLIENT.GetGFLength() / 1000;
        char str[512];
        sprintf(str, "tournament mode: %02u:%02u:%02u remaining", sec / 3600, (sec / 60) % 60, sec % 60);
    }

    NormalFont->Draw(DrawPoint(30, 1), nwf_string, 0, 0xFFFFFF00);

    // Replaydateianzeige in der linken unteren Ecke
    if(GAMECLIENT.IsReplayModeOn())
        NormalFont->Draw(DrawPoint(0, VIDEODRIVER.GetScreenSize().y), GAMECLIENT.GetReplayFileName(), glArchivItem_Font::DF_BOTTOM,
                         0xFFFFFF00);

    // Laggende Spieler anzeigen in Form von Schnecken
    DrawPoint snailPos(VIDEODRIVER.GetScreenSize().x - 70, 35);
    for(unsigned i = 0; i < world.GetPlayerCount(); ++i)
    {
        const GamePlayer& player = world.GetPlayer(i);
        if(player.is_lagging)
            LOADER.GetPlayerImage("rttr", 0)->DrawFull(Rect(snailPos, 30, 30), COLOR_WHITE, player.color);
        snailPos.x -= 40;
    }

    // Show icons in the upper right corner of the game interface
    DrawPoint iconPos(VIDEODRIVER.GetScreenSize().x - 56, 32);

    // Draw cheating indicator icon (WINTER) - Single Player only!
    if(isCheatModeOn)
    {
        glArchivItem_Bitmap* cheatingImg = LOADER.GetImageN("io", 75);
        const DrawPoint drawPos(iconPos);

        iconPos -= DrawPoint(cheatingImg->getWidth() + 6, 0);
        cheatingImg->DrawFull(drawPos);
    }

    // Draw speed indicator icon
    const int startSpeed = SPEED_GF_LENGTHS[game_->ggs.speed];
    const int speedStep = startSpeed / 10 - static_cast<int>(GAMECLIENT.GetGFLength()) / 10;

    if(speedStep != 0)
    {
        glArchivItem_Bitmap* runnerImg = LOADER.GetImageN("io", 164);
        const DrawPoint drawPos(iconPos);

        iconPos -= DrawPoint(runnerImg->getWidth() + 4, 0);
        runnerImg->DrawFull(drawPos);

        if(speedStep != 1)
        {
            std::string multiplier = helpers::toString(std::abs(speedStep));
            NormalFont->Draw(drawPos - runnerImg->GetOrigin() + DrawPoint(19, 6), multiplier, glArchivItem_Font::DF_LEFT,
                             speedStep > 0 ? COLOR_YELLOW : COLOR_RED);
        }
    }

    // Draw zoom level indicator icon
    if(gwv.GetCurrentTargetZoomFactor() != 1.f)
    {
        glArchivItem_Bitmap* magnifierImg = LOADER.GetImageN("io", 36);
        const DrawPoint drawPos(iconPos);

        iconPos -= DrawPoint(magnifierImg->getWidth() + 4, 0);
        magnifierImg->DrawFull(drawPos);

        std::string zoom_percent = helpers::toString((int)(gwv.GetCurrentTargetZoomFactor() * 100)) + "%";
        NormalFont->Draw(drawPos - magnifierImg->GetOrigin() + DrawPoint(9, 7), zoom_percent, glArchivItem_Font::DF_CENTER, COLOR_YELLOW);
    }
}

bool dskGameInterface::Msg_LeftDown(const MouseCoords& mc)
{
    DrawPoint btOrig(VIDEODRIVER.GetScreenSize().x / 2 - LOADER.GetImageN("resource", 29)->getWidth() / 2 + 44,
                     VIDEODRIVER.GetScreenSize().y - LOADER.GetImageN("resource", 29)->getHeight() + 4);
    Extent btSize = Extent(37, 32) * 4u;
    if(IsPointInRect(mc.GetPos(), Rect(btOrig, btSize)))
        return false;

    // Start scrolling also on Ctrl + left click
    if(VIDEODRIVER.GetModKeyState().ctrl)
    {
        Msg_RightDown(mc);
        return true;
    }

    // Unterscheiden je nachdem Straäcnbaumodus an oder aus ist
    if(road.mode)
    {
        // in "richtige" Map-Koordinaten Konvertieren, den aktuellen selektierten Punkt
        const MapPoint selPt = gwv.GetSelectedPt();

        if(selPt == road.point)
        {
            // Selektierter Punkt ist der gleiche wie der Straßenpunkt --> Fenster mit Wegbau abbrechen
            ShowRoadWindow(mc.GetPos());
        } else
        {
            // altes Roadwindow schließen
            WINDOWMANAGER.Close((unsigned)CGI_ROADWINDOW);

            // Ist das ein gültiger neuer Wegpunkt?
            if(worldViewer.IsRoadAvailable(road.mode == RM_BOAT, selPt) && worldViewer.IsOwner(selPt)
               && worldViewer.GetWorld().IsPlayerTerritory(selPt))
            {
                MapPoint targetPt = selPt;
                if(!BuildRoadPart(targetPt))
                    ShowRoadWindow(mc.GetPos());
            } else if(worldViewer.GetBQ(selPt) != BQ_NOTHING)
            {
                // Wurde bereits auf das gebaute Stück geklickt?
                unsigned idOnRoad = GetIdInCurBuildRoad(selPt);
                if(idOnRoad)
                    DemolishRoad(idOnRoad);
                else
                {
                    MapPoint targetPt = selPt;
                    if(BuildRoadPart(targetPt))
                    {
                        // Ist der Zielpunkt der gleiche geblieben?
                        if(selPt == targetPt)
                            GI_BuildRoad();
                    } else if(selPt == targetPt)
                        ShowRoadWindow(mc.GetPos());
                }
            }
            // Wurde auf eine Flagge geklickt und ist diese Flagge nicht der Weganfangspunkt?
            else if(worldViewer.GetWorld().GetNO(selPt)->GetType() == NOP_FLAG && selPt != road.start)
            {
                MapPoint targetPt = selPt;
                if(BuildRoadPart(targetPt))
                {
                    if(selPt == targetPt)
                        GI_BuildRoad();
                } else if(selPt == targetPt)
                    ShowRoadWindow(mc.GetPos());
            } else
            {
                unsigned tbr = GetIdInCurBuildRoad(selPt);
                // Wurde bereits auf das gebaute Stück geklickt?
                if(tbr)
                    DemolishRoad(tbr);
                else
                    ShowRoadWindow(mc.GetPos());
            }
        }
    } else
    {
        bool enable_military_buildings = false;

        iwAction::Tabs action_tabs;

        const MapPoint cSel = gwv.GetSelectedPt();

        // Vielleicht steht hier auch ein Schiff?
        if(noShip* ship = worldViewer.GetShip(cSel))
        {
            WINDOWMANAGER.Show(new iwShip(gwv, GAMECLIENT, ship));
            return true;
        }

        // Evtl ists nen Haus? (unser Haus)
        const noBase& selObj = *worldViewer.GetWorld().GetNO(cSel);
        if(selObj.GetType() == NOP_BUILDING && worldViewer.IsOwner(cSel))
        {
            BuildingType bt = static_cast<const noBuilding&>(selObj).GetBuildingType();
            // HQ
            if(bt == BLD_HEADQUARTERS)
                // WINDOWMANAGER.Show(new iwTrade(gwv,this,gwb.GetSpecObj<nobHQ>(cselx,csely)));
                WINDOWMANAGER.Show(new iwHQ(gwv, GAMECLIENT, worldViewer.GetWorldNonConst().GetSpecObj<nobHQ>(cSel)));
            // Lagerhäuser
            else if(bt == BLD_STOREHOUSE)
                WINDOWMANAGER.Show(new iwBaseWarehouse(gwv, GAMECLIENT, worldViewer.GetWorldNonConst().GetSpecObj<nobStorehouse>(cSel)));
            // Hafengebäude
            else if(bt == BLD_HARBORBUILDING)
                WINDOWMANAGER.Show(
                  new iwHarborBuilding(gwv, GAMECLIENT, worldViewer.GetWorldNonConst().GetSpecObj<nobHarborBuilding>(cSel)));
            // Militärgebäude
            else if(BuildingProperties::IsMilitary(bt))
                WINDOWMANAGER.Show(new iwMilitaryBuilding(gwv, GAMECLIENT, worldViewer.GetWorldNonConst().GetSpecObj<nobMilitary>(cSel)));
            else
                WINDOWMANAGER.Show(new iwBuilding(gwv, GAMECLIENT, worldViewer.GetWorldNonConst().GetSpecObj<nobUsual>(cSel)));
            return true;
        }
        // oder vielleicht eine Baustelle?
        else if(selObj.GetType() == NOP_BUILDINGSITE && worldViewer.IsOwner(cSel))
        {
            WINDOWMANAGER.Show(new iwBuildingSite(gwv, worldViewer.GetWorld().GetSpecObj<noBuildingSite>(cSel)));
            return true;
        }

        action_tabs.watch = true;
        // Unser Land
        if(worldViewer.IsOwner(cSel))
        {
            const BuildingQuality bq = worldViewer.GetBQ(cSel);
            // Kann hier was gebaut werden?
            if(bq >= BQ_HUT)
            {
                action_tabs.build = true;

                // Welches Gebäude kann gebaut werden?
                switch(bq)
                {
                    case BQ_HUT: action_tabs.build_tabs = iwAction::Tabs::BT_HUT; break;
                    case BQ_HOUSE: action_tabs.build_tabs = iwAction::Tabs::BT_HOUSE; break;
                    case BQ_CASTLE: action_tabs.build_tabs = iwAction::Tabs::BT_CASTLE; break;
                    case BQ_MINE: action_tabs.build_tabs = iwAction::Tabs::BT_MINE; break;
                    case BQ_HARBOR: action_tabs.build_tabs = iwAction::Tabs::BT_HARBOR; break;
                    default: break;
                }

                if(!worldViewer.GetWorld().IsFlagAround(cSel))
                    action_tabs.setflag = true;

                // Prüfen, ob sich Militärgebäude in der Nähe befinden, wenn nein, können auch eigene
                // Militärgebäude gebaut werden
                enable_military_buildings = !worldViewer.GetWorld().IsMilitaryBuildingNearNode(cSel, worldViewer.GetPlayerId());
            } else if(bq == BQ_FLAG)
                action_tabs.setflag = true;
            else if(selObj.GetType() == NOP_FLAG)
                action_tabs.flag = true;

            // Prüfen, ob irgendwo Straßen anliegen
            bool roads = false;
            for(unsigned i = 0; i < 6; ++i)
                if(worldViewer.GetVisiblePointRoad(cSel, Direction::fromInt(i)))
                    roads = true;

            if((roads) && !(selObj.GetType() == NOP_FLAG || selObj.GetType() == NOP_BUILDING))
                action_tabs.cutroad = true;
        }
        // evtl ists ein feindliches Militärgebäude, welches NICHT im Nebel liegt?
        else if(worldViewer.GetVisibility(cSel) == VIS_VISIBLE)
        {
            if(selObj.GetType() == NOP_BUILDING)
            {
                const noBuilding* building = worldViewer.GetWorld().GetSpecObj<noBuilding>(cSel);
                BuildingType bt = building->GetBuildingType();

                // Only if trade is enabled
                if(worldViewer.GetWorld().GetGGS().isEnabled(AddonId::TRADE))
                {
                    // Allied warehouse? -> Show trade window
                    if(BuildingProperties::IsWareHouse(bt) && worldViewer.GetPlayer().IsAlly(building->GetPlayer()))
                    {
                        WINDOWMANAGER.Show(new iwTrade(*static_cast<const nobBaseWarehouse*>(building), worldViewer, GAMECLIENT));
                        return true;
                    }
                }

                // Ist es ein gewöhnliches Militärgebäude?
                if(BuildingProperties::IsMilitary(bt))
                {
                    // Dann darf es nicht neu gebaut sein!
                    if(!static_cast<const nobMilitary*>(building)->IsNewBuilt())
                        action_tabs.attack = true;
                }
                // oder ein HQ oder Hafen?
                else if(bt == BLD_HEADQUARTERS || bt == BLD_HARBORBUILDING)
                    action_tabs.attack = true;
                action_tabs.sea_attack = action_tabs.attack && worldViewer.GetWorld().GetGGS().isEnabled(AddonId::SEA_ATTACK);
            }
        }

        // Bisheriges Actionfenster schließen, falls es eins gab
        // aktuelle Mausposition merken, da diese durch das Schließen verändert werden kann
        WINDOWMANAGER.Close(actionwindow);
        VIDEODRIVER.SetMousePos(mc.GetPos());

        ShowActionWindow(action_tabs, cSel, mc.GetPos(), enable_military_buildings);
    }

    return true;
}

bool dskGameInterface::Msg_LeftUp(const MouseCoords& /*mc*/)
{
    isScrolling = false;
    return false;
}

bool dskGameInterface::Msg_MouseMove(const MouseCoords& mc)
{
    if(!isScrolling)
        return false;

    int acceleration = SETTINGS.global.smartCursor ? 2 : 3;

    if(SETTINGS.interface.revert_mouse)
        acceleration = -acceleration;

    gwv.MoveTo((mc.x - startScrollPt.x) * acceleration, (mc.y - startScrollPt.y) * acceleration);
    VIDEODRIVER.SetMousePos(startScrollPt.x, startScrollPt.y);

    if(!SETTINGS.global.smartCursor)
        startScrollPt = Point<int>(mc.x, mc.y);
    return true;
}

bool dskGameInterface::Msg_RightDown(const MouseCoords& mc)
{
    startScrollPt = Point<int>(mc.x, mc.y);
    isScrolling = true;
    GAMEMANAGER.SetCursor(CURSOR_SCROLL);
    return false;
}

bool dskGameInterface::Msg_RightUp(const MouseCoords& /*mc*/) //-V524
{
    isScrolling = false;
    GAMEMANAGER.SetCursor(road.mode == RM_DISABLED ? CURSOR_HAND : CURSOR_RM);
    return false;
}

/**
 *  Druck von Spezialtasten auswerten.
 */
bool dskGameInterface::Msg_KeyDown(const KeyEvent& ke)
{
    switch(ke.kt)
    {
        default: break;
        case KT_RETURN: // Chatfenster öffnen
            WINDOWMANAGER.Show(new iwChat);
            return true;

        case KT_SPACE: // Bauqualitäten anzeigen
            gwv.ToggleShowBQ();
            return true;

        case KT_LEFT: // Nach Links Scrollen
            gwv.MoveToX(-30);
            return true;
        case KT_RIGHT: // Nach Rechts Scrollen
            gwv.MoveToX(30);
            return true;
        case KT_UP: // Nach Oben Scrollen
            gwv.MoveToY(-30);
            return true;
        case KT_DOWN: // Nach Unten Scrollen
            gwv.MoveToY(30);
            return true;

        case KT_F2: // Spiel speichern
            WINDOWMANAGER.Show(new iwSave);
            return true;
        case KT_F3: // Map debug window/ Multiplayer coordinates
            WINDOWMANAGER.Show(new iwMapDebug(gwv, game_->world.IsSinglePlayer() || GAMECLIENT.IsReplayModeOn()));
            return true;
        case KT_F8: // Tastaturbelegung
            WINDOWMANAGER.Show(new iwTextfile("keyboardlayout.txt", _("Keyboard layout")));
            return true;
        case KT_F9: // Readme
            WINDOWMANAGER.Show(new iwTextfile("readme.txt", _("Readme!")));
            return true;
        case KT_F10: { bool allowHumanAI = isCheatModeOn;
#ifndef NDEBUG
            allowHumanAI = true;
#endif // !NDEBUG
            if(GAMECLIENT.GetState() == GameClient::CS_GAME && allowHumanAI && !GAMECLIENT.IsReplayModeOn())
                GAMECLIENT.ToggleHumanAIPlayer();
            return true;
        }
        case KT_F11: // Music player (midi files)
            WINDOWMANAGER.Show(new iwMusicPlayer);
            return true;
        case KT_F12: // Optionsfenster
            WINDOWMANAGER.Show(new iwOptionsWindow());
            return true;
    }

    static std::string winterCheat = "winter";
    switch(ke.c)
    {
        case 'w':
        case 'i':
        case 'n':
        case 't':
        case 'e':
        case 'r':
            curCheatTxt += char(ke.c);
            if(winterCheat.find(curCheatTxt) == 0)
            {
                if(curCheatTxt == winterCheat)
                {
                    isCheatModeOn = !isCheatModeOn;
                    curCheatTxt.clear();
                }
            } else
                curCheatTxt.clear();
            break;
    }

    switch(ke.c)
    {
        case '+':
            if(GAMECLIENT.IsReplayModeOn())
            {
                GAMECLIENT.IncreaseReplaySpeed();
            } else if(game_->world.IsSinglePlayer())
            {
                GAMECLIENT.IncreaseSpeed();
            }
            return true;
        case '-':
            if(GAMECLIENT.IsReplayModeOn())
            {
                GAMECLIENT.DecreaseReplaySpeed();
            } else if(game_->world.IsSinglePlayer())
            {
                GAMECLIENT.DecreaseSpeed();
            }
            return true;

        case '1':
        case '2':
        case '3': // Spieler umschalten
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        {
            unsigned playerIdx = ke.c - '1';
            if(GAMECLIENT.IsReplayModeOn())
            {
                unsigned oldPlayerId = worldViewer.GetPlayerId();
                GAMECLIENT.ChangePlayerIngame(worldViewer.GetPlayerId(), playerIdx);
                RTTR_Assert(worldViewer.GetPlayerId() == oldPlayerId || worldViewer.GetPlayerId() == playerIdx);
            } else if(playerIdx < worldViewer.GetWorld().GetPlayerCount())
            {
                const GamePlayer& player = worldViewer.GetWorld().GetPlayer(playerIdx);
                if(player.ps == PS_AI && player.aiInfo.type == AI::DUMMY)
                    GAMECLIENT.RequestSwapToPlayer(playerIdx);
            }
        }
            return true;

        case 'b': // Zur lezten Position zurückspringen
            gwv.MoveToLastPosition();
            return true;
        case 'v':
            if(game_->world.IsSinglePlayer())
                GAMECLIENT.IncreaseSpeed();
            return true;
        case 'c': // Gebäudenamen anzeigen
            gwv.ToggleShowNames();
            return true;
        case 'd': // Replay: FoW an/ausschalten
            // GameClient Bescheid sagen
            GAMECLIENT.ToggleReplayFOW();
            // Sichtbarkeiten neu setzen auf der Map-Anzeige und der Minimap
            worldViewer.RecalcAllColors();
            minimap.UpdateAll();
            return true;
        case 'h': // Zum HQ springen
        {
            const GamePlayer& player = worldViewer.GetPlayer();
            // Prüfen, ob dieses überhaupt noch existiert
            if(player.GetHQPos().isValid())
                gwv.MoveToMapPt(player.GetHQPos());
        }
            return true;
        case 'i': // Show inventory
            WINDOWMANAGER.Show(new iwInventory(worldViewer.GetPlayer()));
            return true;
        case 'j': // GFs überspringen
            if(game_->world.IsSinglePlayer() || GAMECLIENT.IsReplayModeOn())
                WINDOWMANAGER.Show(new iwSkipGFs(gwv));
            return true;
        case 'l': // Minimap anzeigen
            WINDOWMANAGER.Show(new iwMinimap(minimap, gwv));
            return true;
        case 'm': // Hauptauswahl
            WINDOWMANAGER.Show(new iwMainMenu(gwv, GAMECLIENT));
            return true;
        case 'n': // Show Post window
            WINDOWMANAGER.Show(new iwPostWindow(gwv, GetPostBox()));
            UpdatePostIcon(GetPostBox().GetNumMsgs(), false);
            return true;
        case 'p': // Pause
            if(GAMECLIENT.IsHost())
                GAMESERVER.SetPaused(!GAMECLIENT.IsPaused());
            else if(GAMECLIENT.IsReplayModeOn())
                GAMECLIENT.ToggleReplayPause();
            return true;
        case 'q': // Spiel verlassen
            if(ke.alt)
                WINDOWMANAGER.Show(new iwEndgame);
            return true;
        case 's': // Produktivität anzeigen
            gwv.ToggleShowProductivity();
            return true;
        case 26: // ctrl+z
            gwv.SetZoomFactor(ZOOM_FACTORS[ZOOM_DEFAULT_INDEX]);
            return true;
        case 'z': // zoom
            if(++zoomLvl >= ZOOM_FACTORS.size())
                zoomLvl = 0;

            gwv.SetZoomFactor(ZOOM_FACTORS[zoomLvl]);
            return true;
        case 'Z': // shift-z, reverse zoom
            if(zoomLvl == 0)
                zoomLvl = ZOOM_FACTORS.size() - 1;
            else
                zoomLvl--;

            gwv.SetZoomFactor(ZOOM_FACTORS[zoomLvl]);
            return true;
    }

    return false;
}

bool dskGameInterface::Msg_WheelUp(const MouseCoords& mc)
{
    WheelZoom(ZOOM_WHEEL_INCREMENT);
    return true;
}
bool dskGameInterface::Msg_WheelDown(const MouseCoords& mc)
{
    WheelZoom(-ZOOM_WHEEL_INCREMENT);
    return true;
}

void dskGameInterface::WheelZoom(float step)
{
    float new_zoom = gwv.GetCurrentTargetZoomFactor() * (1 + step);
    gwv.SetZoomFactor(new_zoom);

    // also keep track in terms of fixed defined zoom levels
    zoomLvl = ZOOM_DEFAULT_INDEX;
    for(size_t i = ZOOM_DEFAULT_INDEX; i < ZOOM_FACTORS.size(); ++i)
        if(ZOOM_FACTORS[i] < new_zoom)
            zoomLvl = i;

    for(size_t i = ZOOM_DEFAULT_INDEX; i-- > 0;)
        if(ZOOM_FACTORS[i] > new_zoom)
            zoomLvl = i;
}

void dskGameInterface::OnBuildingNote(const BuildingNote& note)
{
    switch(note.type)
    {
        case BuildingNote::Constructed:
        case BuildingNote::Destroyed:
        case BuildingNote::Lost:
            // Close the related window as the building does not exist anymore
            // In "Constructed" this means the buildingsite
            WINDOWMANAGER.Close(worldViewer.GetWorld().CreateGUIID(note.pos));
            break;
        default: break;
    }
}

void dskGameInterface::Run()
{
    // Reset draw counter of the trees before drawing
    noTree::ResetDrawCounter();

    unsigned water_percent;
    // Draw mouse only if not on window
    bool drawMouse = WINDOWMANAGER.FindWindowAtPos(VIDEODRIVER.GetMousePos()) == NULL;
    gwv.Draw(road, actionwindow != NULL ? actionwindow->GetSelectedPt() : MapPoint::Invalid(), drawMouse, &water_percent);

    // Evtl Meeresrauschen-Sounds abspieln
    SOUNDMANAGER.PlayOceanBrawling(water_percent);
    SOUNDMANAGER.PlayBirdSounds(noTree::QueryDrawCounter());

    // Indicate that the game is paused by darkening the screen (dark semi-transparent overlay)
    if(GAMECLIENT.IsPaused())
    {
        DrawRectangle(Rect(DrawPoint(0, 0), VIDEODRIVER.GetScreenSize()), COLOR_SHADOW);
    }

    messenger.Draw();
}

void dskGameInterface::GI_StartRoadBuilding(const MapPoint startPt, bool waterRoad)
{
    // Im Replay und in der Pause keine Straßen bauen
    if(GAMECLIENT.IsReplayModeOn() || GAMECLIENT.IsPaused())
        return;

    road.mode = waterRoad ? RM_BOAT : RM_NORMAL;
    road.route.clear();
    road.start = road.point = startPt;
    GAMEMANAGER.SetCursor(CURSOR_RM);
}

void dskGameInterface::GI_CancelRoadBuilding()
{
    if(road.mode == RM_DISABLED)
        return;
    road.mode = RM_DISABLED;
    worldViewer.RemoveVisualRoad(road.start, road.route);
    GAMEMANAGER.SetCursor(isScrolling ? CURSOR_SCROLL : CURSOR_RM);
}

bool dskGameInterface::BuildRoadPart(MapPoint& cSel)
{
    std::vector<Direction> new_route = FindPathForRoad(worldViewer, road.point, cSel, road.mode == RM_BOAT, 100);
    // Weg gefunden?
    if(new_route.empty())
        return false;

    // Test on water way length
    if(road.mode == RM_BOAT)
    {
        unsigned char index = worldViewer.GetWorld().GetGGS().getSelection(AddonId::MAX_WATERWAY_LENGTH);

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
    for(unsigned i = 0; i < new_route.size(); ++i)
    {
        worldViewer.SetVisiblePointRoad(road.point, new_route[i], (road.mode == RM_BOAT) ? 3 : 1);
        worldViewer.RecalcBQForRoad(road.point);
        road.point = worldViewer.GetWorld().GetNeighbour(road.point, new_route[i]);
    }
    worldViewer.RecalcBQForRoad(road.point);

    // Zielpunkt updaten (für Wasserweg)
    cSel = road.point;

    road.route.insert(road.route.end(), new_route.begin(), new_route.end());

    return true;
}

unsigned dskGameInterface::GetIdInCurBuildRoad(const MapPoint pt)
{
    MapPoint curPt = road.start;
    for(unsigned i = 0; i < road.route.size(); ++i)
    {
        if(curPt == pt)
            return i + 1;

        curPt = worldViewer.GetNeighbour(curPt, road.route[i]);
    }
    return 0;
}

void dskGameInterface::ShowRoadWindow(const DrawPoint& mousePos)
{
    roadwindow = new iwRoadWindow(*this, worldViewer.GetBQ(road.point) != BQ_NOTHING, mousePos);
    WINDOWMANAGER.Show(roadwindow, true);
}

void dskGameInterface::ShowActionWindow(const iwAction::Tabs& action_tabs, MapPoint cSel, const DrawPoint& mousePos,
                                        const bool enable_military_buildings)
{
    const GameWorldBase& world = worldViewer.GetWorld();

    unsigned params = 0;

    // Sind wir am Wasser?
    if(action_tabs.setflag)
    {
        for(unsigned x = 0; x < Direction::COUNT; ++x)
        {
            if(TerrainData::IsWater(world.GetRightTerrain(cSel, Direction::fromInt(x))))
                params = iwAction::AWFT_WATERFLAG;
        }
    }

    // Wenn es einen Flaggen-Tab gibt, dann den Flaggentyp herausfinden und die Art des Fensters entsprechende setzen
    if(action_tabs.flag)
    {
        if(world.GetNO(world.GetNeighbour(cSel, Direction::NORTHWEST))->GetGOT() == GOT_NOB_HQ)
            params = iwAction::AWFT_HQ;
        else if(world.GetNO(cSel)->GetType() == NOP_FLAG)
        {
            if(world.GetSpecObj<noFlag>(cSel)->GetFlagType() == FT_WATER)
                params = iwAction::AWFT_WATERFLAG;
        }
    }

    // Angriffstab muss wissen, wieviel Soldaten maximal entsendet werden können
    if(action_tabs.attack)
    {
        params = worldViewer.GetNumSoldiersForAttack(cSel);
    }

    actionwindow = new iwAction(*this, gwv, action_tabs, cSel, mousePos, params, enable_military_buildings);
    WINDOWMANAGER.Show(actionwindow, true);
}

void dskGameInterface::GI_BuildRoad()
{
    GAMECLIENT.BuildRoad(road.start, road.mode == RM_BOAT, road.route);
    road.mode = RM_DISABLED;
    GAMEMANAGER.SetCursor(CURSOR_HAND);
}

void dskGameInterface::GI_WindowClosed(Window* wnd)
{
    if(actionwindow == wnd)
        actionwindow = NULL;
    else if(roadwindow == wnd)
        roadwindow = NULL;
    else
        return;
    isScrolling = false;
}

void dskGameInterface::GI_FlagDestroyed(const MapPoint pt)
{
    // Im Wegbaumodus und haben wir von hier eine Flagge gebaut?
    if(road.mode != RM_DISABLED && road.start == pt)
    {
        GI_CancelRoadBuilding();
    }

    // Evtl Actionfenster schließen, da sich das ja auch auf diese Flagge bezieht
    if(actionwindow)
    {
        if(actionwindow->GetSelectedPt() == pt)
            WINDOWMANAGER.Close(actionwindow);
    }
}

void dskGameInterface::CI_PlayerLeft(const unsigned playerId)
{
    // Info-Meldung ausgeben
    char text[256];
    snprintf(text, sizeof(text), _("Player '%s' left the game!"), worldViewer.GetWorld().GetPlayer(playerId).name.c_str());
    messenger.AddMessage("", 0, CD_SYSTEM, text, COLOR_RED);
    // Im Spiel anzeigen, dass die KI das Spiel betreten hat
    snprintf(text, sizeof(text), _("Player '%s' joined the game!"), "KI");
    messenger.AddMessage("", 0, CD_SYSTEM, text, COLOR_GREEN);
}

void dskGameInterface::CI_GGSChanged(const GlobalGameSettings& /*ggs*/)
{
    // TODO: print what has changed
    char text[256];
    snprintf(text, sizeof(text), _("Note: Game settings changed by the server%s"), "");
    messenger.AddMessage("", 0, CD_SYSTEM, text);
}

void dskGameInterface::CI_Chat(const unsigned playerId, const ChatDestination cd, const std::string& msg)
{
    char from[256];
    snprintf(from, sizeof(from), _("<%s> "), worldViewer.GetWorld().GetPlayer(playerId).name.c_str());
    messenger.AddMessage(from, worldViewer.GetWorld().GetPlayer(playerId).color, cd, msg);
}

void dskGameInterface::CI_Async(const std::string& checksums_list)
{
    messenger.AddMessage("", 0, CD_SYSTEM, _("The Game is not in sync. Checksums of some players don't match."), COLOR_RED);
    messenger.AddMessage("", 0, CD_SYSTEM, checksums_list, COLOR_YELLOW);
    messenger.AddMessage("", 0, CD_SYSTEM, _("A auto-savegame is created..."), COLOR_RED);
}

void dskGameInterface::CI_ReplayAsync(const std::string& msg)
{
    messenger.AddMessage("", 0, CD_SYSTEM, msg, COLOR_RED);
}

void dskGameInterface::CI_ReplayEndReached(const std::string& msg)
{
    messenger.AddMessage("", 0, CD_SYSTEM, msg, COLOR_BLUE);
}

void dskGameInterface::CI_GamePaused()
{
    char from[256];
    snprintf(from, sizeof(from), _("<%s> "), _("SYSTEM"));
    messenger.AddMessage(from, COLOR_GREY, CD_SYSTEM, _("Game was paused."));

    /// Straßenbau ggf. abbrechen, wenn aktiviert
    if(road.mode != RM_DISABLED)
    {
        // Fenster schließen
        if(roadwindow)
        {
            roadwindow->Close();
            roadwindow = NULL;
        }
        GI_CancelRoadBuilding();
    }
}

void dskGameInterface::CI_GameResumed()
{
    char from[256];
    snprintf(from, sizeof(from), _("<%s> "), _("SYSTEM"));
    messenger.AddMessage(from, COLOR_GREY, CD_SYSTEM, _("Game was resumed."));
}

void dskGameInterface::CI_Error(const ClientError ce)
{
    switch(ce)
    {
        default: break;

        case CE_CONNECTIONLOST: { messenger.AddMessage("", 0, CD_SYSTEM, _("Lost connection to server!"), COLOR_RED);
        }
        break;
    }
}

/**
 *  Status: Verbindung verloren.
 */
void dskGameInterface::LC_Status_ConnectionLost()
{
    messenger.AddMessage("", 0, CD_SYSTEM, _("Lost connection to lobby!"), COLOR_RED);
}

/**
 *  (Lobby-)Status: Benutzerdefinierter Fehler
 */
void dskGameInterface::LC_Status_Error(const std::string& error)
{
    messenger.AddMessage("", 0, CD_SYSTEM, error, COLOR_RED);
}

void dskGameInterface::CI_PlayersSwapped(const unsigned player1, const unsigned player2)
{
    // Meldung anzeigen
    std::string text = "Player '" + worldViewer.GetWorld().GetPlayer(player1).name + "' switched to player '"
                       + worldViewer.GetWorld().GetPlayer(player2).name + "'";
    messenger.AddMessage("", 0, CD_SYSTEM, text, COLOR_YELLOW);

    // Sichtbarkeiten und Minimap neu berechnen, wenn wir ein von den beiden Spielern sind
    const unsigned localPlayerId = worldViewer.GetPlayerId();
    if(player1 == localPlayerId || player2 == localPlayerId)
    {
        worldViewer.ChangePlayer(player1 == localPlayerId ? player2 : player1);
        // Set visual settings back to the actual ones
        GAMECLIENT.ResetVisualSettings();
        minimap.UpdateAll();
        InitPlayer();
    }
}

/**
 *  Wenn ein Spieler verloren hat
 */
void dskGameInterface::GI_PlayerDefeated(const unsigned playerId)
{
    char text[256];
    snprintf(text, sizeof(text), _("Player '%s' was defeated!"), worldViewer.GetWorld().GetPlayer(playerId).name.c_str());
    messenger.AddMessage("", 0, CD_SYSTEM, text, COLOR_ORANGE);

    /// Lokaler Spieler?
    if(playerId == worldViewer.GetPlayerId())
    {
        /// Sichtbarkeiten neu berechnen
        worldViewer.RecalcAllColors();
        // Minimap updaten
        minimap.UpdateAll();
    }
}

void dskGameInterface::GI_UpdateMinimap(const MapPoint pt)
{
    // Minimap Bescheid sagen
    minimap.UpdateNode(pt);
}

/**
 *  Bündnisvertrag wurde abgeschlossen oder abgebrochen --> Minimap updaten
 */
void dskGameInterface::GI_TreatyOfAllianceChanged(unsigned playerId)
{
    // Nur wenn Team-Sicht aktiviert ist, können sihc die Sichtbarkeiten auch ändern
    if(playerId == worldViewer.GetPlayerId() && worldViewer.GetWorld().GetGGS().teamView)
    {
        /// Sichtbarkeiten neu berechnen
        worldViewer.RecalcAllColors();
        // Minimap updaten
        minimap.UpdateAll();
    }
}

/**
 *  Baut Weg zurück von Ende bis zu start_id
 */
void dskGameInterface::DemolishRoad(const unsigned start_id)
{
    RTTR_Assert(start_id > 0);
    for(unsigned i = road.route.size(); i >= start_id; --i)
    {
        MapPoint t = road.point;
        road.point = worldViewer.GetWorld().GetNeighbour(road.point, road.route[i - 1] + 3u);
        worldViewer.SetVisiblePointRoad(road.point, road.route[i - 1], 0);
        worldViewer.RecalcBQForRoad(t);
    }

    road.route.resize(start_id - 1);
}

/**
 *  Updatet das Post-Icon mit der Nachrichtenanzahl und der Taube
 */
void dskGameInterface::UpdatePostIcon(const unsigned postmessages_count, bool showPigeon)
{
    // Taube setzen oder nicht (Post)
    if(postmessages_count == 0 || !showPigeon)
        GetCtrl<ctrlImageButton>(3)->SetImage(LOADER.GetImageN("io", 62));
    else
        GetCtrl<ctrlImageButton>(3)->SetImage(LOADER.GetImageN("io", 59));

    // und Anzahl der Postnachrichten aktualisieren
    if(postmessages_count > 0)
    {
        std::stringstream ss;
        ss << postmessages_count;
        GetCtrl<ctrlText>(ID_txtNumMsg)->SetText(ss.str());
    } else
        GetCtrl<ctrlText>(ID_txtNumMsg)->SetText("");
}

/**
 *  Neue Post-Nachricht eingetroffen
 */
void dskGameInterface::NewPostMessage(const PostMsg& msg, const unsigned msgCt)
{
    UpdatePostIcon(msgCt, true);
    SoundEffect soundEffect = msg.GetSoundEffect();
    switch(boost::native_value(soundEffect))
    {
        case SoundEffect::Pidgeon: LOADER.GetSoundN("sound", 114)->Play(100, false); break;
        case SoundEffect::Fanfare: LOADER.GetSoundN("sound", 110)->Play(100, false);
    }
}

/**
 *  Es wurde eine Postnachricht vom Spieler gelöscht
 */
void dskGameInterface::PostMessageDeleted(const unsigned msgCt)
{
    UpdatePostIcon(msgCt, false);
}

/**
 *  Ein Spieler hat das Spiel gewonnen.
 */
void dskGameInterface::GI_Winner(const unsigned playerId)
{
    char text[256];
    snprintf(text, sizeof(text), _("Player '%s' is the winner!"), worldViewer.GetWorld().GetPlayer(playerId).name.c_str());
    messenger.AddMessage("", 0, CD_SYSTEM, text, COLOR_ORANGE);
}
/**
 *  Ein Team hat das Spiel gewonnen.
 */
void dskGameInterface::GI_TeamWinner(const unsigned playerId)
{
    unsigned winnercount = 0;
    char winners[5];
    const GameWorldBase& world = worldViewer.GetWorld();
    for(unsigned i = 0; i < world.GetPlayerCount() && winnercount < 5; i++)
    {
        winners[winnercount] = i;
        winnercount += playerId & (1 << i) ? 1 : 0;
    }
    char text[256];
    switch(winnercount)
    {
        case 2:
            snprintf(text, sizeof(text), _("Team victory! '%s' and '%s' are the winners!"), world.GetPlayer(winners[0]).name.c_str(),
                     world.GetPlayer(winners[1]).name.c_str());
            break;
        case 3:
            snprintf(text, sizeof(text), _("Team victory! '%s' and '%s' and '%s' are the winners!"),
                     world.GetPlayer(winners[0]).name.c_str(), world.GetPlayer(winners[1]).name.c_str(),
                     world.GetPlayer(winners[2]).name.c_str());
            break;
        case 4:
            snprintf(text, sizeof(text), _("Team victory! '%s' and '%s' and '%s' and '%s' are the winners!"),
                     world.GetPlayer(winners[0]).name.c_str(), world.GetPlayer(winners[1]).name.c_str(),
                     world.GetPlayer(winners[2]).name.c_str(), world.GetPlayer(winners[3]).name.c_str());
            break;
        default: snprintf(text, sizeof(text), "%s", _("Team victory!")); break;
    }
    messenger.AddMessage("", 0, CD_SYSTEM, text, COLOR_ORANGE);
}
