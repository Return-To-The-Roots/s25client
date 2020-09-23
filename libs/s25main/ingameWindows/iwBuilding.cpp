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

#include "iwBuilding.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "WindowManager.h"
#include "buildings/nobShipYard.h"
#include "controls/ctrlImageButton.h"
#include "controls/ctrlPercent.h"
#include "controls/ctrlText.h"
#include "factories/GameCommandFactory.h"
#include "helpers/containerUtils.h"
#include "iwDemolishBuilding.h"
#include "iwHelp.h"
#include "ogl/FontStyle.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glFont.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldView.h"
#include "gameData/BuildingConsts.h"
#include "gameData/BuildingProperties.h"
#include "gameData/const_gui_ids.h"
#include <sstream>

/// IDs in der IO_DAT von Boot und Schiffs-Bild für den Umschaltebutton beim Schiffsbauer
const unsigned IODAT_BOAT_ID = 219;
const unsigned IODAT_SHIP_ID = 218;

iwBuilding::iwBuilding(GameWorldView& gwv, GameCommandFactory& gcFactory, nobUsual* const building)
    : IngameWindow(CGI_BUILDING + MapBase::CreateGUIID(building->GetPos()), IngameWindow::posAtMouse, Extent(226, 194),
                   _(BUILDING_NAMES[building->GetBuildingType()]), LOADER.GetImageN("resource", 41)),
      gwv(gwv), gcFactory(gcFactory), building(building)
{
    // Arbeitersymbol
    AddImage(0, DrawPoint(28, 39), LOADER.GetMapImageN(2298));

    // Exception: charburner
    if(building->GetBuildingType() != BLD_CHARBURNER)
    {
        if(const auto job = BLD_WORK_DESC[building->GetBuildingType()].job)
            AddImage(13, DrawPoint(28, 39), LOADER.GetMapImageN(2300 + *job));
    } else
    {
        AddImage(13, DrawPoint(28, 39), LOADER.GetImageN("io_new", 5));
    }

    // Gebäudesymbol
    AddImage(1, DrawPoint(117, 114), building->GetBuildingImage());

    // Symbol der produzierten Ware (falls hier was produziert wird)
    const auto producedWare = BLD_WORK_DESC[building->GetBuildingType()].producedWare;
    if(producedWare)
    {
        AddImage(2, DrawPoint(196, 39), LOADER.GetMapImageN(2298));
        AddImage(3, DrawPoint(196, 39), LOADER.GetMapImageN(WARES_TEX_MAP_OFFSET + *producedWare));
    }

    // Info
    AddImageButton(4, DrawPoint(16, 147), Extent(30, 32), TC_GREY, LOADER.GetImageN("io", 225), _("Help"));
    // Abreißen
    AddImageButton(5, DrawPoint(50, 147), Extent(34, 32), TC_GREY, LOADER.GetImageN("io", 23), _("Demolish house"));
    // Produktivität einstellen (196,197) (bei Spähturm ausblenden)
    Window* enable_productivity = AddImageButton(
      6, DrawPoint(90, 147), Extent(34, 32), TC_GREY,
      LOADER.GetImageN("io", ((building->IsProductionDisabledVirtual()) ? 197 : 196)), _("Production on/off"));
    if(building->GetBuildingType() == BLD_LOOKOUTTOWER)
        enable_productivity->SetVisible(false);
    // Bei Bootsbauer Button zum Umwählen von Booten und Schiffen
    if(building->GetBuildingType() == BLD_SHIPYARD)
    {
        // Jenachdem Boot oder Schiff anzeigen
        unsigned io_dat_id =
          (static_cast<nobShipYard*>(building)->GetMode() == nobShipYard::BOATS) ? IODAT_BOAT_ID : IODAT_SHIP_ID;
        AddImageButton(11, DrawPoint(130, 147), Extent(43, 32), TC_GREY, LOADER.GetImageN("io", io_dat_id));
    }

    // "Gehe Zum Ort"
    AddImageButton(7, DrawPoint(179, 147), Extent(30, 32), TC_GREY, LOADER.GetImageN("io", 107), _("Go to place"));

    // Gebäudebild und dessen Schatten
    AddImage(8, DrawPoint(117, 114),
             LOADER.GetNationImage(building->GetNation(), 250 + 5 * building->GetBuildingType()));

    // Produktivitätsanzeige (bei Katapulten und Spähtürmen ausblenden)
    Window* productivity = AddPercent(9, DrawPoint(59, 31), Extent(106, 16), TC_GREY, 0xFFFFFF00, SmallFont,
                                      building->GetProductivityPointer());
    if(building->GetBuildingType() == BLD_CATAPULT || building->GetBuildingType() == BLD_LOOKOUTTOWER)
        productivity->SetVisible(false);

    AddText(10, DrawPoint(113, 50), _("(House unoccupied)"), COLOR_RED, FontStyle::CENTER, NormalFont);

    // "Go to next" (building of same type)
    AddImageButton(12, DrawPoint(179, 115), Extent(30, 32), TC_GREY, LOADER.GetImageN("io_new", 11),
                   _("Go to next building of same type"));
}

void iwBuilding::Msg_PaintBefore()
{
    IngameWindow::Msg_PaintBefore();

    // Haus unbesetzt ggf ausblenden
    GetCtrl<ctrlText>(10)->SetVisible(!building->HasWorker());
}

