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

#include "iwBuildings.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "WindowManager.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobUsual.h"
#include "files.h"
#include "iwBaseWarehouse.h"
#include "iwBuilding.h"
#include "iwHarborBuilding.h"
#include "iwHelp.h"
#include "iwMilitaryBuilding.h"
#include "ogl/FontStyle.h"
#include "ogl/glFont.h"
#include "world/GameWorldView.h"
#include "world/GameWorldViewer.h"
#include "gameTypes/BuildingCount.h"
#include "gameData/BuildingConsts.h"
#include "gameData/BuildingProperties.h"
#include "gameData/const_gui_ids.h"

/// Reihenfolge der Gebäude
const std::array<BuildingType, 32> bts = {
  BLD_BARRACKS,       BLD_GUARDHOUSE,   BLD_WATCHTOWER, BLD_FORTRESS,   BLD_GRANITEMINE, BLD_COALMINE,    BLD_IRONMINE,
  BLD_GOLDMINE,       BLD_LOOKOUTTOWER, BLD_CATAPULT,   BLD_WOODCUTTER, BLD_FISHERY,     BLD_QUARRY,      BLD_FORESTER,
  BLD_SLAUGHTERHOUSE, BLD_HUNTER,       BLD_BREWERY,    BLD_ARMORY,     BLD_METALWORKS,  BLD_IRONSMELTER, BLD_PIGFARM,
  BLD_STOREHOUSE, // entry 21
  BLD_MILL,           BLD_BAKERY,       BLD_SAWMILL,    BLD_MINT,       BLD_WELL,        BLD_SHIPYARD,    BLD_FARM,
  BLD_DONKEYBREEDER,  BLD_CHARBURNER,
  BLD_HARBORBUILDING // entry 31
};

// Abstand des ersten Icons vom linken oberen Fensterrand
const DrawPoint iconPadding(30, 40);
// Abstand der einzelnen Symbole untereinander
const DrawPoint iconSpacing(40, 48);
// Abstand der Schriften unter den Icons
const unsigned short font_distance_y = 20;

iwBuildings::iwBuildings(GameWorldView& gwv, GameCommandFactory& gcFactory)
    : IngameWindow(CGI_BUILDINGS, IngameWindow::posLastOrCenter, Extent(185, 480), _("Buildings"),
                   LOADER.GetImageN("resource", 41)),
      gwv(gwv), gcFactory(gcFactory)
{
    const Nation playerNation = gwv.GetViewer().GetPlayer().nation;
    // Symbole für die einzelnen Gebäude erstellen
    for(unsigned y = 0; y < bts.size() / 4 + (bts.size() % 4 > 0 ? 1 : 0); ++y)
    {
        for(unsigned x = 0; x < 4; ++x)
        {
            if(y * 4 + x >= bts.size()) //-V547
                break;
            glArchivItem_Bitmap* img;
            if(bts[y * 4 + x] != BLD_CHARBURNER)
                img = LOADER.GetNationIcon(playerNation, bts[y * 4 + x]);
            else
                img = LOADER.GetImageN("charburner", playerNation * 8 + 8);
            Extent btSize = Extent(32, 32);
            DrawPoint btPos = iconPadding - btSize / 2 + iconSpacing * DrawPoint(x, y);
            AddImageButton(y * 4 + x, btPos, btSize, TC_GREY, img, _(BUILDING_NAMES[bts[y * 4 + x]]));
        }
    }

    // Hilfe-Button
    Extent btSize = Extent(30, 32);
    AddImageButton(32, GetSize() - DrawPoint(14, 20) - btSize, btSize, TC_GREY, LOADER.GetImageN("io", 225), _("Help"));
}

/// Anzahlen der Gebäude zeichnen
void iwBuildings::Msg_PaintAfter()
{
    static boost::format fmt("%1%/%2%");
    IngameWindow::Msg_PaintAfter();
    // Anzahlen herausfinden
    BuildingCount bc = gwv.GetViewer().GetPlayer().GetBuildingRegister().GetBuildingNums();

    // Anzahlen unter die Gebäude schreiben
    DrawPoint rowPos = GetDrawPos() + iconPadding + DrawPoint(0, font_distance_y);
    for(unsigned y = 0; y < bts.size() / 4 + (bts.size() % 4 > 0 ? 1 : 0); ++y)
    {
        DrawPoint curPos = rowPos;
        for(unsigned x = 0; x < 4; ++x)
        {
            if(y * 4 + x >= bts.size()) //-V547
                break;
            fmt % bc.buildings[bts[y * 4 + x]] % bc.buildingSites[bts[y * 4 + x]];
            NormalFont->Draw(curPos, fmt.str(), FontStyle::CENTER, COLOR_YELLOW);
            curPos.x += iconSpacing.x;
        }
        rowPos.y += iconSpacing.y;
    }
}

void iwBuildings::Msg_ButtonClick(const unsigned ctrl_id)
{
    if(ctrl_id == 32) // Help button
    {
        WINDOWMANAGER.ReplaceWindow(
          std::make_unique<iwHelp>(_("The building statistics window gives you an insight into "
                                     "the number of buildings you have, by type. The number on "
                                     "the left is the total number of this type of building "
                                     "completed, the number on the right shows how many are "
                                     "currently under construction.")));
        return;
    }

    // no buildings of type complete? -> do nothing
    const GamePlayer& localPlayer = gwv.GetViewer().GetPlayer();
    const BuildingRegister& buildingRegister = localPlayer.GetBuildingRegister();

    BuildingType bldType = bts[ctrl_id];
    if(BuildingProperties::IsMilitary(bldType))
        GoToFirstMatching<iwMilitaryBuilding>(bldType, buildingRegister.GetMilitaryBuildings());
    else if(bldType == BLD_HARBORBUILDING)
        GoToFirstMatching<iwHarborBuilding>(bldType, buildingRegister.GetHarbors());
    else if(BuildingProperties::IsWareHouse(bldType))
        GoToFirstMatching<iwBaseWarehouse>(bldType, buildingRegister.GetStorehouses());
    else
        GoToFirstMatching<iwBuilding>(bldType, buildingRegister.GetBuildings(bldType));
}

template<class T_Window, class T_Building>
void iwBuildings::GoToFirstMatching(BuildingType bldType, const std::list<T_Building*>& blds)
{
    for(T_Building* bld : blds)
    {
        if(bld->GetBuildingType() == bldType)
        {
            gwv.MoveToMapPt(bld->GetPos());
            auto nextscrn = std::make_unique<T_Window>(gwv, gcFactory, static_cast<T_Building*>(bld));
            nextscrn->SetPos(GetPos());
            WINDOWMANAGER.ReplaceWindow(std::move(nextscrn));
            return;
        }
    }
}
