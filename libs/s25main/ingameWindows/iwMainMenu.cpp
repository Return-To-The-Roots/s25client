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

#include "iwMainMenu.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "WindowManager.h"
#include "addons/const_addons.h"
#include "controls/ctrlTextButton.h"
#include "iwAIDebug.h"
#include "iwBuildOrder.h"
#include "iwBuildingProductivities.h"
#include "iwBuildings.h"
#include "iwDiplomacy.h"
#include "iwDistribution.h"
#include "iwInventory.h"
#include "iwMerchandiseStatistics.h"
#include "iwMilitary.h"
#include "iwOptionsWindow.h"
#include "iwShip.h"
#include "iwStatistics.h"
#include "iwTools.h"
#include "iwTransport.h"
#include "network/GameClient.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldView.h"
#include "world/GameWorldViewer.h"
#include "gameData/const_gui_ids.h"

iwMainMenu::iwMainMenu(GameWorldView& gwv, GameCommandFactory& gcFactory)
    : IngameWindow(CGI_MAINSELECTION, IngameWindow::posLastOrCenter, Extent(190, 286), _("Main selection"),
                   LOADER.GetImageN("io", 5)),
      gwv(gwv), gcFactory(gcFactory)
{
    // Verteilung
    AddImageButton(0, DrawPoint(12, 22), Extent(53, 44), TC_GREY, LOADER.GetImageN("io", 134),
                   _("Distribution of goods"));
    // Transport
    AddImageButton(1, DrawPoint(68, 22), Extent(53, 44), TC_GREY, LOADER.GetImageN("io", 198), _("Transport"));
    // Werkzeugproduktion
    AddImageButton(2, DrawPoint(124, 22), Extent(53, 44), TC_GREY, LOADER.GetImageN("io", 137), _("Tools"));

    // Statistiken
    AddImageButton(3, DrawPoint(12, 70), Extent(39, 44), TC_GREY, LOADER.GetImageN("io", 166), _("General statistics"));
    AddImageButton(4, DrawPoint(54, 70), Extent(39, 44), TC_GREY, LOADER.GetImageN("io", 135),
                   _("Merchandise statistics"));
    AddImageButton(5, DrawPoint(96, 70), Extent(39, 44), TC_GREY, LOADER.GetImageN("io", 132), _("Buildings"));

    // Inventur
    AddImageButton(6, DrawPoint(138, 70), Extent(39, 44), TC_GREY, LOADER.GetImageN("io", 214), _("Stock"));

    // Gebäude
    AddImageButton(7, DrawPoint(12, 118), Extent(53, 44), TC_GREY, LOADER.GetImageN("io", 136), _("Productivity"));
    // Militär
    AddImageButton(8, DrawPoint(68, 118), Extent(53, 44), TC_GREY, LOADER.GetImageN("io", 133), _("Military"));
    // Schiffe
    AddImageButton(9, DrawPoint(124, 118), Extent(53, 44), TC_GREY, LOADER.GetImageN("io", 175), _("Ship register"));

    // Baureihenfolge
    if(gwv.GetWorld().GetGGS().isEnabled(AddonId::CUSTOM_BUILD_SEQUENCE))
        AddImageButton(10, DrawPoint(12, 166), Extent(53, 44), TC_GREY, LOADER.GetImageN("io", 24),
                       _("Building sequence"));

    // Diplomatie (todo: besseres Bild suchen)
    AddImageButton(11, DrawPoint(68, 166), Extent(53, 44), TC_GREY, LOADER.GetImageN("io", 190), _("Diplomacy"));

// AI-Debug
#ifdef NDEBUG
    bool enableAIDebug = gwv.GetWorld().GetGGS().isEnabled(AddonId::AI_DEBUG_WINDOW);
#else
    bool enableAIDebug = true;
#endif
    if(gwv.GetViewer().GetPlayer().isHost && enableAIDebug)
    {
        auto* bt = static_cast<ctrlTextButton*>(
          AddTextButton(13, DrawPoint(80, 210), Extent(0, 22), TC_GREY, _("AI"), NormalFont, _("AI Debug Window")));
        bt->ResizeForMaxChars(bt->GetText().size());
    }

    // Optionen
    AddImageButton(30, DrawPoint(12, 231), Extent(165, 32), TC_GREY, LOADER.GetImageN("io", 37), _("Options"));
}

/**
 *  Button-Click-Handler.
 */
void iwMainMenu::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 0: // Verteilung
        {
            WINDOWMANAGER.ToggleWindow(std::make_unique<iwDistribution>(gwv.GetViewer(), gcFactory));
        }
        break;
        case 1: // Transport
        {
            WINDOWMANAGER.ToggleWindow(std::make_unique<iwTransport>(gwv.GetViewer(), gcFactory));
        }
        break;
        case 2: // Werkzeugproduktion
        {
            WINDOWMANAGER.ToggleWindow(std::make_unique<iwTools>(gwv.GetViewer(), gcFactory));
        }
        break;
        case 3: // Statistik
        {
            WINDOWMANAGER.ToggleWindow(std::make_unique<iwStatistics>(gwv.GetViewer()));
        }
        break;
        case 4: // Warenstatistik
        {
            WINDOWMANAGER.ToggleWindow(std::make_unique<iwMerchandiseStatistics>(gwv.GetViewer().GetPlayer()));
        }
        break;
        case 5: // Gebäudestatistik
        {
            WINDOWMANAGER.ToggleWindow(std::make_unique<iwBuildings>(gwv, gcFactory));
        }
        break;
        case 6: // Inventur
        {
            WINDOWMANAGER.ToggleWindow(std::make_unique<iwInventory>(gwv.GetViewer().GetPlayer()));
        }
        break;
        case 7: // Produktivitäten
        {
            WINDOWMANAGER.ToggleWindow(std::make_unique<iwBuildingProductivities>(gwv.GetViewer().GetPlayer()));
        }
        break;
        case 8: // Militär
        {
            WINDOWMANAGER.ToggleWindow(std::make_unique<iwMilitary>(gwv.GetViewer(), gcFactory));
        }
        break;
        case 9: // Schiffe
        {
            WINDOWMANAGER.ToggleWindow(std::make_unique<iwShip>(
              gwv, gcFactory, gwv.GetViewer().GetPlayer().GetShipByID(0), IngameWindow::posCenter));
        }
        break;
        case 10: // Baureihenfolge
        {
            WINDOWMANAGER.ToggleWindow(std::make_unique<iwBuildOrder>(gwv.GetViewer()));
        }
        break;
        case 11: // Diplomatie
        {
            WINDOWMANAGER.ToggleWindow(std::make_unique<iwDiplomacy>(gwv.GetViewer(), gcFactory));
        }
        break;
        case 13: // AI Debug
        {
            if(auto* wnd = WINDOWMANAGER.FindNonModalWindow(CGI_AI_DEBUG))
                wnd->Close();
            else if(gwv.GetViewer().GetPlayer().isHost)
            {
                std::vector<const AIPlayer*> ais;
                for(unsigned i = 0; i < gwv.GetViewer().GetNumPlayers(); ++i)
                {
                    const AIPlayer* ai = GAMECLIENT.GetAIPlayer(i);
                    if(ai)
                        ais.push_back(ai);
                }
                WINDOWMANAGER.Show(std::make_unique<iwAIDebug>(gwv, ais));
            }
        }
        break;
        case 30: // Optionen
        {
            WINDOWMANAGER.ToggleWindow(std::make_unique<iwOptionsWindow>());
        }
        break;
    }
}