void iwBuilding::Msg_PaintAfter()
{
    IngameWindow::Msg_PaintAfter();
    const auto& bldWorkDesk = BLD_WORK_DESC[building->GetBuildingType()];
    if(BuildingProperties::IsMine(building->GetBuildingType()))
    {
        // Bei Bergwerken sieht die Nahrungsanzeige ein wenig anders aus (3x 2)

        // "Schwarzer Rahmen"
        DrawRectangle(Rect(GetDrawPos() + DrawPoint(40, 60), Extent(144, 24)), 0x80000000);
        DrawPoint curPos = GetDrawPos() + DrawPoint(52, 72);
        for(unsigned char i = 0; i < bldWorkDesk.waresNeeded.size(); ++i)
        {
            for(unsigned char z = 0; z < bldWorkDesk.numSpacesPerWare; ++z)
            {
                glArchivItem_Bitmap* bitmap = LOADER.GetMapImageN(WARES_TEX_MAP_OFFSET + bldWorkDesk.waresNeeded[i]);
                bitmap->DrawFull(curPos, (z < building->GetNumWares(i) ? 0xFFFFFFFF : 0xFF404040));
                curPos.x += 24;
            }
        }
    } else
    {
        DrawPoint curPos = GetDrawPos() + DrawPoint(GetSize().x / 2, 60);
        for(unsigned char i = 0; i < bldWorkDesk.waresNeeded.size(); ++i)
        {
            const unsigned wares_count = bldWorkDesk.numSpacesPerWare;

            // "Schwarzer Rahmen"
            DrawPoint waresPos = curPos - DrawPoint(24 * wares_count / 2, 0);
            DrawRectangle(Rect(waresPos, Extent(24 * wares_count, 24)), 0x80000000);
            waresPos += DrawPoint(12, 12);

            for(unsigned char z = 0; z < wares_count; ++z)
            {
                glArchivItem_Bitmap* bitmap = LOADER.GetMapImageN(WARES_TEX_MAP_OFFSET + bldWorkDesk.waresNeeded[i]);
                bitmap->DrawFull(waresPos, (z < building->GetNumWares(i) ? COLOR_WHITE : 0xFF404040));
                waresPos.x += 24;
            }

            std::stringstream text;
            text << (unsigned)building->GetNumWares(i) << "/" << wares_count;
            NormalFont->Draw(curPos + DrawPoint(0, 12), text.str(), FontStyle::CENTER | FontStyle::VCENTER);
            curPos.y += 29;
        }
    }
}

void iwBuilding::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 4: // Hilfe
        {
            WINDOWMANAGER.ReplaceWindow(
              std::make_unique<iwHelp>(_(BUILDING_HELP_STRINGS[building->GetBuildingType()])));
        }
        break;
        case 5: // Gebäude abbrennen
        {
            // Abreißen?
            Close();
            WINDOWMANAGER.Show(std::make_unique<iwDemolishBuilding>(gwv, building));
        }
        break;
        case 6:
        {
            // Produktion einstellen/fortführen
            // NC senden
            if(gcFactory.SetProductionEnabled(building->GetPos(), building->IsProductionDisabledVirtual()))
            {
                // visuell anzeigen, falls erfolgreich
                building->ToggleProductionVirtual();

                // anderes Bild auf dem Button
                if(building->IsProductionDisabledVirtual())
                    GetCtrl<ctrlImageButton>(6)->SetImage(LOADER.GetImageN("io", 197));
                else
                    GetCtrl<ctrlImageButton>(6)->SetImage(LOADER.GetImageN("io", 196));

                auto* text = GetCtrl<ctrlText>(10);
                if(building->IsProductionDisabledVirtual() && building->HasWorker())
                    text->SetText(_("(House unoccupied)"));
                else if(building->HasWorker())
                    text->SetVisible(false);
            }
        }
        break;
        case 7: // "Gehe Zum Ort"
        {
            gwv.MoveToMapPt(building->GetPos());
        }
        break;
        case 11: // Schiff/Boot umstellen bei Schiffsbauer
        {
            if(gcFactory.SetShipYardMode(building->GetPos(),
                                         static_cast<const nobShipYard*>(building)->GetMode() == nobShipYard::BOATS))
            {
                // Auch optisch den Button umstellen
                auto* button = GetCtrl<ctrlImageButton>(11);
                if(button->GetImage() == LOADER.GetImageN("io", IODAT_BOAT_ID))
                    button->SetImage(LOADER.GetImageN("io", IODAT_SHIP_ID));
                else
                    button->SetImage(LOADER.GetImageN("io", IODAT_BOAT_ID));
            }
        }
        break;
        case 12: // go to next of same type
        {
            const std::list<nobUsual*>& buildings = gwv.GetWorld()
                                                      .GetPlayer(building->GetPlayer())
                                                      .GetBuildingRegister()
                                                      .GetBuildings(building->GetBuildingType());
            // go through list once we get to current building -> open window for the next one and go to next location
            auto it = helpers::find_if(
              buildings, [bldPos = building->GetPos()](const auto* it) { return it->GetPos() == bldPos; });
            if(it != buildings.end()) // got to current building in the list?
            {
                // close old window, open new window (todo: only open if it isnt already open), move to location of next
                // building
                Close();
                ++it;
                if(it == buildings.end()) // was last entry in list -> goto first
                    it = buildings.begin();
                gwv.MoveToMapPt((*it)->GetPos());
                WINDOWMANAGER.ReplaceWindow(std::make_unique<iwBuilding>(gwv, gcFactory, *it)).SetPos(GetPos());
                break;
            }
        }
        break;
    }
}
