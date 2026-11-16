// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwBuildings.h"
#include "AddonHelperFunctions.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "LeatherLoader.h"
#include "Loader.h"
#include "WindowManager.h"
#include "WineLoader.h"
#include "addons/const_addons.h"
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
#include "iwTempleBuilding.h"
#include "ogl/FontStyle.h"
#include "ogl/glFont.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldView.h"
#include "world/GameWorldViewer.h"
#include "gameTypes/BuildingCount.h"
#include "gameData/BuildingConsts.h"
#include "gameData/BuildingProperties.h"
#include "gameData/const_gui_ids.h"

void iwBuildings::setBuildingOrder()
{
    // Order of the buildings in which they will be shown
    bts = {
      BuildingType::Barracks,       BuildingType::Guardhouse, BuildingType::Watchtower,     BuildingType::Fortress,
      BuildingType::GraniteMine,    BuildingType::CoalMine,   BuildingType::IronMine,       BuildingType::GoldMine,
      BuildingType::LookoutTower,   BuildingType::Catapult,   BuildingType::Woodcutter,     BuildingType::Fishery,
      BuildingType::Quarry,         BuildingType::Forester,   BuildingType::Slaughterhouse, BuildingType::Hunter,
      BuildingType::Brewery,        BuildingType::Armory,     BuildingType::Metalworks,     BuildingType::Ironsmelter,
      BuildingType::PigFarm,
      BuildingType::Storehouse, // entry 21
      BuildingType::Mill,           BuildingType::Bakery,     BuildingType::Sawmill,        BuildingType::Mint,
      BuildingType::Well,           BuildingType::Shipyard,   BuildingType::Farm,           BuildingType::DonkeyBreeder,
      BuildingType::Charburner,
      BuildingType::HarborBuilding,                                                       // entry 31
      BuildingType::Vineyard,       BuildingType::Winery,     BuildingType::Temple,       // entry 34
      BuildingType::Skinner,        BuildingType::Tannery,    BuildingType::LeatherWorks, // entry 37
    };

    helpers::erase_if(bts, makeIsUnusedBuilding(gwv.getWorld().getGGS()));
}

// Abstand des ersten Icons vom linken oberen Fensterrand
const Extent bldContentOffset(30, 40);
// Abstand der einzelnen Symbole untereinander
const Extent iconSpacing(40, 48);
// Abstand der Schriften unter den Icons
const unsigned short font_distance_y = 20;

namespace {
enum
{
    ID_Help,
    ID_BuildingsStart,
};
}

iwBuildings::iwBuildings(GameWorldView& gwv, GameCommandFactory& gcFactory)
    : IngameWindow(CGI_BUILDINGS, IngameWindow::posLastOrCenter, Extent(185, 480), _("Buildings"),
                   LOADER.getImageN("resource", 41)),
      gwv(gwv), gcFactory(gcFactory)
{
    setBuildingOrder();
    Resize(iconSpacing * Extent(4, helpers::divCeil(bts.size(), 4) + 1) + bldContentOffset);

    const Nation playerNation = gwv.getViewer().getPlayer().nation;
    // Symbole für die einzelnen Gebäude erstellen
    for(unsigned y = 0; y < bts.size() / 4 + (bts.size() % 4 > 0 ? 1 : 0); ++y)
    {
        for(unsigned x = 0; x < 4; ++x)
        {
            if(y * 4 + x >= bts.size()) //-V547
                break;

            Extent btSize = Extent(32, 32);
            DrawPoint btPos = bldContentOffset - btSize / 2 + iconSpacing * DrawPoint(x, y);
            AddImageButton(ID_BuildingsStart + y * 4 + x, btPos, btSize, TextureColor::Grey,
                           LOADER.getNationIcon(playerNation, bts[y * 4 + x]), _(BUILDING_NAMES[bts[y * 4 + x]]));
        }
    }

    // "Help" button
    Extent btSize = Extent(30, 32);
    AddImageButton(ID_Help, getFullSize() - DrawPoint(14, 20) - btSize, btSize, TextureColor::Grey,
                   LOADER.getImageN("io", 225), _("Help"));
}

/// Anzahlen der Gebäude zeichnen
void iwBuildings::msgPaintAfter()
{
    static boost::format fmt("%1%/%2%");
    IngameWindow::msgPaintAfter();
    // Anzahlen herausfinden
    BuildingCount bc = gwv.getViewer().getPlayer().getBuildingRegister().getBuildingNums();

    // Anzahlen unter die Gebäude schreiben
    DrawPoint rowPos = getDrawPos() + bldContentOffset + DrawPoint(0, font_distance_y);
    for(unsigned y = 0; y < helpers::divCeil(bts.size(), 4); ++y)
    {
        DrawPoint curPos = rowPos;
        for(unsigned x = 0; x < 4; x++)
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

void iwBuildings::msgButtonClick(const unsigned ctrl_id)
{
    if(ctrl_id == ID_Help) // Help button
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
    const GamePlayer& localPlayer = gwv.getViewer().getPlayer();
    const BuildingRegister& buildingRegister = localPlayer.getBuildingRegister();

    BuildingType bldType = bts[ctrl_id - ID_BuildingsStart];
    if(BuildingProperties::isMilitary(bldType))
        GoToFirstMatching<iwMilitaryBuilding>(bldType, buildingRegister.getMilitaryBuildings());
    else if(bldType == BuildingType::HarborBuilding)
        GoToFirstMatching<iwHarborBuilding>(bldType, buildingRegister.getHarbors());
    else if(BuildingProperties::isWareHouse(bldType))
        GoToFirstMatching<iwBaseWarehouse>(bldType, buildingRegister.getStorehouses());
    else if(bldType == BuildingType::Temple)
        GoToFirstMatching<iwTempleBuilding>(bldType, buildingRegister.getBuildings(bldType));
    else
        GoToFirstMatching<iwBuilding>(bldType, buildingRegister.getBuildings(bldType));
}

template<class T_Window, class T_Building>
void iwBuildings::GoToFirstMatching(BuildingType bldType, const std::list<T_Building*>& blds)
{
    for(T_Building* bld : blds)
    {
        if(bld->getBuildingType() == bldType)
        {
            gwv.MoveToMapPt(bld->getPos());
            auto nextscrn = std::make_unique<T_Window>(gwv, gcFactory, static_cast<T_Building*>(bld));
            nextscrn->setPos(getPos());
            WINDOWMANAGER.ReplaceWindow(std::move(nextscrn));
            return;
        }
    }
}
