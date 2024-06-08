// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwTempleBuilding.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "WindowManager.h"
#include "WineLoader.h"
#include "buildings/nobTemple.h"
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
#include "gameData/const_gui_ids.h"
#include <sstream>

iwTempleBuilding::iwTempleBuilding(GameWorldView& gwv, GameCommandFactory& gcFactory, nobUsual* const building)
    : IngameWindow(CGI_BUILDING + MapBase::CreateGUIID(building->GetPos()), IngameWindow::posAtMouse, Extent(226, 223),
                   _(BUILDING_NAMES[building->GetBuildingType()]), LOADER.GetImageN("resource", 41)),
      gwv(gwv), gcFactory(gcFactory), building(building)
{
    AddImage(1, DrawPoint(28, 39), LOADER.GetJobTex(*BLD_WORK_DESC[building->GetBuildingType()].job));
    AddImage(2, DrawPoint(117, 160), &building->GetBuildingImage());
    AddImage(3, DrawPoint(196, 39), LOADER.GetMapTexture(2298));
    AddImage(4, DrawPoint(196, 39),
             wineaddon::GetTempleProductionModeTex(static_cast<nobTemple*>(building)->GetProductionMode()));
    AddImageButton(5, DrawPoint(16, 176), Extent(30, 32), TextureColor::Grey, LOADER.GetImageN("io", 225), _("Help"));
    AddImageButton(6, DrawPoint(50, 176), Extent(34, 32), TextureColor::Grey, LOADER.GetImageN("io", 23),
                   _("Demolish house"));
    AddImageButton(7, DrawPoint(90, 176), Extent(34, 32), TextureColor::Grey,
                   LOADER.GetImageN("io", ((building->IsProductionDisabledVirtual()) ? 197 : 196)),
                   _("Production on/off"));
    AddImageButton(8, DrawPoint(130, 176), Extent(34, 32), TextureColor::Grey,
                   wineaddon::GetTempleProductionModeTex(static_cast<nobTemple*>(building)->GetProductionMode()));
    AddImageButton(9, DrawPoint(179, 176), Extent(30, 32), TextureColor::Grey, LOADER.GetImageN("io", 107),
                   _("Go to place"));
    AddPercent(10, DrawPoint(59, 31), Extent(106, 16), TextureColor::Grey, 0xFFFFFF00, SmallFont,
               building->GetProductivityPointer());
    AddText(11, DrawPoint(113, 50), _("(House unoccupied)"), COLOR_RED, FontStyle::CENTER, NormalFont);
    AddImageButton(12, DrawPoint(179, 144), Extent(30, 32), TextureColor::Grey, LOADER.GetImageN("io_new", 11),
                   _("Go to next building of same type"));
}

void iwTempleBuilding::Msg_PaintBefore()
{
    IngameWindow::Msg_PaintBefore();
    GetCtrl<ctrlText>(11)->SetVisible(!building->HasWorker());
}

void iwTempleBuilding::Msg_PaintAfter()
{
    IngameWindow::Msg_PaintAfter();
    const auto& bldWorkDesk = BLD_WORK_DESC[building->GetBuildingType()];
    DrawPoint curPos = GetDrawPos() + DrawPoint(GetSize().x / 2, 60);
    for(unsigned char i = 0; i < bldWorkDesk.waresNeeded.size(); ++i)
    {
        const unsigned wares_count = bldWorkDesk.numSpacesPerWare;

        // Black border
        DrawPoint waresPos = curPos - DrawPoint(24 * wares_count / 2, 0);
        DrawRectangle(Rect(waresPos, Extent(24 * wares_count, 24)), 0x80000000);
        waresPos += DrawPoint(12, 12);

        for(unsigned char z = 0; z < wares_count; ++z)
        {
            LOADER.GetWareTex(bldWorkDesk.waresNeeded[i])
              ->DrawFull(waresPos, (z < building->GetNumWares(i) ? COLOR_WHITE : 0xFF404040));
            waresPos.x += 24;
        }

        std::stringstream text;
        text << (unsigned)building->GetNumWares(i) << "/" << wares_count;
        NormalFont->Draw(curPos + DrawPoint(0, 12), text.str(), FontStyle::CENTER | FontStyle::VCENTER);
        curPos.y += 29;
    }
}

void iwTempleBuilding::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 5: // Help
        {
            WINDOWMANAGER.ReplaceWindow(
              std::make_unique<iwHelp>(_(BUILDING_HELP_STRINGS[building->GetBuildingType()])));
        }
        break;
        case 6: // Burn building
        {
            // Destroy?
            Close();
            WINDOWMANAGER.Show(std::make_unique<iwDemolishBuilding>(gwv, building));
        }
        break;
        case 7:
        {
            // Stop/continue production
            // Send NC
            if(gcFactory.SetProductionEnabled(building->GetPos(), building->IsProductionDisabledVirtual()))
            {
                // visuell anzeigen, falls erfolgreich
                building->ToggleProductionVirtual();

                // other image on button
                if(building->IsProductionDisabledVirtual())
                    GetCtrl<ctrlImageButton>(7)->SetImage(LOADER.GetImageN("io", 197));
                else
                    GetCtrl<ctrlImageButton>(7)->SetImage(LOADER.GetImageN("io", 196));

                auto* text = GetCtrl<ctrlText>(11);
                if(building->IsProductionDisabledVirtual() && building->HasWorker())
                    text->SetText(_("(House unoccupied)"));
                else if(building->HasWorker())
                    text->SetVisible(false);
            }
        }
        break;
        case 8:
        {
            const auto nextProductionMode = static_cast<nobTemple*>(building)->getNextProductionMode();
            if(gcFactory.SetTempleProductionMode(building->GetPos(), nextProductionMode))
            {
                GetCtrl<ctrlImageButton>(8)->SetImage(wineaddon::GetTempleProductionModeTex(nextProductionMode));
                static_cast<nobTemple*>(building)->SetProductionMode(nextProductionMode);
            }
        }
        break;
        case 9: // "Go to place"
        {
            gwv.MoveToMapPt(building->GetPos());
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
                WINDOWMANAGER.ReplaceWindow(std::make_unique<iwTempleBuilding>(gwv, gcFactory, *it)).SetPos(GetPos());
                break;
            }
        }
        break;
    }
}
