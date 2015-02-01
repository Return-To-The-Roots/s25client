// $Id: iwMainMenu.cpp 9595 2015-02-01 09:40:54Z marcus $
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
#include "iwMainMenu.h"

#include "Loader.h"
#include "WindowManager.h"


#include "iwDistribution.h"
#include "iwTransport.h"
#include "iwTools.h"
#include "iwMilitary.h"
#include "iwBuildOrder.h"
#include "iwOptionsWindow.h"
#include "iwInventory.h"
#include "iwBuildings.h"
#include "iwBuildingProductivities.h"
#include "iwStatistics.h"
#include "iwSettings.h"
#include "iwDiplomacy.h"
#include "iwShip.h"
#include "iwAIDebug.h"
#include "iwMerchandiseStatistics.h"

#include "GameClient.h"
#include "GameClientPlayer.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p iwMainMenu.
 *
 *  @author OLiver
*/
iwMainMenu::iwMainMenu(GameWorldViewer* const gwv, dskGameInterface* const gi)
    : IngameWindow(CGI_MAINSELECTION, 0xFFFF, 0xFFFF, 190, 286, _("Main selection"), LOADER.GetImageN("io", 5)),
      gwv(gwv), gi(gi)
{
    // Verteilung
    AddImageButton( 0,  12,  22,  53, 44, TC_GREY, LOADER.GetImageN("io", 134), _("Distribution of goods"));
    // Transport
    AddImageButton( 1,  68,  22,  53, 44, TC_GREY, LOADER.GetImageN("io", 198), _("Transport"));
    // Werkzeugproduktion
    AddImageButton( 2, 124,  22,  53, 44, TC_GREY, LOADER.GetImageN("io", 137), _("Tools"));

    // Statistiken
    AddImageButton( 3,  12,  70,  39, 44, TC_GREY, LOADER.GetImageN("io", 166), _("General statistics"));
    AddImageButton( 4,  54,  70,  39, 44, TC_GREY, LOADER.GetImageN("io", 135), _("Merchandise statistics"));
    AddImageButton( 5,  96,  70,  39, 44, TC_GREY, LOADER.GetImageN("io", 132), _("Buildings"));

    // Inventur
    AddImageButton( 6, 138,  70,  39, 44, TC_GREY, LOADER.GetImageN("io", 214), _("Stock"));

    // Gebäude
    AddImageButton( 7,  12, 118,  53, 44, TC_GREY, LOADER.GetImageN("io", 136), _("Productivity"));
    // Militär
    AddImageButton( 8,  68, 118,  53, 44, TC_GREY, LOADER.GetImageN("io", 133), _("Military"));
    // Schiffe
    AddImageButton( 9, 124, 118,  53, 44, TC_GREY, LOADER.GetImageN("io", 175), _("Ship register"));

    // Baureihenfolge
    if(GameClient::inst().GetGGS().isEnabled(ADDON_CUSTOM_BUILD_SEQUENCE))
        AddImageButton( 10,  12, 166,  53, 44, TC_GREY, LOADER.GetImageN("io", 24), _("Building sequence"));

    // Diplomatie (todo: besseres Bild suchen)
    AddImageButton( 11,  68, 166,  53, 44, TC_GREY, LOADER.GetImageN("io", 190), _("Diplomacy"));

    // AI-Debug
    if(GameClient::inst().IsHost() && GameClient::inst().GetGGS().isEnabled(ADDON_AI_DEBUG_WINDOW))
        AddImageButton( 13,  80, 210,  20, 20, TC_GREY, NULL, _("AI Debug Window"));

    // Optionen
    AddImageButton(30,  12, 231, 165, 32, TC_GREY, LOADER.GetImageN("io",  37), _("Options"));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Button-Click-Handler.
 *
 *  @author OLiver
*/
void iwMainMenu::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 0: // Verteilung
        {
            WindowManager::inst().Show(new iwDistribution);
        } break;
        case 1: // Transport
        {
            WindowManager::inst().Show(new iwTransport);
        } break;
        case 2: // Werkzeugproduktion
        {
            WindowManager::inst().Show(new iwTools);
        } break;
        case 3: // Statistik
        {
            WindowManager::inst().Show(new iwStatistics);
        } break;
        case 4: // Warenstatistik
        {
            WindowManager::inst().Show(new iwMerchandiseStatistics);
        } break;
        case 5: // Gebäudestatistik
        {
            WindowManager::inst().Show(new iwBuildings(gwv,gi));
        } break;
        case 6: // Inventur
        {
            WindowManager::inst().Show(new iwInventory);
        } break;
        case 7: // Produktivitäten
        {
            WindowManager::inst().Show(new iwBuildingProductivities);
        } break;
        case 8: // Militär
        {
            WindowManager::inst().Show(new iwMilitary);
        } break;
        case 9: // Schiffe
        {
            WindowManager::inst().Show(new iwShip(gwv, gi, GAMECLIENT.GetLocalPlayer()->GetShipByID(0)));
        } break;
        case 10: // Baureihenfolge
        {
            WindowManager::inst().Show(new iwBuildOrder);
        } break;
        case 11: // Diplomatie
        {
            WindowManager::inst().Show(new iwDiplomacy);
        } break;
        case 13: // AI Debug
        {
            if(GAMECLIENT.IsHost())
                WindowManager::inst().Show(new iwAIDebug(gwv));
        } break;
        case 30: // Optionen
        {
            WindowManager::inst().Show(new iwOptionsWindow(this->gi));
        } break;
    }
}
