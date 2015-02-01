// $Id: dskGameInterface.cpp 9592 2015-02-01 09:39:38Z marcus $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#include "main.h"
#include "dskGameInterface.h"

#include "files.h"
#include "VideoDriverWrapper.h"
#include "Settings.h"
#include "GlobalVars.h"
#include "ListDir.h"
#include "GameWorld.h"
#include "WindowManager.h"
#include "SoundManager.h"
#include "EventManager.h"
#include "GameServer.h"
#include "GameClient.h"
#include "GameClientPlayer.h"
#include "VideoDriverWrapper.h"
#include "LobbyClient.h"
#include "ctrlButton.h"
#include "GameMessages.h"
#include "GameManager.h"

#include "iwChat.h"
#include "iwHQ.h"
#include "iwInventory.h"
#include "iwMusicPlayer.h"
#include "iwStorehouse.h"
#include "iwHarborBuilding.h"
#include "iwAction.h"
#include "iwRoadWindow.h"
#include "iwBuildingSite.h"
#include "iwMainMenu.h"
#include "iwPostWindow.h"
#include "iwBuilding.h"
#include "iwMilitaryBuilding.h"
#include "iwSkipGFs.h"
#include "iwMinimap.h"
#include "iwSave.h"
#include "iwTextfile.h"
#include "iwOptionsWindow.h"
#include "iwEndgame.h"
#include "iwShip.h"
#include "iwTrade.h"

#include "nobHQ.h"
#include "nobHarborBuilding.h"
#include "noBuildingSite.h"
#include "nobMilitary.h"
#include "nobStorehouse.h"
#include "nobUsual.h"
#include <sstream>

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p dskGameInterface.
 *  Startet das Spiel und lädt alles Notwendige.
 *
 *  @author OLiver
 */
