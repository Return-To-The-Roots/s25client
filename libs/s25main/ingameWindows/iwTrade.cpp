// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwTrade.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "buildings/nobBaseWarehouse.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlImage.h"
#include "controls/ctrlText.h"
#include "factories/GameCommandFactory.h"
#include "helpers/strUtils.h"
#include "helpers/toString.h"
#include "ogl/FontStyle.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "variant.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldViewer.h"
#include "gameData/GoodConsts.h"
#include "gameData/JobConsts.h"
#include "gameData/ShieldConsts.h"
#include "gameData/const_gui_ids.h"

iwTrade::iwTrade(const nobBaseWarehouse& wh, const GameWorldViewer& gwv, GameCommandFactory& gcFactory)
    : IngameWindow(CGI_BUILDING + MapBase::createGUIID(wh.getPos()), IngameWindow::posAtMouse, Extent(400, 194),
                   _("Trade"), LOADER.getImageN("resource", 41)),
      wh(wh), gwv(gwv), gcFactory(gcFactory), possibleSrcWarehouses(gwv.getPlayer().getWarehousesForTrading(wh))
{
    // Get title of the player
    setTitle((boost::format(_("Trade with %s")) % gwv.getWorld().getPlayer(wh.getPlayer()).name).str());
    // Gebäudebild und dessen Schatten
    addImage(0, DrawPoint(100, 144), &wh.getBuildingImage());

    const unsigned left_column = 200;

    addComboBox(4, DrawPoint(left_column, 84), Extent(160, 18), TextureColor::Grey, NormalFont,
                90); // Ware/Figure names
    addText(1, DrawPoint(left_column, 30), "Deal in:", COLOR_YELLOW, FontStyle::LEFT, NormalFont);
    ctrlComboBox* box = this->addComboBox(2, DrawPoint(left_column, 44), Extent(160, 18), TextureColor::Grey,
                                          NormalFont, 200); // Ware or figure?
    box->addItem(_("Wares"));
    box->addItem(_("Settlers"));
    addText(3, DrawPoint(left_column, 70), "Type:", COLOR_YELLOW, FontStyle::LEFT, NormalFont);

    // Create possible wares, figures
    for(const auto i : helpers::enumRange<GoodType>())
    {
        // Only add one shield type
        if(i != ConvertShields(i))
            continue;
        // Don't add empty water
        if(i == GoodType::WaterEmpty)
            continue;
        wares.push_back(i);
    }
    for(const auto i : helpers::enumRange<Job>())
    {
        // Can't trade boat carriers
        if(i == Job::BoatCarrier)
            continue;
        jobs.push_back(i);
    }

    addImage(5, DrawPoint(left_column + 20, 130), static_cast<ITexture*>(nullptr), _("Ware you like to trade"));
    addEdit(6, DrawPoint(left_column + 34, 120), Extent(39, 20), TextureColor::Grey, NormalFont)
      ->setType(EditType::Number);
    addText(7, DrawPoint(left_column + 75, 125), "/ 20", COLOR_YELLOW, FontStyle::LEFT, NormalFont);

    addTextButton(8, DrawPoint(left_column, 150), Extent(150, 22), TextureColor::Green2, _("Send"), NormalFont);

    // Choose wares at first
    box->setSelection(0);
    msgComboSelectItem(2, 0);
}

void iwTrade::msgButtonClick(const unsigned /*ctrl_id*/)
{
    // pressed the send button
    const unsigned ware_figure_selection = getCtrl<ctrlComboBox>(4)->getSelection().value();
    const bool isJob = this->getCtrl<ctrlComboBox>(2)->getSelection() == 1u;
    boost_variant2<GoodType, Job> what;
    if(isJob)
        what = jobs[ware_figure_selection];
    else
        what = wares[ware_figure_selection];

    const std::string number_str = getCtrl<ctrlEdit>(6)->getText();

    // Start trading
    if(!getCtrl<ctrlComboBox>(4)->getCtrl<ctrlList>(0)->isVisible() && helpers::fromString(number_str, 0) > 0)
    {
        if(gcFactory.TradeOverLand(wh.getPos(), what, helpers::fromString(number_str, 0)))
            this->close();
    }
}

void iwTrade::msgComboSelectItem(const unsigned ctrl_id, const unsigned selection)
{
    switch(ctrl_id)
    {
        // Change ware/figure mode
        case 2:
        {
            auto* names = this->getCtrl<ctrlComboBox>(4);
            names->deleteAllItems();
            if(selection == 0)
            {
                // Add ware names
                for(GoodType ware : wares)
                    names->addItem(_(WARE_NAMES[ware]));

            } else
            {
                // Add job names
                for(Job job : jobs)
                    names->addItem(_(JOB_NAMES[job]));
            }
            names->setSelection(0);
            msgComboSelectItem(4, 0);
        }
        break;
        case 4:
        {
            unsigned number;
            if(this->getCtrl<ctrlComboBox>(2)->getSelection() == 0u)
            {
                // Wares

                // Set the new image of the ware which was selected
                getCtrl<ctrlImage>(5)->setImage(LOADER.getWareTex(wares[selection]));

                // Get the number of available wares
                number = getPossibleTradeAmount(wares[selection]);
            } else
            {
                getCtrl<ctrlImage>(5)->setImage(LOADER.getJobTex(jobs[selection]));

                // Get the number of available figures
                number = getPossibleTradeAmount(jobs[selection]);
            }

            getCtrl<ctrlText>(7)->setText("/ " + helpers::toString(number));
        }
        break;
    }
}

unsigned iwTrade::getPossibleTradeAmount(const Job job) const
{
    const GamePlayer& player = gwv.getPlayer();
    unsigned amount = 0;
    for(auto* possibleSrcWarehouse : possibleSrcWarehouses)
    {
        if(player.isWarehouseValid(possibleSrcWarehouse))
            amount += possibleSrcWarehouse->getAvailableFiguresForTrading(job);
    }
    return amount;
}

unsigned iwTrade::getPossibleTradeAmount(const GoodType good) const
{
    const GamePlayer& player = gwv.getPlayer();
    unsigned amount = 0;
    for(auto* possibleSrcWarehouse : possibleSrcWarehouses)
    {
        if(player.isWarehouseValid(possibleSrcWarehouse))
            amount += possibleSrcWarehouse->getAvailableWaresForTrading(good);
    }
    return amount;
}
