// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "rttrDefines.h" // IWYU pragma: keep
#include "iwTrade.h"

#include "GamePlayer.h"
#include "Loader.h"
#include "buildings/nobBaseWarehouse.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlImage.h"
#include "controls/ctrlText.h"
#include "factories/GameCommandFactory.h"
#include "helpers/converters.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldViewer.h"
#include "gameData/JobConsts.h"
#include "gameData/ShieldConsts.h"
#include <boost/format.hpp>
#include <cstdio>

#include "ogl/FontStyle.h"
#include "ogl/glArchivItem_Bitmap.h"
iwTrade::iwTrade(const nobBaseWarehouse& wh, const GameWorldViewer& gwv, GameCommandFactory& gcFactory)
    : IngameWindow(wh.CreateGUIID(), IngameWindow::posAtMouse, Extent(400, 194), _("Trade"), LOADER.GetImageN("resource", 41)), wh(wh),
      gwv(gwv), gcFactory(gcFactory), possibleSrcWarehouses(gwv.GetPlayer().GetWarehousesForTrading(wh))
{
    // Get title of the player
    SetTitle((boost::format(_("Trade with %s")) % gwv.GetWorld().GetPlayer(wh.GetPlayer()).name).str());
    // Gebäudebild und dessen Schatten
    AddImage(0, DrawPoint(100, 144), LOADER.GetNationImage(wh.GetNation(), 250 + 5 * wh.GetBuildingType()));

    const unsigned left_column = 200;

    AddComboBox(4, DrawPoint(left_column, 84), Extent(160, 18), TC_GREY, NormalFont, 90); // Ware/Figure names
    AddText(1, DrawPoint(left_column, 30), "Deal in:", COLOR_YELLOW, FontStyle::LEFT, NormalFont);
    ctrlComboBox* box = this->AddComboBox(2, DrawPoint(left_column, 44), Extent(160, 18), TC_GREY, NormalFont, 200); // Ware or figure?
    box->AddString(_("Wares"));
    box->AddString(_("Settlers"));
    AddText(3, DrawPoint(left_column, 70), "Type:", COLOR_YELLOW, FontStyle::LEFT, NormalFont);

    // Create possible wares, figures
    for(unsigned i = 0; i < NUM_WARE_TYPES; ++i)
    {
        // Only add one shield type
        if(GoodType(i) != ConvertShields(GoodType(i)))
            continue;
        // Don't add empty water
        if(i == GD_WATEREMPTY)
            continue;
        wares.push_back(GoodType(i));
    }
    for(unsigned i = 0; i < NUM_JOB_TYPES; ++i)
    {
        // Can't trade boat carriers
        if(i == JOB_BOATCARRIER)
            continue;
        jobs.push_back(Job(i));
    }

    AddImage(5, DrawPoint(left_column + 20, 130), static_cast<ITexture*>(NULL), _("Ware you like to trade"));
    AddEdit(6, DrawPoint(left_column + 34, 120), Extent(39, 20), TC_GREY, NormalFont)->SetNumberOnly(true);
    AddText(7, DrawPoint(left_column + 75, 125), "/ 20", COLOR_YELLOW, FontStyle::LEFT, NormalFont);

    AddTextButton(8, DrawPoint(left_column, 150), Extent(150, 22), TC_GREEN2, _("Send"), NormalFont);

    // Choose wares at first
    box->SetSelection(0);
    Msg_ComboSelectItem(2, 0);
}

void iwTrade::Msg_PaintBefore()
{
    IngameWindow::Msg_PaintBefore();

    // Schatten des Gebäudes (muss hier gezeichnet werden wegen schwarz und halbdurchsichtig)
    glArchivItem_Bitmap* bitmap = LOADER.GetNationImage(wh.GetNation(), 250 + 5 * wh.GetBuildingType() + 1);

    if(bitmap)
    {
        ctrlImage* img = GetCtrl<ctrlImage>(0);
        bitmap->DrawFull(img->GetDrawPos(), COLOR_SHADOW);
    }
}