dskGameInterface::dskGameInterface()
    : Desktop(NULL),
      gwv(GameClient::inst().QueryGameWorldViewer()), cbb(LOADER.GetPaletteN("pal5")),
      actionwindow(NULL), roadwindow(NULL),
      selected_x(0), selected_y(0), minimap(*gwv)
{
    road.mode = RM_DISABLED;
    road.point_x = 0;
    road.point_y = 0;
    road.start_x = 0;
    road.start_y = 0;

    SetScale(false);;

    int barx = (VideoDriverWrapper::inst().GetScreenWidth() - LOADER.GetImageN("resource", 29)->getWidth()) / 2 + 44;
    int bary = VideoDriverWrapper::inst().GetScreenHeight() - LOADER.GetImageN("resource", 29)->getHeight() + 4;

    AddImageButton(0, barx,        bary, 37, 32, TC_GREEN1, LOADER.GetImageN("io",  50), _("Map"))
    ->SetBorder(false);
    AddImageButton(1, barx + 37,   bary, 37, 32, TC_GREEN1, LOADER.GetImageN("io", 192), _("Main selection"))
    ->SetBorder(false);
    AddImageButton(2, barx + 37 * 2, bary, 37, 32, TC_GREEN1, LOADER.GetImageN("io",  83), _("Construction aid mode"))
    ->SetBorder(false);
    AddImageButton(3, barx + 37 * 3, bary, 37, 32, TC_GREEN1, LOADER.GetImageN("io",  62), _("Post office"))
    ->SetBorder(false);

    AddText(4, barx + 37 * 3 + 18, bary + 24, "", COLOR_YELLOW, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER, SmallFont);

    LOBBYCLIENT.SetInterface(this);
    GAMECLIENT.SetInterface(this);

    // Wir sind nun ingame
    GLOBALVARS.ingame = true;
    gwv->SetGameInterface(this);

    cbb.loadEdges( &LOADER.GetFiles().find("resource")->second );
    cbb.buildBorder(VideoDriverWrapper::inst().GetScreenWidth(),
                    VideoDriverWrapper::inst().GetScreenHeight(), &borders);

    // Kann passieren dass schon Nachrichten vorliegen, bevor es uns gab (insb. HQ-Landverlust)
    if (GAMECLIENT.GetPostMessages().size() > 0)
        CI_NewPostMessage(GAMECLIENT.GetPostMessages().size());
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Destruktor von @p dskGameInterface.
 *  Beendet das Spiel und räumt alles auf.
 *
 *  @author OLiver
 */
dskGameInterface::~dskGameInterface()
{
    GLOBALVARS.ingame = false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskGameInterface::SettingsChanged(void)
{
}

void dskGameInterface::Resize_(unsigned short width, unsigned short height)
{
    // recreate borders
    cbb.buildBorder(width, height, &borders);

    // move buttons
    int barx = (width - LOADER.GetImageN("resource", 29)->getWidth()) / 2 + 44;
    int bary = height - LOADER.GetImageN("resource", 29)->getHeight() + 4;

    ctrlImageButton* button = GetCtrl<ctrlImageButton>(0);
    button->Move(barx, bary, true);

    button = GetCtrl<ctrlImageButton>(1);
    button->Move(barx + 37, bary, true);

    button = GetCtrl<ctrlImageButton>(2);
    button->Move(barx + 37 * 2, bary, true);

    button = GetCtrl<ctrlImageButton>(3);
    button->Move(barx + 37 * 3, bary, true);
    ctrlText* text = GetCtrl<ctrlText>(4);
    text->Move(barx + 37 * 3 + 18, bary + 24);

    this->gwv->Resize(width, height);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskGameInterface::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 0: // Karte
        {
            WindowManager::inst().Show(new iwMinimap(&minimap, *gwv));
        } break;
        case 1: // Optionen
        {
            WindowManager::inst().Show(new iwMainMenu(gwv, this));
        } break;
        case 2: // Baukosten
        {
            if(WindowManager::inst().IsDesktopActive())
                gwv->ShowBQ();
        } break;
        case 3: // Post
        {
            WindowManager::inst().Show(new iwPostWindow(*gwv));
            UpdatePostIcon(GAMECLIENT.GetPostMessages().size(), false);
        } break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskGameInterface::Msg_PaintBefore()
{
    // Spiel ausführen
    Run();

    // Rahmen zeichnen
    dynamic_cast<glArchivItem_Bitmap*>(borders.get(0))->Draw(0, 0); // oben (mit Ecken)
    dynamic_cast<glArchivItem_Bitmap*>(borders.get(1))->Draw(0, VideoDriverWrapper::inst().GetScreenHeight() - 12); // unten (mit Ecken)
    dynamic_cast<glArchivItem_Bitmap*>(borders.get(2))->Draw(0, 12); // links
    dynamic_cast<glArchivItem_Bitmap*>(borders.get(3))->Draw(VideoDriverWrapper::inst().GetScreenWidth() - 12, 12); // rechts

    LOADER.GetImageN("resource", 17)->Draw(12, 12, 0, 0, 0, 0, 0, 0);
    LOADER.GetImageN("resource", 18)->Draw(VideoDriverWrapper::inst().GetScreenWidth() - 12 - LOADER.GetImageN("resource", 18)->getWidth(), 12, 0, 0, 0, 0, 0, 0);
    LOADER.GetImageN("resource", 19)->Draw(12, VideoDriverWrapper::inst().GetScreenHeight() - 12 - LOADER.GetImageN("resource", 19)->getHeight(), 0, 0, 0, 0, 0, 0);
    LOADER.GetImageN("resource", 20)->Draw(VideoDriverWrapper::inst().GetScreenWidth() - 12 - LOADER.GetImageN("resource", 20)->getWidth(), VideoDriverWrapper::inst().GetScreenHeight() - 12 - LOADER.GetImageN("resource", 20)->getHeight(), 0, 0, 0, 0, 0, 0);
    LOADER.GetImageN("resource", 29)->Draw(VideoDriverWrapper::inst().GetScreenWidth() / 2 - LOADER.GetImageN("resource", 29)->getWidth() / 2, VideoDriverWrapper::inst().GetScreenHeight() - LOADER.GetImageN("resource", 29)->getHeight(), 0, 0, 0, 0, 0, 0);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskGameInterface::Msg_PaintAfter()
{
    /* NWF-Anzeige (vorläufig)*/
    char nwf_string[256];

    if(GameClient::inst().IsReplayModeOn())
        snprintf(nwf_string, 255, _("(Replay-Mode) Current GF: %u (End at: %u) / GF length: %u ms / NWF length: %u gf (%u ms)"), GAMECLIENT.GetGFNumber(), GAMECLIENT.GetLastReplayGF(), GAMECLIENT.GetGFLength(), GAMECLIENT.GetNWFLength(), GAMECLIENT.GetNWFLength() * GAMECLIENT.GetGFLength());
    else
        snprintf(nwf_string, 255, _("Current GF: %u / GF length: %u ms / NWF length: %u gf (%u ms) /  Ping: %u ms"), GAMECLIENT.GetGFNumber(), GAMECLIENT.GetGFLength(), GAMECLIENT.GetNWFLength(), GAMECLIENT.GetNWFLength() * GAMECLIENT.GetGFLength(), GAMECLIENT.GetLocalPlayer()->ping);

    // tournament mode?
    unsigned tmd = GameClient::inst().GetTournamentModeDuration();

    if(tmd)
    {
        // Convert gf to seconds
        unsigned sec = (GameClient::inst().GetGGS().game_objective - OBJECTIVES_COUNT) * 60 -
                       GameClient::inst().GetGFNumber() * GameClient::inst().GetGFLength() / 1000;
        char str[512];
        sprintf(str, "tournament mode: %02u:%02u:%02u remaining", sec / 3600, (sec / 60) % 60, sec % 60);
    }

    NormalFont->Draw(30, 1, nwf_string, 0, 0xFFFFFF00);

    // Replaydateianzeige in der linken unteren Ecke
    if(GameClient::inst().IsReplayModeOn())
        NormalFont->Draw(0, VideoDriverWrapper::inst().GetScreenHeight(), GameClient::inst().GetReplayFileName(), glArchivItem_Font::DF_BOTTOM, 0xFFFFFF00);

    // Mauszeiger
    if(road.mode != RM_DISABLED)
    {
        if(VideoDriverWrapper::inst().IsLeftDown())
            GAMEMANAGER.SetCursor(CURSOR_RM_PRESSED, /*once*/ true);
        else
            GAMEMANAGER.SetCursor(CURSOR_RM, /*once*/ true);
    }
    else if(VideoDriverWrapper::inst().IsRightDown())
    {
        GAMEMANAGER.SetCursor(CURSOR_SCROLL, /*once*/ true);
    }

    // Laggende Spieler anzeigen in Form von Schnecken
    for(unsigned int i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
    {
        GameClientPlayer* player = GAMECLIENT.GetPlayer(i);
        if(player->is_lagging)
            LOADER.GetImageN("rttr", 0)->Draw(VideoDriverWrapper::inst().GetScreenWidth() - 70 - i * 40, 35, 30, 30, 0, 0, 0, 0,  COLOR_WHITE, COLORS[player->color]);
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
bool dskGameInterface::Msg_LeftDown(const MouseCoords& mc)
{
    if(Coll(mc.x, mc.y, VideoDriverWrapper::inst().GetScreenWidth() / 2 - LOADER.GetImageN("resource", 29)->getWidth() / 2 + 44,
            VideoDriverWrapper::inst().GetScreenHeight() - LOADER.GetImageN("resource", 29)->getHeight() + 4, 37 * 4, 32 * 4))
        return false;

    // Start scrolling also on Ctrl + left click
    if(VideoDriverWrapper::inst().GetModKeyState().ctrl)
    {
        gwv->MouseDown(mc);
        return true;
    }

    // Unterscheiden je nachdem Straäcnbaumodus an oder aus ist
    if(road.mode)
    {
        // in "richtige" Map-Koordinaten Konvertieren, den aktuellen selektierten Punkt
        int cselx = gwv->GetSelX(), csely = gwv->GetSelY();
        // Um auf Wasserweglängenbegrenzun reagieren zu können:
        int cselx2 = cselx, csely2 = csely;

        if(cselx == road.point_x && csely == road.point_y)
        {
            // Selektierter Punkt ist der gleiche wie der Straßenpunkt --> Fenster mit Wegbau abbrechen
            ShowRoadWindow(mc.x, mc.y);
        }
        else
        {
            // altes Roadwindow schließen
            WindowManager::inst().Close((unsigned int)CGI_ROADWINDOW);

            // Ist das ein gültiger neuer Wegpunkt?
            if(gwv->RoadAvailable(road.mode == RM_BOAT, cselx, csely, 0xFF) && gwv->GetNode(cselx, csely).owner - 1 == (signed)GAMECLIENT.GetPlayerID() &&
                    gwv->IsPlayerTerritory(cselx, csely))
            {
                if(!BuildRoadPart(cselx, csely, false))
                    ShowRoadWindow(mc.x, mc.y);
            }
            else if(gwv->CalcBQ(cselx, csely, GAMECLIENT.GetPlayerID(), 1))
            {
                // Wurde bereits auf das gebaute Stück geklickt?
                unsigned tbr;
                if((tbr = TestBuiltRoad(cselx, csely)))
                    DemolishRoad(tbr);
                else
                {
                    if(BuildRoadPart(cselx, csely, 1))
                    {
                        // Ist der Zielpunkt der gleiche geblieben?
                        if (cselx2 == cselx && csely2 == csely)
                            CommandBuildRoad();
                    }
                    else if (cselx2 == cselx && csely2 == csely)
                        ShowRoadWindow(mc.x, mc.y);
                }
            }
            // Wurde auf eine Flagge geklickt und ist diese Flagge nicht der Weganfangspunkt?
            else if(gwv->GetNO(cselx, csely)->GetType() == NOP_FLAG && !(cselx == road.start_x && csely == road.start_y))
            {
                if(BuildRoadPart(cselx, csely, 1))
                {
                    if (cselx2 == cselx && csely2 == csely)
                        CommandBuildRoad();
                }
                else if (cselx2 == cselx && csely2 == csely)
                    ShowRoadWindow(mc.x, mc.y);
            }

            else
            {
                unsigned tbr;
                // Wurde bereits auf das gebaute Stück geklickt?
                if((tbr = TestBuiltRoad(cselx, csely)))
                    DemolishRoad(tbr);
                else
                    ShowRoadWindow(mc.x, mc.y);
            }
        }
    }
    else
    {
        bool enable_military_buildings = false;

        iwAction::Tabs action_tabs;

        unsigned short cselx = gwv->GetSelX(), csely = gwv->GetSelY();


        // Vielleicht steht hier auch ein Schiff?
        if(noShip* ship = gwv->GetShip(cselx, csely, GameClient::inst().GetPlayerID()))
        {
            WindowManager::inst().Show(new iwShip(gwv, this, ship));
            return true;
        }

        // Evtl ists nen Haus? (unser Haus)
        if(gwv->GetNO(cselx, csely)->GetType() == NOP_BUILDING   && gwv->GetNode(cselx, csely).owner - 1 == (signed)GAMECLIENT.GetPlayerID())
        {
            BuildingType bt = static_cast<noBuilding*>(gwv->GetNO(cselx, csely))->GetBuildingType();
            // HQ
            if(bt == BLD_HEADQUARTERS)
                //WindowManager::inst().Show(new iwTrade(gwv,this,gwv->GetSpecObj<nobHQ>(cselx,csely)));
                WindowManager::inst().Show(new iwHQ(gwv,this, gwv->GetSpecObj<nobHQ>(cselx, csely), _("Headquarters"), 3));
            // Lagerhäuser
            else if(bt == BLD_STOREHOUSE)
                WindowManager::inst().Show(new iwStorehouse(gwv, this, gwv->GetSpecObj<nobStorehouse>(cselx, csely)));
            // Hafengebäude
            else if(bt == BLD_HARBORBUILDING)
                WindowManager::inst().Show(new iwHarborBuilding(gwv, this, gwv->GetSpecObj<nobHarborBuilding>(cselx, csely)));
            // Militärgebäude
            else if(bt <= BLD_FORTRESS)
                WindowManager::inst().Show(new iwMilitaryBuilding(gwv, this, gwv->GetSpecObj<nobMilitary>(cselx, csely)));
            else
                WindowManager::inst().Show(new iwBuilding(gwv, this, gwv->GetSpecObj<nobUsual>(cselx, csely)));
            return true;
        }

        // oder vielleicht eine Baustelle?
        else if(gwv->GetNO(cselx, csely)->GetType() == NOP_BUILDINGSITE && gwv->GetNode(cselx, csely).owner - 1 == (signed)GAMECLIENT.GetPlayerID())
        {
            WindowManager::inst().Show(new iwBuildingSite(gwv, gwv->GetSpecObj<noBuildingSite>(cselx, csely)));
            return true;

        }


        action_tabs.watch = true;
        // Unser Land
        if(gwv->GetNode(cselx, csely).owner == GAMECLIENT.GetPlayerID() + 1)
        {
            // Kann hier was gebaut werden?
            if(gwv->GetNode(gwv->GetSelX(), gwv->GetSelY()).bq >= BQ_HUT)
            {
                action_tabs.build = true;

                // Welches Gebäude kann gebaut werden?
                switch(gwv->GetNode(gwv->GetSelX(), gwv->GetSelY()).bq)
                {
                    case BQ_HUT: action_tabs.build_tabs = iwAction::Tabs::BT_HUT; break;
                    case BQ_HOUSE: action_tabs.build_tabs = iwAction::Tabs::BT_HOUSE; break;
                    case BQ_CASTLE: action_tabs.build_tabs = iwAction::Tabs::BT_CASTLE; break;
                    case BQ_MINE: action_tabs.build_tabs = iwAction::Tabs::BT_MINE; break;
                    case BQ_HARBOR: action_tabs.build_tabs = iwAction::Tabs::BT_HARBOR; break;
                    default: break;
                }

                if(!gwv->FlagNear(cselx, csely))
                    action_tabs.setflag = true;

                // Prüfen, ob sich Militärgebäude in der Nähe befinden, wenn nein, können auch eigene
                // Militärgebäude gebaut werden
                enable_military_buildings = !gwv->IsMilitaryBuildingNearNode(cselx, csely, GAMECLIENT.GetPlayerID());
            }
            else if(gwv->GetNode(gwv->GetSelX(), gwv->GetSelY()).bq == BQ_FLAG)
                action_tabs.setflag = true;



            if(gwv->GetNO(cselx, csely)->GetType() == NOP_FLAG)
                action_tabs.flag = true;

            // Prüfen, ob irgendwo Straßen anliegen
            bool roads = false;
            for(unsigned i = 0; i < 6; ++i)
                if(gwv->GetPointRoad(cselx, csely, i, true))
                    roads = true;

            if( (roads) && !(
                        gwv->GetNO(cselx, csely)->GetType() == NOP_FLAG ||
                        gwv->GetNO(cselx, csely)->GetType() == NOP_BUILDING) )
                action_tabs.cutroad = true;
        }
        // evtl ists ein feindliches Militärgebäude, welches NICHT im Nebel liegt?
        else if(gwv->GetVisibility(cselx, csely) == VIS_VISIBLE)
        {
            if(gwv->GetNO(cselx, csely)->GetType() == NOP_BUILDING)
            {
                noBuilding* building = gwv->GetSpecObj<noBuilding>(cselx, csely);
                BuildingType bt = building->GetBuildingType();

                // Only if trade is enabled
                if(GameClient::inst().GetGGS().isEnabled(ADDON_TRADE))
                {
                    // Allied warehouse? -> Show trade window
                    if(GameClient::inst().GetLocalPlayer()->IsAlly(building->GetPlayer())
                            && (bt == BLD_HEADQUARTERS || bt == BLD_HARBORBUILDING || bt == BLD_STOREHOUSE))
                    {
                        WindowManager::inst().Show(new iwTrade(gwv, this, static_cast<nobBaseWarehouse*>(building)));
                        return true;
                    }
                }

                // Ist es ein gewöhnliches Militärgebäude?
                if(bt >= BLD_BARRACKS && bt <= BLD_FORTRESS)
                {
                    // Dann darf es nicht neu gebaut sein!
                    if(!gwv->GetSpecObj<nobMilitary>(cselx, csely)->IsNewBuilt())
                        action_tabs.attack = action_tabs.sea_attack = true;
                }
                // oder ein HQ oder Hafen?
                else if(bt == BLD_HEADQUARTERS || bt == BLD_HARBORBUILDING)
                    action_tabs.attack = action_tabs.sea_attack = true;
            }
        }


        // Bisheriges Actionfenster schließen, falls es eins gab
        // aktuelle Mausposition merken, da diese durch das Schließen verändert werden kann
        int mx = mc.x, my = mc.y;
        WindowManager::inst().Close(actionwindow);
        VideoDriverWrapper::inst().SetMousePos(mx, my);

        ShowActionWindow(action_tabs, cselx, csely, mc.x, mc.y, enable_military_buildings);

        selected_x = static_cast<unsigned short>(cselx);
        selected_y = static_cast<unsigned short>(csely);

    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author Divan
 */
bool dskGameInterface::Msg_LeftUp(const MouseCoords& mc)\
{
    // Stop Scrolling
    gwv->MouseUp();
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
bool dskGameInterface::Msg_MouseMove(const MouseCoords& mc)
{
    gwv->MouseMove(mc);
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
bool dskGameInterface::Msg_RightDown(const MouseCoords& mc)
{
    gwv->MouseDown(mc);
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
bool dskGameInterface::Msg_RightUp(const MouseCoords& mc)
{
    // Stop Scrolling
    gwv->MouseUp();
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Druck von Spezialtasten auswerten.
 *
 *  @author OLiver
 */
bool dskGameInterface::Msg_KeyDown(const KeyEvent& ke)
{
    switch(ke.kt)
    {
        default:
            break;
        case KT_RETURN: // Chatfenster öffnen
        {
            WindowManager::inst().Show(new iwChat);
        } return true;

        case KT_SPACE: // Bauqualitäten anzeigen
        {
            gwv->ShowBQ();
        } return true;

        case KT_LEFT: // Nach Links Scrollen
        {
            gwv->MoveToX(-30);
        } return true;

        case KT_RIGHT: // Nach Rechts Scrollen
        {
            gwv->MoveToX(30);
        } return true;

        case KT_UP: // Nach Oben Scrollen
        {
            gwv->MoveToY(-30);
        } return true;

        case KT_DOWN: // Nach Unten Scrollen
        {
            gwv->MoveToY(30);
        } return true;
        case KT_F2: // Spiel speichern
        {
            WindowManager::inst().Show(new iwSave);
        } return true;
        case KT_F3: // Koordinatenanzeige ein/aus vorläufig zu Debugzwecken
        {
            gwv->ShowCoordinates();
        } return true;
        case KT_F8: // Tastaturbelegung
        {
            WindowManager::inst().Show(new iwTextfile("keyboardlayout.txt", _("Keyboard layout")));
        } return true;
        case KT_F9: // Readme
        {
            WindowManager::inst().Show(new iwTextfile("readme.txt", _("Readme!")));
        } return true;
        case KT_F11: // Music player (midi files)
        {
            WindowManager::inst().Show(new iwMusicPlayer);
        } return true;
        case KT_F12: // Optionsfenster
        {
            WindowManager::inst().Show(new iwOptionsWindow(this));
        } return true;
    }

    switch(ke.c)
    {
        case '+': // Geschwindigkeit im Replay erhöhen
        {
            GAMECLIENT.IncreaseReplaySpeed();
        } return true;
        case '-': // Geschwindigkeit im Replay verringern
        {
            GAMECLIENT.DecreaseReplaySpeed();
        } return true;

        case '1':   case '2':   case '3': // Spieler umschalten
        case '4':   case '5':   case '6':
        case '7':   case '8':
        {
            if(GameClient::inst().IsReplayModeOn())
                GameClient::inst().ChangeReplayPlayer(ke.c - '1');
            else
            {
                GameClientPlayer* player = GameClient::inst().GetPlayer(ke.c - '1');
                if(player)
                {
                    if(player->ps == PS_KI && player->aiType == AI_DUMMY)
                        GAMECLIENT.AddGC(new gc::SwitchPlayer(ke.c - '1'));
                }
            }
        } return true;

        case 'b': // Zur lezten Position zurückspringen
        {
            gwv->MoveToLastPosition();
        } return true;
        case 'v':
        {
            unsigned singleplayer = 0, i = 0;
            while(i < GameClient::inst().GetPlayerCount() && singleplayer < 2)
            {
                if(GameClient::inst().GetPlayer(i)->ps == PS_OCCUPIED)singleplayer++;
                i++;
            }
            if(singleplayer < 2)GAMECLIENT.IncreaseSpeed();
        } return true;
        case 'c': // Gebäudenamen anzeigen
        {
            gwv->ShowNames();
        } return true;
        case 'd': // Replay: FoW an/ausschalten
        {
            // GameClient Bescheid sagen
            GameClient::inst().ToggleReplayFOW();
            // Sichtbarkeiten neu setzen auf der Map-Anzeige und der Minimap
            gwv->RecalcAllColors();
            minimap.UpdateAll();
        } return true;
        case 'h': // Zum HQ springen
        {
            GameClientPlayer* player = GAMECLIENT.GetLocalPlayer();
            // Prüfen, ob dieses überhaupt noch existiert
            if(player->hqx != 0xFFFF)
                gwv->MoveToMapObject(player->hqx, player->hqy);
        } return true;
        case 'i': // Show inventory
        {
            WindowManager::inst().Show(new iwInventory);
        } return true;
        case 'j': // GFs überspringen
        {
            unsigned singleplayer = 0, i = 0;
            while(i < GameClient::inst().GetPlayerCount() && singleplayer < 2)
            {
                if(GameClient::inst().GetPlayer(i)->ps == PS_OCCUPIED)singleplayer++;
                i++;
            }
            if(singleplayer < 2 || GAMECLIENT.IsReplayModeOn())
				WindowManager::inst().Show(new iwSkipGFs);
                
        } return true;
        case 'l': // Minimap anzeigen
        {
            WindowManager::inst().Show(new iwMinimap(&minimap, *gwv));
        } return true;
        case 'm': // Hauptauswahl
        {
            WindowManager::inst().Show(new iwMainMenu(gwv, this));
        } return true;
        case 'n': // Show Post window
        {
            WindowManager::inst().Show(new iwPostWindow(*gwv));
            UpdatePostIcon(GAMECLIENT.GetPostMessages().size(), false);
        } return true;
        case 'p': // Pause
        {
            if(GAMECLIENT.IsHost())
            {
                GAMESERVER.TogglePause();
                //if(GAMESERVER.TogglePause())
                //  GAMECLIENT.Command_Chat(_("pausing game"));
                //else
                //  GAMECLIENT.Command_Chat(_("continuing game"));
            }
            else if(GAMECLIENT.IsReplayModeOn())
            {
                GAMECLIENT.ToggleReplayPause();
            }
        } return true;
        case 'q': // Spiel verlassen
        {
            if(ke.alt)
                WindowManager::inst().Show(new iwEndgame);
        } return true;
        case 's': // Produktivität anzeigen
        {
            gwv->ShowProductivity();
        } return true;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskGameInterface::Run()
{
    unsigned water_percent;
    gwv->Draw(GAMECLIENT.GetPlayerID(), &water_percent, actionwindow ? true : false, selected_x, selected_y, road);

    // Evtl Meeresrauschen-Sounds abspieln
    SOUNDMANAGER.PlayOceanBrawling(water_percent);

    messenger.Draw();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskGameInterface::ActivateRoadMode(const RoadMode rm)
{
    // Im Replay und in der Pause keine Straßen bauen
    if(GameClient::inst().IsReplayModeOn() || GameClient::inst().IsPaused())
        return;

    road.mode = rm;
    if(rm != RM_DISABLED)
    {
        road.route.clear();
        road.start_x = road.point_x = selected_x;
        road.start_y = road.point_y = selected_y;
        unsigned short tmp_px, tmp_py, tmp_sx, tmp_sy;
        gwv->ConvertCoords(road.point_x, road.point_y, &tmp_px, &tmp_py);
        gwv->ConvertCoords(road.start_x, road.start_y, &tmp_sx, &tmp_sy);
        road.point_x = tmp_px;
        road.point_y = tmp_py;
        road.start_x = tmp_sx;
        road.start_y = tmp_sy;
    }
    else
    {
        gwv->RemoveVisualRoad(road.start_x, road.start_y, road.route);
        for(unsigned i = 0; i < road.route.size(); ++i)
        {
            gwv->SetPointVirtualRoad(road.start_x, road.start_y, road.route[i], 0);
            int tx2 = road.start_x, ty2 = road.start_y;
            road.start_x = gwv->GetXA(tx2, ty2, road.route[i]);
            road.start_y = gwv->GetYA(tx2, ty2, road.route[i]);
        }
    }

}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
bool dskGameInterface::BuildRoadPart(int& cselx, int& csely, bool end)
{
    std::vector<unsigned char> new_route;
    bool path_found = gwv->FindRoadPath(road.point_x, road.point_y, cselx, csely, new_route, road.mode == RM_BOAT);

    // Weg gefunden?
    if(!path_found)
        return false;

    // Test on water way length
    if(road.mode == RM_BOAT)
    {
        unsigned char waterway_lengthes[] = {3, 5, 9, 13, 21, 0}; // these are written into GameWorldViewer.cpp, too
        unsigned char index = GameClient::inst().GetGGS().getSelection(ADDON_MAX_WATERWAY_LENGTH);

        assert(index <= sizeof(waterway_lengthes) - 1);
        const unsigned char max_length = waterway_lengthes[index];

        unsigned short length = road.route.size() + new_route.size();

        // max_length == 0 heißt beliebig lang, ansonsten
        // Weg zurechtstutzen.
        if (max_length > 0)
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
        gwv->SetPointVirtualRoad(road.point_x, road.point_y, new_route[i], (road.mode == RM_BOAT) ? 3 : 1);
        gwv->GetPointA(road.point_x, road.point_y, new_route[i]);
        gwv->CalcRoad(road.point_x, road.point_y, GAMECLIENT.GetPlayerID());
    }
    // Zielpunkt updaten (für Wasserweg)
    cselx = road.point_x;
    csely = road.point_y;

    road.route.insert(road.route.end(), new_route.begin(), new_route.end());

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
unsigned dskGameInterface::TestBuiltRoad(const int x, const int y)
{
    int x2 = road.start_x, y2 = road.start_y;
    for(unsigned i = 0; i < road.route.size(); ++i)
    {
        if(x2 == x && y2 == y)
            return i + 1;

        int tx2 = x2, ty2 = y2;
        x2 = gwv->GetXA(tx2, ty2, road.route[i]);
        y2 = gwv->GetYA(tx2, ty2, road.route[i]);
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskGameInterface::ShowRoadWindow(int mouse_x, int mouse_y)
{
    if(gwv->CalcBQ(road.point_x, road.point_y, GAMECLIENT.GetPlayerID(), 1))
        WindowManager::inst().Show(roadwindow = new iwRoadWindow(this, 1, mouse_x, mouse_y), true);
    else
        WindowManager::inst().Show(roadwindow = new iwRoadWindow(this, 0, mouse_x, mouse_y), true);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskGameInterface::ShowActionWindow(const iwAction::Tabs& action_tabs, int cselx, int csely, int mouse_x, int mouse_y, const bool enable_military_buildings)
{
    unsigned int params = 0;

    // Sind wir am Wasser?
    if(action_tabs.setflag)
    {
        for(unsigned char x = 0; x < 6; ++x)
        {
            if(gwv->GetTerrainAround(cselx, csely, x) == 14)
                params = iwAction::AWFT_WATERFLAG;
        }
    }

    // Wenn es einen Flaggen-Tab gibt, dann den Flaggentyp herausfinden und die Art des Fensters entsprechende setzen
    if(action_tabs.flag)
    {

        if(gwv->GetNO(gwv->GetXA(cselx, csely, 1), gwv->GetYA(cselx, csely, 1))->GetGOT() == GOT_NOB_HQ)
            params = iwAction::AWFT_HQ;
        else if(gwv->GetNO(cselx, csely)->GetType() == NOP_FLAG)
        {
            if(gwv->GetSpecObj<noFlag>(cselx, csely)->GetFlagType() == FT_WATER)
                params = iwAction::AWFT_WATERFLAG;
        }
    }

    // Angriffstab muss wissen, wieviel Soldaten maximal entsendet werden können
    if(action_tabs.attack)
    {
        if(GAMECLIENT.GetLocalPlayer()->IsPlayerAttackable(gwv->GetSpecObj<noBuilding>(cselx, csely)->GetPlayer()))
            params = gwv->GetAvailableSoldiersForAttack(GAMECLIENT.GetPlayerID(), cselx, csely);
    }

    WindowManager::inst().Show((actionwindow = new iwAction(this, gwv, action_tabs, cselx, csely, mouse_x, mouse_y, params, enable_military_buildings)), true);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskGameInterface::CommandBuildRoad()
{
    GameClient::inst().AddGC(new gc::BuildRoad(road.start_x, road.start_y, road.mode == RM_BOAT, road.route));
    road.mode = RM_DISABLED;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskGameInterface::GI_FlagDestroyed(const unsigned short x, const unsigned short y)
{
    // Im Wegbaumodus und haben wir von hier eine Flagge gebaut?
    if(road.mode != RM_DISABLED && road.start_x == x && road.start_y == y)
    {
        // Wegbau abbrechen
        ActivateRoadMode(RM_DISABLED);
    }

    // Evtl Actionfenster schließen, da sich das ja auch auf diese Flagge bezieht
    if(actionwindow)
    {
        if(actionwindow->GetSelectedX() == x && actionwindow->GetSelectedY() == y)
            WindowManager::inst().Close(actionwindow);
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskGameInterface::ActionWindowClosed()
{
    actionwindow = NULL;
    gwv->DontScroll();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskGameInterface::RoadWindowClosed()
{
    roadwindow = NULL;
    gwv->DontScroll();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskGameInterface::CI_PlayerLeft(const unsigned player_id)
{
    // Info-Meldung ausgeben
    char text[256];
    snprintf(text, sizeof(text), _("Player '%s' left the game!"), GameClient::inst().GetPlayer(player_id)->name.c_str());
    messenger.AddMessage("", 0, CD_SYSTEM, text, COLOR_RED);
    // Im Spiel anzeigen, dass die KI das Spiel betreten hat
    snprintf(text, sizeof(text), _("Player '%s' joined the game!"), "KI");
    messenger.AddMessage("", 0, CD_SYSTEM, text, COLOR_GREEN);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskGameInterface::CI_GGSChanged(const GlobalGameSettings& ggs)
{
    // TODO: print what has changed
    char text[256];
    snprintf(text, sizeof(text), _("Note: Game settings changed by the server%s"), "");
    messenger.AddMessage("", 0, CD_SYSTEM, text);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskGameInterface::CI_Chat(const unsigned player_id, const ChatDestination cd, const std::string& msg)
{
    char from[256];
    snprintf(from, sizeof(from), _("<%s> "), GameClient::inst().GetPlayer(player_id)->name.c_str());
    messenger.AddMessage(from,
                         COLORS[GameClient::inst().GetPlayer(player_id)->color], cd, msg);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskGameInterface::CI_Async(const std::string& checksums_list)
{
    messenger.AddMessage("", 0, CD_SYSTEM, _("The Game is not in sync. Checksums of some players don't match."), COLOR_RED);
    messenger.AddMessage("", 0, CD_SYSTEM, checksums_list.c_str(), COLOR_YELLOW);
    messenger.AddMessage("", 0, CD_SYSTEM, _("A auto-savegame is created..."), COLOR_RED);

}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskGameInterface::CI_ReplayAsync(const std::string& msg)
{
    messenger.AddMessage("", 0, CD_SYSTEM, msg, COLOR_RED);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskGameInterface::CI_ReplayEndReached(const std::string& msg)
{
    messenger.AddMessage("", 0, CD_SYSTEM, msg, COLOR_BLUE);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskGameInterface::CI_GamePaused()
{
    char from[256];
    snprintf(from, sizeof(from), _("<%s> "), _("SYSTEM"));
    messenger.AddMessage(from, COLOR_GREY, CD_SYSTEM, _("Game was paused."));

    /// Straßenbau ggf. abbrechen, wenn aktiviert
    if(road.mode != RM_DISABLED)
    {
        road.mode = RM_DISABLED;
        // Fenster schließen
        if(roadwindow)
        {
            //WindowManager::inst().Close(roadwindow);
            roadwindow->Close();
            roadwindow = 0;
        }
        // Weg zurückbauen
        this->DemolishRoad(1);

    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskGameInterface::CI_GameResumed()
{
    char from[256];
    snprintf(from, sizeof(from), _("<%s> "), _("SYSTEM"));
    messenger.AddMessage(from, COLOR_GREY, CD_SYSTEM, _("Game was resumed."));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskGameInterface::CI_Error(const ClientError ce)
{
    switch(ce)
    {
        default:
            break;

        case CE_CONNECTIONLOST:
        {
            messenger.AddMessage("", 0, CD_SYSTEM, _("Lost connection to server!"), COLOR_RED);
        } break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Status: Verbindung verloren.
 *
 *  @author FloSoft
 */
void dskGameInterface::LC_Status_ConnectionLost()
{
    messenger.AddMessage("", 0, CD_SYSTEM, _("Lost connection to lobby!"), COLOR_RED);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  (Lobby-)Status: Benutzerdefinierter Fehler
 *
 *  @author FloSoft
 */
void dskGameInterface::LC_Status_Error(const std::string& error)
{
    messenger.AddMessage("", 0, CD_SYSTEM, error, COLOR_RED);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskGameInterface::CI_PlayersSwapped(const unsigned player1, const unsigned player2)
{
    // Meldung anzeigen
    char text[256];
    snprintf(text, sizeof(text), _("Player '%s' switched to player '%s'"), GameClient::inst().GetPlayer(player1)->name.c_str()
             , GameClient::inst().GetPlayer(player2)->name.c_str());
    messenger.AddMessage("", 0, CD_SYSTEM, text, COLOR_YELLOW);


    // Sichtbarkeiten und Minimap neu berechnen, wenn wir ein von den beiden Spielern sind
    if(player1 == GameClient::inst().GetPlayerID() || player2 == GameClient::inst().GetPlayerID())
    {
        minimap.UpdateAll();
        gwv->RecalcAllColors();
    }

}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Wenn ein Spieler verloren hat
 *
 *  @author OLiver
 */
void dskGameInterface::GI_PlayerDefeated(const unsigned player_id)
{
    char text[256];
    snprintf(text, sizeof(text), _("Player '%s' was defeated!"), GameClient::inst().GetPlayer(player_id)->name.c_str());
    messenger.AddMessage("", 0, CD_SYSTEM, text, COLOR_ORANGE);

    /// Lokaler Spieler?
    if(player_id == GameClient::inst().GetPlayerID())
    {
        /// Sichtbarkeiten neu berechnen
        gwv->RecalcAllColors();
        // Minimap updaten
        minimap.UpdateAll();
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskGameInterface::GI_UpdateMinimap(const MapCoord x, const MapCoord y)
{
    // Minimap Bescheid sagen
    minimap.UpdateNode(x, y);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Bündnisvertrag wurde abgeschlossen oder abgebrochen --> Minimap updaten
 *
 *  @author OLiver
 */
void dskGameInterface::GI_TreatyOfAllianceChanged()
{
    // Nur wenn Team-Sicht aktiviert ist, können sihc die Sichtbarkeiten auch ändern
    if(GameClient::inst().GetGGS().team_view)
    {
        /// Sichtbarkeiten neu berechnen
        gwv->RecalcAllColors();
        // Minimap updaten
        minimap.UpdateAll();
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Baut Weg zurück von Ende bis zu start_id
 *
 *  @author OLiver
 */
void dskGameInterface::DemolishRoad(const unsigned start_id)
{
    for(unsigned i = road.route.size(); i >= start_id; --i)
    {
        MapCoord tx = road.point_x, ty = road.point_y;
        road.point_x = gwv->GetXA(tx, ty, (road.route[i - 1] + 3) % 6);
        road.point_y = gwv->GetYA(tx, ty, (road.route[i - 1] + 3) % 6);
        gwv->SetPointVirtualRoad(road.point_x, road.point_y, road.route[i - 1], 0);
        gwv->CalcRoad(tx, ty, GAMECLIENT.GetPlayerID());
    }

    road.route.resize(start_id - 1);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Updatet das Post-Icon mit der Nachrichtenanzahl und der Taube
 *
 *  @author OLiver
 */
void dskGameInterface::UpdatePostIcon(const unsigned postmessages_count, bool showPigeon)
{
    // Taube setzen oder nicht (Post)
    if (postmessages_count == 0 || !showPigeon)
        GetCtrl<ctrlImageButton>(3)->SetImage(LOADER.GetImageN("io", 62));
    else
        GetCtrl<ctrlImageButton>(3)->SetImage(LOADER.GetImageN("io", 59));

    // und Anzahl der Postnachrichten aktualisieren
    if(postmessages_count > 0)
    {
        std::stringstream ss;
        ss << postmessages_count;
        GetCtrl<ctrlText>(4)->SetText(ss.str());
    }
    else
        GetCtrl<ctrlText>(4)->SetText("");

}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Neue Post-Nachricht eingetroffen
 *
 *  @author OLiver
 */
void dskGameInterface::CI_NewPostMessage(const unsigned postmessages_count)
{
    UpdatePostIcon(postmessages_count, true);

    // Tauben-Sound abspielen
    LOADER.GetSoundN("sound", 114)->Play(255, false);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Es wurde eine Postnachricht vom Spieler gelöscht
 *
 *  @author OLiver
 */
void dskGameInterface::CI_PostMessageDeleted(const unsigned postmessages_count)
{
    UpdatePostIcon(postmessages_count, false);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Ein Spieler hat das Spiel gewonnen.
 *
 *  @author OLiver
 */
void dskGameInterface::GI_Winner(const unsigned player_id)
{
    char text[256];
    snprintf(text, sizeof(text), _("Player '%s' is the winner!"), GameClient::inst().GetPlayer(player_id)->name.c_str());
    messenger.AddMessage("", 0, CD_SYSTEM, text, COLOR_ORANGE);
}
///////////////////////////////////////////////////////////////////////////////
/**
 *  Ein Team hat das Spiel gewonnen.
 *
 *  @author poc
 */
void dskGameInterface::GI_TeamWinner(const unsigned player_id)
{
    unsigned winnercount = 0;
    char winners[5];
    for(unsigned i = 0; i < GameClient::inst().GetPlayerCount() && winnercount < 5; i++)
    {
        winners[winnercount] = i;
        winnercount += player_id & (1 << i) ? 1 : 0;
    }
    char text[256];
    switch (winnercount)
    {
        case 2:
            snprintf(text, sizeof(text), _("Team victory! '%s' and '%s' are the winners!"), GameClient::inst().GetPlayer(winners[0])->name.c_str(), GameClient::inst().GetPlayer(winners[1])->name.c_str());
            break;
        case 3:
            snprintf(text, sizeof(text), _("Team victory! '%s' and '%s' and '%s' are the winners!"), GameClient::inst().GetPlayer(winners[0])->name.c_str(), GameClient::inst().GetPlayer(winners[1])->name.c_str(), GameClient::inst().GetPlayer(winners[2])->name.c_str());
            break;
        case 4:
            snprintf(text, sizeof(text), _("Team victory! '%s' and '%s' and '%s' and '%s' are the winners!"), GameClient::inst().GetPlayer(winners[0])->name.c_str(), GameClient::inst().GetPlayer(winners[1])->name.c_str(), GameClient::inst().GetPlayer(winners[2])->name.c_str(), GameClient::inst().GetPlayer(winners[3])->name.c_str());
            break;
        default:
            snprintf(text, sizeof(text), "%s", _("Team victory!"));
            break;
    }
    messenger.AddMessage("", 0, CD_SYSTEM, text, COLOR_ORANGE);
}

