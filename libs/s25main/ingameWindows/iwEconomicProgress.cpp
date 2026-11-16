// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwEconomicProgress.h"
#include "EconomyModeHandler.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "Settings.h"
#include "WindowManager.h"
#include "addons/const_addons.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlText.h"
#include "controls/ctrlTextDeepening.h"
#include "iwHelp.h"
#include "network/GameClient.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldViewer.h"
#include "gameData/GoodConsts.h"
#include "gameData/const_gui_ids.h"

constexpr Extent wareIconSize(26, 26);
constexpr unsigned txtBoxWidth = 63;
constexpr Extent padding1(11, 21); // Left, Top
constexpr Extent padding2(11, 18); // Right, Bottom
constexpr unsigned teamRectHeight = 24;
constexpr unsigned extraSpacing = 2;

iwEconomicProgress::iwEconomicProgress(const GameWorldViewer& gwv)
    : IngameWindow(CGI_ECONOMICPROGRESS, IngameWindow::posLastOrCenter,
                   Extent(0, 0) /*will be resized inside the constructor*/, _("Economic progress"),
                   LOADER.getImageN("resource", 41)),
      gwv(gwv)
{
    const unsigned textcolor[] = {COLOR_GREEN, COLOR_YELLOW, COLOR_RED};
    const GameWorldBase& world = gwv.getWorld();
    const EconomyModeHandler* eH = world.getEconHandler();

    // resize window
    Extent size(padding1.x + wareIconSize.x + (std::max((int)eH->getTeams().size(), 2) + 1) * txtBoxWidth + padding2.x,
                padding1.y + teamRectHeight + ((int)eH->getGoodTypesToCollect().size() + 1) * wareIconSize.y
                  + extraSpacing * 2 + padding2.y);
    this->Resize(size);

    AddText(1, DrawPoint(padding1.x + wareIconSize.x, padding1.y + teamRectHeight), _("Player"), COLOR_GREEN,
            FontStyle::LEFT | FontStyle::BOTTOM, NormalFont);

    // determine team display order (main player team first)
    for(const auto& curTeam : eH->getTeams())
    {
        if(curTeam.containsPlayer(gwv.getPlayer().getPlayerId()))
        {
            teamOrder.push_back(&curTeam);
            break;
        }
    }
    for(const auto& curTeam : eH->getTeams())
    {
        if(&curTeam != teamOrder[0])
        {
            teamOrder.push_back(&curTeam);
        }
    }

    // the goods table
    const std::vector<GoodType>& goodsToCollect = eH->getGoodTypesToCollect();
    DrawPoint curBoxPos(padding1.x, padding1.y + teamRectHeight + extraSpacing);
    for(unsigned i = 0; i < goodsToCollect.size(); i++)
    {
        GoodType good = goodsToCollect[i];

        AddImage(100 + i, curBoxPos + wareIconSize / 2, LOADER.getMapTexture(2298), _(WARE_NAMES[good]));
        const DrawPoint warePos = curBoxPos + wareIconSize / 2;
        AddImage(200 + i, warePos, LOADER.getWareTex(good));

        DrawPoint curTxtPos = curBoxPos + DrawPoint(wareIconSize.x, 0);
        for(unsigned j = 0; j < 1 + eH->getTeams().size(); j++)
        {
            AddTextDeepening(300 + (MAX_PLAYERS + 2) * j + i, curTxtPos, Extent(txtBoxWidth, wareIconSize.y),
                             TextureColor::Grey, "?", NormalFont, textcolor[j < 2 ? j : 2]);
            curTxtPos.x += txtBoxWidth;
        }
        curBoxPos.y += wareIconSize.y;
    }

    if(!eH->isInfinite())
    {
        // game progress display
        AddText(2, DrawPoint(wareIconSize.x + extraSpacing + padding1.x, this->getSize().y - padding2.y),
                _("Time remaining:"), COLOR_ORANGE, FontStyle::LEFT | FontStyle::BOTTOM, NormalFont);
        txtRemainingTime =
          AddText(3, DrawPoint(wareIconSize.x + extraSpacing + txtBoxWidth * 3, this->getSize().y - padding2.y), "%",
                  COLOR_ORANGE, FontStyle::RIGHT | FontStyle::BOTTOM, NormalFont);
    }

    // help button
    AddImageButton(25, DrawPoint(padding1.x, this->getSize().y - wareIconSize.y - padding2.y), wareIconSize,
                   TextureColor::Grey, LOADER.getImageN("io", 225), _("Help"));
}

