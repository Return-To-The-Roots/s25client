// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#include "defines.h" // IWYU pragma: keep
#include "iwTrade.h"

#include "Loader.h"
#include "GameClient.h"
#include "buildings/nobBaseWarehouse.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlImage.h"
#include "controls/ctrlText.h"
#include "ogl/glArchivItem_Font.h"
#include "gameData/JobConsts.h"
#include "gameData/ShieldConsts.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

iwTrade::iwTrade(nobBaseWarehouse& wh)
    : IngameWindow(wh.CreateGUIID(), (unsigned short) - 2, (unsigned short) - 2, 400, 194, _("Trade"), LOADER.GetImageN("resource", 41)),
      wh(wh), possibleSrcWarehouses(GAMECLIENT.GetLocalPlayer().GetWarehousesForTrading(wh))
{
    // Get title of the player
    SetTitle(_("Trade with %s") + GAMECLIENT.GetPlayer(wh.GetPlayer()).name);
    // Gebäudebild und dessen Schatten
    AddImage( 0, 100, 144, LOADER.GetNationImage(wh.GetNation(), 250 + 5 * wh.GetBuildingType()));

    const unsigned left_column = 200;

    this->AddComboBox(4, left_column, 84, 160, 18, TC_GREY, NormalFont, 90); // Ware/Figure names
    this->AddText(1, left_column, 30, "Deal in:", COLOR_YELLOW, glArchivItem_Font::DF_LEFT, NormalFont);
    ctrlComboBox* box = this->AddComboBox(2, left_column, 44, 160, 18, TC_GREY, NormalFont, 200); // Ware or figure?
    box->AddString(_("Wares"));
    box->AddString(_("Settlers"));
    this->AddText(3, left_column, 70, "Type:", COLOR_YELLOW, glArchivItem_Font::DF_LEFT, NormalFont);


    // Create possible wares, figures
    for(unsigned i = 0; i < WARE_TYPES_COUNT; ++i)
    {
        // Only add one shield type
        if(GoodType(i) != ConvertShields(GoodType(i)))
            continue;
        // Don't add nothing or empty water
        if(i == GD_NOTHING || i == GD_WATEREMPTY)
            continue;
        wares.push_back(GoodType(i));
    }
    for(unsigned i = 0; i < JOB_TYPES_COUNT; ++i)
    {
        // Can't trade boat carriers
        if(i == JOB_BOATCARRIER)
            continue;
        jobs.push_back(Job(i));
    }

    AddImage(5, left_column + 20, 130, NULL, _("Ware you like to trade"));
    AddEdit(6, left_column + 34, 120, 39 , 20, TC_GREY, NormalFont)->SetNumberOnly(true);
    AddText(7, left_column + 75, 125, "/ 20", COLOR_YELLOW, glArchivItem_Font::DF_LEFT, NormalFont);

    AddTextButton(8, left_column, 150, 150, 22, TC_GREEN2, _("Send"), NormalFont);

    // Choose wares at first
    box->SetSelection(0);
    Msg_ComboSelectItem(2, 0);
}


void iwTrade::Msg_PaintBefore()
{
    // Schatten des Gebäudes (muss hier gezeichnet werden wegen schwarz und halbdurchsichtig)
    glArchivItem_Bitmap* bitmap = LOADER.GetNationImage(wh.GetNation(), 250 + 5 * wh.GetBuildingType() + 1);

    if(bitmap)
    {
        ctrlImage* img = GetCtrl<ctrlImage>(0);
        bitmap->Draw(img->GetX(), img->GetY(), 0, 0, 0, 0, 0, 0, COLOR_SHADOW);
    }
}

void iwTrade::Msg_PaintAfter()
{}


void iwTrade::Msg_ButtonClick(const unsigned int  /*ctrl_id*/)
{
    //pressed the send button
    unsigned short ware_figure_selection = GetCtrl<ctrlComboBox>(4)->GetSelection();
    bool ware_figure = this->GetCtrl<ctrlComboBox>(2)->GetSelection() == 1;
    GoodType gt = ware_figure ? GD_NOTHING : wares[ware_figure_selection];
    Job job = ware_figure ? jobs[ware_figure_selection] : JOB_NOTHING;

    const std::string number_str = GetCtrl<ctrlEdit>(6)->GetText();

    // Start trading
    if(!GetCtrl<ctrlComboBox>(4)->GetCtrl<ctrlList>(0)->GetVisible() && atoi(number_str.c_str()) > 0)
    {
        GAMECLIENT.TradeOverLand(wh.GetPos(), ware_figure, gt, job, atoi(number_str.c_str()));
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

            }
            else
            {
                // Add job names
                for(unsigned i = 0; i < jobs.size(); ++i)
                    names->AddString(_(JOB_NAMES[jobs[i]]));

            }
            names->SetSelection(0);
            Msg_ComboSelectItem(4, 0);

        } break;
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
            }
            else
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
        } break;
    }
}

unsigned iwTrade::GetPossibleTradeAmount(const Job job) const
{
    GameClientPlayer& player = GAMECLIENT.GetLocalPlayer();
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
    GameClientPlayer& player = GAMECLIENT.GetLocalPlayer();
    unsigned amount = 0;
    for(std::vector<nobBaseWarehouse*>::const_iterator it = possibleSrcWarehouses.begin(); it != possibleSrcWarehouses.end(); ++it)
    {
        if(player.IsWarehouseValid(*it))
            amount += (*it)->GetAvailableWaresForTrading(good);
    }
    return amount;

}