void iwTrade::Msg_PaintAfter() {}

void iwTrade::Msg_ButtonClick(const unsigned /*ctrl_id*/)
{
    // pressed the send button
    unsigned short ware_figure_selection = GetCtrl<ctrlComboBox>(4)->GetSelection();
    bool ware_figure = this->GetCtrl<ctrlComboBox>(2)->GetSelection() == 1;
    GoodType gt = ware_figure ? GD_NOTHING : wares[ware_figure_selection];
    Job job = ware_figure ? jobs[ware_figure_selection] : JOB_NOTHING;

    const std::string number_str = GetCtrl<ctrlEdit>(6)->GetText();

    // Start trading
    if(!GetCtrl<ctrlComboBox>(4)->GetCtrl<ctrlList>(0)->IsVisible() && helpers::fromString(number_str, 0) > 0)
    {
        gcFactory.TradeOverLand(wh.GetPos(), gt, job, helpers::fromString(number_str, 0));
        this->Close();
    }
}

void iwTrade::Msg_ComboSelectItem(const unsigned ctrl_id, const int selection)
{
    switch(ctrl_id)
    {
        // Change ware/figure mode
        case 2:
        {
            ctrlComboBox* names = this->GetCtrl<ctrlComboBox>(4);
            names->DeleteAllItems();
            if(selection == 0)
            {
                // Add ware names
                for(unsigned i = 0; i < wares.size(); ++i)
                    names->AddString(_(WARE_NAMES[wares[i]]));

            } else
            {
                // Add job names
                for(unsigned i = 0; i < jobs.size(); ++i)
                    names->AddString(_(JOB_NAMES[jobs[i]]));
            }
            names->SetSelection(0);
            Msg_ComboSelectItem(4, 0);
        }
        break;
        case 4:
        {
            unsigned number;
            if(this->GetCtrl<ctrlComboBox>(2)->GetSelection() == 0)
            {
                // Wares

                // Set the new image of the ware which was selected
                GetCtrl<ctrlImage>(5)->SetImage(LOADER.GetMapImageN(2250 + wares[selection]));

                // Get the number of available wares
                number = GetPossibleTradeAmount(wares[selection]);
            } else
            {
                glArchivItem_Bitmap* image = LOADER.GetMapImageN(2300 + jobs[selection]);
                // Exception: charburner
                if(jobs[selection] == JOB_CHARBURNER)
                    image = LOADER.GetImageN("io_new", 5);
                GetCtrl<ctrlImage>(5)->SetImage(image);

                // Get the number of available figures
                number = GetPossibleTradeAmount(jobs[selection]);
            }

            char str[256];
            sprintf(str, "/ %u", number);
            GetCtrl<ctrlText>(7)->SetText(str);
        }
        break;
    }
}

unsigned iwTrade::GetPossibleTradeAmount(const Job job) const
{
    const GamePlayer& player = gwv.GetPlayer();
    unsigned amount = 0;
    for(std::vector<nobBaseWarehouse*>::const_iterator it = possibleSrcWarehouses.begin(); it != possibleSrcWarehouses.end(); ++it)
    {
        if(player.IsWarehouseValid(*it))
            amount += (*it)->GetAvailableFiguresForTrading(job);
    }
    return amount;
}

unsigned iwTrade::GetPossibleTradeAmount(const GoodType good) const
{
    const GamePlayer& player = gwv.GetPlayer();
    unsigned amount = 0;
    for(std::vector<nobBaseWarehouse*>::const_iterator it = possibleSrcWarehouses.begin(); it != possibleSrcWarehouses.end(); ++it)
    {
        if(player.IsWarehouseValid(*it))
            amount += (*it)->GetAvailableWaresForTrading(good);
    }
    return amount;
}