iwEconomicProgress::~iwEconomicProgress() = default;

void iwEconomicProgress::DrawContent()
{
    // draw team colors
    DrawPoint curTeamRectPos = getDrawPos() + DrawPoint(padding1.x + wareIconSize.x + txtBoxWidth, padding1.y);
    for(auto& team : teamOrder)
    {
        unsigned ystep = teamRectHeight / team->playersInTeam.count();
        unsigned ypos = 0;
        for(unsigned i = 0; i < gwv.getWorld().getNumPlayers(); ++i)
        {
            if(team->containsPlayer(i))
            {
                if(teamRectHeight - ypos < 2 * ystep)
                    ystep = teamRectHeight - ypos;
                DrawRectangle(Rect(curTeamRectPos + DrawPoint(0, ypos), Extent(txtBoxWidth - 1, ystep)),
                              gwv.getWorld().getPlayer(i).color);
                ypos += ystep;
            }
        }
        curTeamRectPos.x += txtBoxWidth;
    }
}

void iwEconomicProgress::msgButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        break;
        case 25: // help
        {
            WINDOWMANAGER.ReplaceWindow(
              std::make_unique<iwHelp>(_("This window shows the wares that should be collected for "
                                         "the economy mode. Displayed is the current inventory of "
                                         "the player, his team and after game end of the opponent "
                                         "teams.")));
        }
        break;
    }
}

void iwEconomicProgress::msgPaintBefore()
{
    IngameWindow::msgPaintBefore();

    const GameWorldBase& world = gwv.getWorld();
    EconomyModeHandler* eH = world.getEconHandler();
    const std::vector<EconomyModeHandler::EconTeam>& economyModeTeams = eH->getTeams();
    const GamePlayer& mainPlayer = gwv.getPlayer();

    RTTR_Assert(economyModeTeams.size() == teamOrder.size());

    // make sure the amounts are current
    eH->UpdateAmounts();

    // update table elements
    for(unsigned i = 0; i < eH->getGoodTypesToCollect().size(); i++)
    {
        for(unsigned j = 0; j < 1 + economyModeTeams.size(); j++)
        {
            auto* text = getCtrl<ctrlTextDeepening>(300 + (MAX_PLAYERS + 2) * j + i);
            if(j == 0)
            {
                text->setText(std::to_string(eH->getAmount(i, mainPlayer.getPlayerId())));
            } else if(j == 1 || eH->showAllTeamAmounts())
            {
                text->setText(std::to_string(teamOrder[j - 1]->amountsTheTeamCollected[i]));

                // White color when all teams are shown to mark good types that the team has won or is leading in
                if(eH->showAllTeamAmounts() && teamOrder[j - 1]->amountsTheTeamCollected[i] >= eH->getMaxTeamAmount(i))
                {
                    text->setTextColor(COLOR_WHITE);
                } else
                {
                    if(j == 1)
                        text->setTextColor(COLOR_YELLOW);
                    else
                        text->setTextColor(COLOR_RED);
                }
            } else
            {
                text->setText("???");
            }
        }
    }

    if(!eH->isInfinite())
    {
        // Refresh game progress tracker
        if(!eH->isOver())
        {
            txtRemainingTime->setText(GAMECLIENT.FormatGFTime(eH->getEndFrame() - world.getEvMgr().getCurrentGF()));
        } else
        {
            txtRemainingTime->setText(GAMECLIENT.FormatGFTime(0));
        }
    }
}
