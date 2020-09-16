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

#include "iwBuildingProductivities.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "files.h"
#include "gameData/BuildingConsts.h"
#include "gameData/const_gui_ids.h"
#include "s25util/colors.h"

/// Reihenfolge der Gebäude
const std::array<BuildingType, 24> bts = {
  BLD_GRANITEMINE, BLD_COALMINE,    BLD_IRONMINE,       BLD_GOLDMINE, BLD_WOODCUTTER,    BLD_FISHERY,
  BLD_QUARRY,      BLD_FORESTER,    BLD_SLAUGHTERHOUSE, BLD_HUNTER,   BLD_BREWERY,       BLD_ARMORY,
  BLD_METALWORKS,  BLD_IRONSMELTER, BLD_PIGFARM,        BLD_MILL,     BLD_BAKERY,        BLD_SAWMILL,
  BLD_MINT,        BLD_WELL,        BLD_SHIPYARD,       BLD_FARM,     BLD_DONKEYBREEDER, BLD_CHARBURNER};

/// Abstand vom linken, oberen Fensterrand
const Extent bldProdContentOffset(50, 30);
/// Abstand vom rechten Fensterrand
const unsigned short right_x = 40;
/// Horizontaler Abstand zwischen Bild und Prozentbar
const unsigned short image_percent_x = 35;
/// Horizontaler Abstand zwischen Prozentbar und nächstem Bild
const unsigned short percent_image_x = 40;
/// Vertikaler Abstand zwischen 2 nacheinanderfolgenden "Zeilen"
const unsigned short distance_y = 35;

/// Größe der Prozentbalken
const Extent percentSize(100, 18);

iwBuildingProductivities::iwBuildingProductivities(const GamePlayer& player)
    : IngameWindow(CGI_BUILDINGSPRODUCTIVITY, IngameWindow::posLastOrCenter,
                   Extent(2 * percentSize.x + 2 * image_percent_x + percent_image_x + right_x,
                          (bts.size() / 2 + 1) * (distance_y + 1))
                     + bldProdContentOffset,
                   _("Productivity"), LOADER.GetImageN("resource", 41)),
      player(player), percents(NUM_BUILDING_TYPES, 0)
{
    const Nation playerNation = player.nation;
    for(unsigned y = 0; y < bts.size() / 2 + bts.size() % 2; ++y)
    {
        for(unsigned x = 0; x < 2; ++x)
        {
            if(y * 2 + x >= bts.size()) //-V547
                break;
            unsigned imgId = (y * 2 + x) * 2;
            DrawPoint imgPos(x * (percent_image_x + percentSize.x + image_percent_x),
                             distance_y * y + percentSize.y / 2);
            imgPos = imgPos + bldProdContentOffset;
            if(player.IsBuildingEnabled(bts[y * 2 + x]))
            {
                glArchivItem_Bitmap* img;
                if(bts[y * 2 + x] != BLD_CHARBURNER)
                    img = LOADER.GetNationIcon(playerNation, bts[y * 2 + x]);
                else
                    img = LOADER.GetImageN("charburner", playerNation * 8 + 8);
                AddImage(imgId, imgPos, img, _(BUILDING_NAMES[bts[y * 2 + x]]));
                DrawPoint percentPos(image_percent_x + x * (percent_image_x + percentSize.x + image_percent_x),
                                     distance_y * y);
                AddPercent(imgId + 1, percentPos + bldProdContentOffset, percentSize, TC_GREY, COLOR_YELLOW, SmallFont,
                           &percents[bts[y * 2 + x]]);
            } else
                AddImage(imgId, imgPos, LOADER.GetImageN("io", 188));
        }
    }

    UpdatePercents();

    // Hilfe-Button
    // Original S2 does not have a Help button in this window. Add it if you have something to say.
    // AddImageButton(500, GetSize().x - 14 - 30, GetSize().y - 20 - 32, 30, 32, TC_GREY, LOADER.GetImageN("io", 225),
    // _("Help"));
}

/// Aktualisieren der Prozente
void iwBuildingProductivities::UpdatePercents()
{
    player.GetBuildingRegister().CalcProductivities(percents);
}

/// Produktivitäts-percentbars aktualisieren
void iwBuildingProductivities::Msg_PaintAfter()
{
    IngameWindow::Msg_PaintAfter();
    UpdatePercents();
}
