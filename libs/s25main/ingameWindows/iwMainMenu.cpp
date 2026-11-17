// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
#include "iwEconomicProgress.h"
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
                   LOADER.getImageN("io", 5)),
      gwv(gwv), gcFactory(gcFactory)
{
    // Verteilung
    addImageButton(0, DrawPoint(12, 22), Extent(53, 44), TextureColor::Grey, LOADER.getImageN("io", 134),
                   _("Distribution of goods"));
    // Transport
    addImageButton(1, DrawPoint(68, 22), Extent(53, 44), TextureColor::Grey, LOADER.getImageN("io", 198),
                   _("Transport"));
    // Werkzeugproduktion
    addImageButton(2, DrawPoint(124, 22), Extent(53, 44), TextureColor::Grey, LOADER.getImageN("io", 137), _("Tools"));

    // Statistiken
    addImageButton(3, DrawPoint(12, 70), Extent(39, 44), TextureColor::Grey, LOADER.getImageN("io", 166),
                   _("General statistics"));
    addImageButton(4, DrawPoint(54, 70), Extent(39, 44), TextureColor::Grey, LOADER.getImageN("io", 135),
                   _("Merchandise statistics"));
    addImageButton(5, DrawPoint(96, 70), Extent(39, 44), TextureColor::Grey, LOADER.getImageN("io", 132),
                   _("Buildings"));

    // Inventur
    addImageButton(6, DrawPoint(138, 70), Extent(39, 44), TextureColor::Grey, LOADER.getImageN("io", 214), _("Stock"));

    // Gebäude
    addImageButton(7, DrawPoint(12, 118), Extent(53, 44), TextureColor::Grey, LOADER.getImageN("io", 136),
                   _("Productivity"));
    // Militär
    addImageButton(8, DrawPoint(68, 118), Extent(53, 44), TextureColor::Grey, LOADER.getImageN("io", 133),
                   _("Military"));
    // Schiffe
    addImageButton(9, DrawPoint(124, 118), Extent(53, 44), TextureColor::Grey, LOADER.getImageN("io", 175),
                   _("Ship register"));

    // Baureihenfolge
    if(gwv.getWorld().getGGS().isEnabled(AddonId::CUSTOM_BUILD_SEQUENCE))
        addImageButton(10, DrawPoint(12, 166), Extent(53, 44), TextureColor::Grey, LOADER.getImageN("io", 24),
                       _("Building sequence"));

    // Diplomatie (todo: besseres Bild suchen)
    addImageButton(11, DrawPoint(68, 166), Extent(53, 44), TextureColor::Grey, LOADER.getImageN("io", 190),
                   _("Diplomacy"));

    if(gwv.getWorld().getEconHandler())
    {
        // Economy Mode
        addImageButton(12, DrawPoint(124, 166), Extent(53, 44), TextureColor::Grey, LOADER.getImageN("io", 196),
                       _("Economic Progress"));
    }

// AI-Debug
#ifdef NDEBUG
    bool enableAIDebug = gwv.getWorld().getGGS().isEnabled(AddonId::AI_DEBUG_WINDOW);
#else
    bool enableAIDebug = true;
#endif
    if(gwv.getViewer().getPlayer().isHost && enableAIDebug)
    {
        auto* bt = static_cast<ctrlTextButton*>(addTextButton(13, DrawPoint(80, 210), Extent(0, 22), TextureColor::Grey,
                                                              _("AI"), NormalFont, _("AI Debug Window")));
        bt->resizeForMaxChars(bt->getText().size());
    }

    // Optionen
    addImageButton(30, DrawPoint(12, 231), Extent(165, 32), TextureColor::Grey, LOADER.getImageN("io", 37),
                   _("Options"));
}

/**
 *  Button-Click-Handler.
 */
void iwMainMenu::msgButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 0: // Verteilung
        {
            WINDOWMANAGER.toggleWindow(std::make_unique<iwDistribution>(gwv.getViewer(), gcFactory));
        }
        break;
        case 1: // Transport
        {
            WINDOWMANAGER.toggleWindow(std::make_unique<iwTransport>(gwv.getViewer(), gcFactory));
        }
        break;
        case 2: // Werkzeugproduktion
        {
            WINDOWMANAGER.toggleWindow(std::make_unique<iwTools>(gwv.getViewer(), gcFactory));
        }
        break;
        case 3: // Statistik
        {
            WINDOWMANAGER.toggleWindow(std::make_unique<iwStatistics>(gwv.getViewer()));
        }
        break;
        case 4: // Warenstatistik
        {
            WINDOWMANAGER.toggleWindow(std::make_unique<iwMerchandiseStatistics>(gwv.getViewer().getPlayer()));
        }
        break;
        case 5: // Gebäudestatistik
        {
            WINDOWMANAGER.toggleWindow(std::make_unique<iwBuildings>(gwv, gcFactory));
        }
        break;
        case 6: // Inventur
        {
            WINDOWMANAGER.toggleWindow(std::make_unique<iwInventory>(gwv.getViewer().getPlayer()));
        }
        break;
        case 7: // Produktivitäten
        {
            WINDOWMANAGER.toggleWindow(std::make_unique<iwBuildingProductivities>(gwv.getViewer().getPlayer()));
        }
        break;
        case 8: // Militär
        {
            WINDOWMANAGER.toggleWindow(std::make_unique<iwMilitary>(gwv.getViewer(), gcFactory));
        }
        break;
        case 9: // Schiffe
        {
            WINDOWMANAGER.toggleWindow(std::make_unique<iwShip>(
              gwv, gcFactory, gwv.getViewer().getPlayer().getShipByID(0), IngameWindow::posCenter));
        }
        break;
        case 10: // Baureihenfolge
        {
            WINDOWMANAGER.toggleWindow(std::make_unique<iwBuildOrder>(gwv.getViewer()));
        }
        break;
        case 11: // Diplomatie
        {
            WINDOWMANAGER.toggleWindow(std::make_unique<iwDiplomacy>(gwv.getViewer(), gcFactory));
        }
        break;
        case 12: // Wirtschaftsmodusfortschritt
        {
            WINDOWMANAGER.toggleWindow(std::make_unique<iwEconomicProgress>(gwv.getViewer()));
        }
        break;
        case 13: // AI Debug
        {
            if(auto* wnd = WINDOWMANAGER.findNonModalWindow(CGI_AI_DEBUG))
                wnd->close();
            else if(gwv.getViewer().getPlayer().isHost)
            {
                std::vector<const AIPlayer*> ais;
                for(unsigned i = 0; i < gwv.getViewer().getNumPlayers(); ++i)
                {
                    const AIPlayer* ai = GAMECLIENT.getAIPlayer(i);
                    if(ai)
                        ais.push_back(ai);
                }
                WINDOWMANAGER.show(std::make_unique<iwAIDebug>(gwv, ais));
            }
        }
        break;
        case 30: // Optionen
        {
            WINDOWMANAGER.toggleWindow(std::make_unique<iwOptionsWindow>(gwv.getSoundMgr()));
        }
        break;
    }
}
