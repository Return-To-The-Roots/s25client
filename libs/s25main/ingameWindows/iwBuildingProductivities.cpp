// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwBuildingProductivities.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "files.h"
#include "helpers/mathFuncs.h"
#include "gameData/BuildingConsts.h"
#include "gameData/const_gui_ids.h"
#include "s25util/colors.h"

const std::array<BuildingType, 27> iwBuildingProductivities::icons = {
  // clang-format off
  BuildingType::Woodcutter,    BuildingType::Slaughterhouse,
  BuildingType::Forester,      BuildingType::Metalworks,
  BuildingType::Quarry,        BuildingType::Armory,
  BuildingType::Fishery,       BuildingType::Ironsmelter,
  BuildingType::Hunter,        BuildingType::Mint,
  BuildingType::Sawmill,       BuildingType::Brewery,
  BuildingType::Mill,          BuildingType::Shipyard,
  BuildingType::Bakery,        BuildingType::GoldMine,
  BuildingType::Well,          BuildingType::IronMine,
  BuildingType::Farm,          BuildingType::CoalMine,
  BuildingType::PigFarm,       BuildingType::GraniteMine,
  BuildingType::DonkeyBreeder, BuildingType::Charburner,
  BuildingType::Vineyard,      BuildingType::Winery,
  BuildingType::Temple,
  // clang-format on
};

/// Abstand vom linken, oberen Fensterrand
constexpr Extent bldProdContentOffset(50, 30);
/// Horizontaler Abstand zwischen Bild und Prozentbar
constexpr unsigned short image_percent_x = 35;
/// Horizontaler Abstand zwischen Prozentbar und nächstem Bild
constexpr unsigned short percent_image_x = 40;
/// Vertikaler Abstand zwischen 2 nacheinanderfolgenden "Zeilen"
constexpr unsigned short distance_y = 35;
/// Größe der Prozentbalken
constexpr Extent percentSize(100, 18);

constexpr unsigned numRows = helpers::divCeil(iwBuildingProductivities::icons.size(), 2);
constexpr Extent cellSize(percent_image_x + percentSize.x + image_percent_x, distance_y);

iwBuildingProductivities::iwBuildingProductivities(const GamePlayer& player)
    : IngameWindow(CGI_BUILDINGSPRODUCTIVITY, IngameWindow::posLastOrCenter,
                   cellSize * Extent(2, numRows + 1) + bldProdContentOffset, _("Productivity"),
                   LOADER.GetImageN("resource", 41)),
      player(player), percents()
{
    const Nation playerNation = player.nation;
    unsigned curIdx = 0;
    for(unsigned y = 0; y < numRows; ++y)
    {
        for(unsigned x = 0; x < 2; ++x, ++curIdx)
        {
            if(curIdx >= icons.size()) //-V547
                break;
            const DrawPoint curPos = cellSize * DrawPoint(x, y) + bldProdContentOffset;
            const DrawPoint imgPos = curPos + DrawPoint(0, percentSize.y / 2);
            const unsigned imgId = curIdx * 2;
            if(player.IsBuildingEnabled(icons[curIdx]))
            {
                AddImage(imgId, imgPos, LOADER.GetNationIcon(playerNation, icons[curIdx]),
                         _(BUILDING_NAMES[icons[curIdx]]));
                DrawPoint percentPos = curPos + DrawPoint(image_percent_x, 0);
                AddPercent(imgId + 1, percentPos, percentSize, TextureColor::Grey, COLOR_YELLOW, SmallFont,
                           &percents[icons[curIdx]]);
            } else
                AddImage(imgId, imgPos, LOADER.GetImageN("io", 188));
        }
    }

    UpdatePercents();

    // Hilfe-Button
    // Original S2 does not have a Help button in this window. Add it if you have something to say.
    // AddImageButton(500, GetSize().x - 14 - 30, GetSize().y - 20 - 32, 30, 32, TextureColor::Grey,
    // LOADER.GetImageN("io", 225),
    // _("Help"));
}

void iwBuildingProductivities::UpdatePercents()
{
    percents = player.GetBuildingRegister().CalcProductivities();
}

void iwBuildingProductivities::Msg_PaintAfter()
{
    IngameWindow::Msg_PaintAfter();
    UpdatePercents();
}
