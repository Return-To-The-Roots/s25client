// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#pragma once

#include "iwAIDebug.h"
#include "iwAction.h"
#include "iwBaseWarehouse.h"
#include "iwBuildOrder.h"
#include "iwBuilding.h"
#include "iwBuildingProductivities.h"
#include "iwBuildingSite.h"
#include "iwBuildings.h"
#include "iwChat.h"
#include "iwDiplomacy.h"
#include "iwDistribution.h"
#include "iwEconomicProgress.h"
#include "iwEndgame.h"
#include "iwHQ.h"
#include "iwHarborBuilding.h"
#include "iwInventory.h"
#include "iwMainMenu.h"
#include "iwMapDebug.h"
#include "iwMerchandiseStatistics.h"
#include "iwMilitary.h"
#include "iwMilitaryBuilding.h"
#include "iwMinimap.h"
#include "iwMusicPlayer.h"
#include "iwOptionsWindow.h"
#include "iwPostWindow.h"
#include "iwRoadWindow.h"
#include "iwSave.h"
#include "iwShip.h"
#include "iwSkipGFs.h"
#include "iwStatistics.h"
#include "iwTextfile.h"
#include "iwTools.h"
#include "iwTrade.h"
#include "iwTransport.h"
#include "iwVictory.h"
#include "gameData/const_gui_ids.h"
#include <map>
#include <string>

struct PersistentWindowData
{
    std::string name;
    std::string optionName;

    PersistentWindowData(std::string name, std::string optionName = "")
        : name(std::move(name)), optionName(std::move(optionName))
    {}
};

const std::map<GUI_ID, PersistentWindowData> persistentWindows = {
  {CGI_CHAT, {"wnd_chat"}},
  {CGI_POSTOFFICE, {"wnd_postoffice"}},
  {CGI_DISTRIBUTION, {"wnd_distribution"}},
  {CGI_BUILDORDER, {"wnd_buildorder"}},
  {CGI_TRANSPORT, {"wnd_transport"}},
  {CGI_MILITARY, {"wnd_military"}},
  {CGI_TOOLS, {"wnd_tools"}},
  {CGI_INVENTORY, {"wnd_inventory"}},
  {CGI_MINIMAP, {"wnd_minimap", "is_extended"}},
  {CGI_BUILDINGS, {"wnd_buildings"}},
  {CGI_BUILDINGSPRODUCTIVITY, {"wnd_buildingsproductivity"}},
  {CGI_MUSICPLAYER, {"wnd_musicplayer"}},
  {CGI_STATISTICS, {"wnd_statistics"}},
  {CGI_ECONOMICPROGRESS, {"wnd_economicprogress"}},
  {CGI_DIPLOMACY, {"wnd_diplomacy"}},
  {CGI_SHIP, {"wnd_ship"}},
  {CGI_MERCHANDISE_STATISTICS, {"wnd_merchandise_statistics"}}};
