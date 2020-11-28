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
#include "gameData/const_gui_ids.h"

iwEconomicProgress::iwEconomicProgress(const GameWorldViewer& gwv)
    : IngameWindow(CGI_ECONOMICPROGRESS, IngameWindow::posLastOrCenter, Extent(240, 96 + 26 * 7),
                   _("Economic Progress"), LOADER.GetImageN("resource", 41)),
      gwv(gwv)
{
    const unsigned int textcolor[] = {COLOR_GREEN, COLOR_YELLOW, COLOR_RED};

    const GameWorldBase& world = gwv.GetWorld();

    EconomyModeHandler* eH = world.econHandler.get();

    const unsigned int numGoodTypesToCollect = eH->GetGoodTypesToCollect().size();

    AddText(1, DrawPoint(11 + 27, 46), _("Player"), COLOR_GREEN, FontStyle::LEFT | FontStyle::BOTTOM, NormalFont);

    const std::vector<EconomyModeHandler::EconTeam>& economyModeTeams = eH->GetTeams();
    unsigned int num_teams = economyModeTeams.size();

    // determine team display order (main player team first)
    unsigned int mainTeam = 0;
    for(unsigned int i = 0; i < economyModeTeams.size(); i++)
    {
        if(economyModeTeams[i].inTeam(gwv.GetPlayer().GetPlayerId()))
        {
            mainTeam = i;
            break;
        }
    }
    teamOrder.push_back(mainTeam);
    for(unsigned int i = 0; i < economyModeTeams.size(); i++)
    {
        if(i != mainTeam)
        {
            teamOrder.push_back(i);
        }
    }
    // potentially resize window
    if(num_teams > 2 || numGoodTypesToCollect != 7)
    {
        Extent size = this->GetSize();
        size.x += (std::max((int)num_teams, 2) - 2) * 63;
        size.y += ((int)numGoodTypesToCollect - 7) * 26;
        this->Resize(size);
    }
    // the goods table
    const Extent btSize(26, 26);
    auto& goodTypes = eH->GetGoodTypesToCollect();
    for(unsigned int i = 0; i < goodTypes.size(); i++)
    {
        GoodType good = eH->GetGoodTypesToCollect()[i];

        const DrawPoint btPos(11, 48 + 26 * i);
        AddImage(100 + i, btPos + btSize / 2, LOADER.GetMapImageN(2298), _(WARE_NAMES[good]));
        const DrawPoint warePos = btPos + btSize / 2;
        AddImage(200 + i, warePos, LOADER.GetMapImageN(WARES_TEX_MAP_OFFSET + good));

        for(unsigned j = 0; j < 1 + num_teams; j++)
        {
            const DrawPoint txtPos = btPos + DrawPoint(26 + 63 * j, 0);
            AddTextDeepening(300 + 10 * j + i, txtPos, Extent(63, btSize.y), TC_GREY, "?", NormalFont,
                             textcolor[j < 2 ? j : 2]);
        }
    }

    if(!eH->isInfinite())
    {
        // game progress display
        AddText(2, DrawPoint(11 + 30, 240), _("Time remaining:"), COLOR_ORANGE, FontStyle::LEFT | FontStyle::TOP,
                NormalFont);
        txtRemainingTime =
          AddText(3, DrawPoint(30 + 63 * 3, 240), "%", COLOR_ORANGE, FontStyle::RIGHT | FontStyle::TOP, NormalFont);
    }

    // help button
    AddImageButton(25, DrawPoint(11, 235), Extent(26, 26), TC_GREY, LOADER.GetImageN("io", 225), _("Help"));
}

iwEconomicProgress::~iwEconomicProgress() = default;

void iwEconomicProgress::Draw_()
{
    IngameWindow::Draw_();

    // draw team colors
    const std::vector<EconomyModeHandler::EconTeam>& economyModeTeams = gwv.GetWorld().econHandler->GetTeams();
    for(unsigned int t = 0; t < economyModeTeams.size(); t++)
    {
        DrawPoint drawPt = GetDrawPos() + DrawPoint(37 + (t + 1) * 63, 22);
        unsigned int height = 24;
        unsigned int ystep = height / economyModeTeams[teamOrder[t]].playersInTeam.count();
        unsigned int ypos = 0;
        for(unsigned i = 0; i < gwv.GetWorld().GetNumPlayers(); ++i)
        {
            if(economyModeTeams[teamOrder[t]].inTeam(i))
            {
                if(height - ypos < 2 * ystep)
                    ystep = height - ypos;
                DrawRectangle(Rect(drawPt + DrawPoint(0, ypos), Extent(62, ystep)), gwv.GetWorld().GetPlayer(i).color);
                ypos += ystep;
            }
        }
    }
}

void iwEconomicProgress::Msg_ButtonClick(const unsigned ctrl_id)
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

void iwEconomicProgress::Msg_PaintBefore()
{
    IngameWindow::Msg_PaintBefore();

    const GameWorldBase& world = gwv.GetWorld();
    EconomyModeHandler* eH = world.econHandler.get();
    const std::vector<EconomyModeHandler::EconTeam>& economyModeTeams = eH->GetTeams();
    const GamePlayer& mainPlayer = gwv.GetPlayer();

    // make sure the amounts are current
    eH->UpdateAmounts();

    // update table elements
    for(unsigned int i = 0; i < eH->GetGoodTypesToCollect().size(); i++)
    {
        for(unsigned j = 0; j < 1 + economyModeTeams.size(); j++)
        {
            auto* text = GetCtrl<ctrlTextDeepening>(300 + 10 * j + i);
            if(j == 0)
            {
                text->SetText(std::to_string(eH->GetAmount(i, mainPlayer.GetPlayerId())));
            } else if(j == 1 || eH->showAllTeamAmounts())
            {
                text->SetText(std::to_string(economyModeTeams[teamOrder[j - 1]].amountsTheTeamCollected[i]));

                // White color when all teams are shown to mark good types that the team has won or is leading in
                if(eH->showAllTeamAmounts()
                   && economyModeTeams[teamOrder[j - 1]].amountsTheTeamCollected[i] >= eH->GetMaxTeamAmount(i))
                {
                    text->SetTextColor(COLOR_WHITE);
                } else
                {
                    if(j == 1)
                        text->SetTextColor(COLOR_YELLOW);
                    else
                        text->SetTextColor(COLOR_RED);
                }
            } else
            {
                text->SetText("???");
            }
        }
    }

    if(!eH->isInfinite())
    {
        // Refresh game progress tracker
        if(!eH->isOver())
        {
            txtRemainingTime->SetText(GAMECLIENT.FormatGFTime(eH->GetEndFrame() - world.GetEvMgr().GetCurrentGF()));
        } else
        {
            txtRemainingTime->SetText(_("0"));
        }
    }
}
