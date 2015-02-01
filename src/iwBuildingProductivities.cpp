// $Id: iwBuildingProductivities.cpp 9594 2015-02-01 09:40:27Z marcus $
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
#include "iwBuildingProductivities.h"
#include "Loader.h"
#include "GameClient.h"


/// Anzahl der angezeigten Gebäude
const unsigned BUILDINGS_COUNT = 24;

/// Reihenfolge der Gebäude
const BuildingType bts[BUILDINGS_COUNT] =
{
    BLD_GRANITEMINE,
    BLD_COALMINE,
    BLD_IRONMINE,
    BLD_GOLDMINE,
    BLD_WOODCUTTER,
    BLD_FISHERY,
    BLD_QUARRY,
    BLD_FORESTER,
    BLD_SLAUGHTERHOUSE,
    BLD_HUNTER,
    BLD_BREWERY,
    BLD_ARMORY,
    BLD_METALWORKS,
    BLD_IRONSMELTER,
    BLD_PIGFARM,
    BLD_MILL,
    BLD_BAKERY,
    BLD_SAWMILL,
    BLD_MINT,
    BLD_WELL,
    BLD_SHIPYARD,
    BLD_FARM,
    BLD_DONKEYBREEDER,
    BLD_CHARBURNER
};



/// Abstand vom linken, oberen Fensterrand
const unsigned short left_x = 50;
const unsigned short top_y = 30;
/// Abstand vom rechten Fensterrand
const unsigned short right_x = 40;
/// Horizontaler Abstand zwischen Bild und Prozentbar
const unsigned short image_percent_x = 35;
/// Horizontaler Abstand zwischen Prozentbar und nächstem Bild
const unsigned short percent_image_x = 40;
/// Vertikaler Abstand zwischen 2 nacheinanderfolgenden "Zeilen"
const unsigned short distance_y = 35;

/// Breite der Prozentbalken
const unsigned short percent_width = 100;
/// Höhe der Scrollbars
const unsigned short percent_height = 18;



/// Konstruktor von @p iwBuildingProductivities.
iwBuildingProductivities::iwBuildingProductivities()
    : IngameWindow(CGI_BUILDINGSPRODUCTIVITY, 0xFFFE, 0xFFFE,
                   left_x + 2 * percent_width + 2 * image_percent_x + percent_image_x + right_x,
                   top_y + (BUILDINGS_COUNT / 2 + 1) * (distance_y + 1), _("Productivity"), LOADER.GetImageN("resource", 41)),
    percents(40, 0)
{
    for(unsigned y = 0; y < BUILDINGS_COUNT / 2 + BUILDINGS_COUNT % 2; ++y)
    {
        for(unsigned x = 0; x < 2; ++x)
        {
            if(y * 2 + x < BUILDINGS_COUNT)
            {
				if(bts[y*2+x]!=BLD_CHARBURNER)
					AddImage((y * 2 + x) * 2, left_x + x * (percent_image_x + percent_width + image_percent_x), top_y + distance_y * y + percent_height / 2,LOADER.GetImageN(NATION_ICON_IDS[GameClient::inst().GetLocalPlayer()->nation], bts[y * 2 + x]), _(BUILDING_NAMES[bts[y * 2 + x]]));
				else
					AddImage((y * 2 + x) * 2, left_x + x * (percent_image_x + percent_width + image_percent_x), top_y + distance_y * y + percent_height / 2,LOADER.GetImageN("charburner", GameClient::inst().GetLocalPlayer()->nation * 8 + 8), _(BUILDING_NAMES[bts[y * 2 + x]]));

                AddPercent((y * 2 + x) * 2 + 1, left_x + image_percent_x + x * (percent_image_x + percent_width + image_percent_x), top_y + distance_y * y,
                           percent_width, percent_height, TC_GREY, COLOR_YELLOW, SmallFont, &percents[bts[y * 2 + x]]);
            }
        }
    }

    UpdatePercents();

    // Hilfe-Button
    AddImageButton(500, width - 14 - 30, height - 20 - 32, 30, 32, TC_GREY, LOADER.GetImageN("io", 21), _("Help"));
}

/// Aktualisieren der Prozente
void iwBuildingProductivities::UpdatePercents()
{
    GameClient::inst().GetLocalPlayer()->CalcProductivities(percents);
}

/// Produktivitäts-percentbars aktualisieren
void iwBuildingProductivities::Msg_PaintAfter()
{
    UpdatePercents();
}

