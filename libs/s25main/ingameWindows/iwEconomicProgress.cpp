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
#include "ogl/FontStyle.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldViewer.h"
#include "gameData/const_gui_ids.h"

iwEconomicProgress::iwEconomicProgress(const GameWorldViewer& gwv)
    : IngameWindow(CGI_ECONOMICPROGRESS, IngameWindow::posLastOrCenter,
                   Extent(240, 96 + 26 * EconomyModeHandler::numGoodTypesToCollect), _("Economic Progress"),
                   LOADER.GetImageN("resource", 41)),
      gwv(gwv)
{
    const unsigned int textcolor[] = {COLOR_GREEN, COLOR_YELLOW, COLOR_RED};

    const GameWorldBase& world = gwv.GetWorld();

    EconomyModeHandler* eH = world.econHandler;

    const GamePlayer& curPlayer = gwv.GetPlayer();

    headline =
      AddText(1, DrawPoint(11 + 27, 46), _("Player"), COLOR_GREEN, FontStyle::LEFT | FontStyle::BOTTOM, NormalFont);

    const std::vector<EconomyModeHandler::econTeam>& teams = eH->GetTeams();
    unsigned int num_teams = teams.size();

    // Teamreihenfolge bestimmen (eigenes Team zuerst)
    unsigned int mainTeam;
    for(unsigned int i = 0; i < teams.size(); i++)
    {
        if(teams[i].inTeam(gwv.GetPlayer().GetPlayerId()))
        {
            mainTeam = i;
            break;
        }
    }
    teamorder.push_back(mainTeam);
    for(unsigned int i = 0; i < teams.size(); i++)
    {
        if(i != mainTeam)
        {
            teamorder.push_back(i);
        }
    }
    // Eventuell Fenster vergrößern
    if(num_teams > 2)
    {
        Extent size = this->GetSize();
        size.x += (num_teams - 2) * 63;
        this->Resize(size);
    }
    // Die Warentabelle
    const Extent btSize(26, 26);
    for(unsigned int i = 0; i < eH->numGoodTypesToCollect; i++)
    {
        GoodType good = eH->GetTypes()[i];

        const DrawPoint btPos(11, 48 + 26 * i);
        AddImage(100 + i, btPos + btSize / 2, LOADER.GetMapImageN(2298), _(WARE_NAMES[good]));
        const DrawPoint warePos = btPos + btSize / 2;
        AddImage(200 + i, warePos, LOADER.GetMapImageN(WARES_TEX_MAP_OFFSET + good));

        for(unsigned j = 0; j < 1 + num_teams; j++)
        {
            const DrawPoint txtPos = btPos + DrawPoint(26 + 63 * j, 0);
            auto* txt = static_cast<ctrlTextDeepening*>(AddTextDeepening(
              300 + 10 * j + i, txtPos, Extent(63, btSize.y), TC_GREY, "?", NormalFont, textcolor[j < 2 ? j : 2]));
        }
    }

    // Spielzeit-Fortschrittsanzeige
    AddText(2, DrawPoint(11 + 30, 240), _("Percent elapsed:"), COLOR_ORANGE, FontStyle::LEFT | FontStyle::TOP,
            NormalFont);
    elapsedTime =
      AddText(3, DrawPoint(30 + 63 * 3, 240), "%", COLOR_ORANGE, FontStyle::RIGHT | FontStyle::TOP, NormalFont);

    // Hilfe-Button
    AddImageButton(25, DrawPoint(11, 235), Extent(26, 26), TC_GREY, LOADER.GetImageN("io", 225), _("Help"));
}

iwEconomicProgress::~iwEconomicProgress() = default;

void iwEconomicProgress::Draw_()
{
    IngameWindow::Draw_();

    // Teamfarben zeichnen
    const std::vector<EconomyModeHandler::econTeam>& teams = gwv.GetWorld().econHandler->GetTeams();
    for(unsigned int t = 0; t < teams.size(); t++)
    {
        DrawPoint drawPt = GetDrawPos() + DrawPoint(37 + (t + 1) * 63, 22);
        unsigned int height = 24;
        unsigned int ystep = height / teams[teamorder[t]].num_players_in_team;
        unsigned int ypos = 0;
        for(unsigned i = 0; i < gwv.GetWorld().GetNumPlayers(); ++i)
        {
            if(teams[teamorder[t]].inTeam(i))
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
        case 25: // Hilfe
        {
            WINDOWMANAGER.ReplaceWindow(
              std::make_unique<iwHelp>(_("This window shows the wares that should be collected for "
                                         "the economy mode. Displayed is the current inventory of "
                                         "the player, his team and after game end of the opponent "
                                         "team.")));
        }
        break;
    }
}

void iwEconomicProgress::Msg_PaintBefore()
{
    IngameWindow::Msg_PaintBefore();

    const GameWorldBase& world = gwv.GetWorld();
    EconomyModeHandler* eH = world.econHandler;
    const std::vector<EconomyModeHandler::econTeam>& teams = eH->GetTeams();
    eH->UpdateAmounts(); // make sure the amounts are current
    const GamePlayer& mainPlayer = gwv.GetPlayer();

    for(unsigned int i = 0; i < eH->numGoodTypesToCollect; i++)
    {
        for(unsigned j = 0; j < 1 + teams.size(); j++)
        {
            auto* text = GetCtrl<ctrlTextDeepening>(300 + 10 * j + i);
            if(j == 0)
            {
                text->SetText(std::to_string(eH->GetAmount(i, mainPlayer.GetPlayerId())));
            } else if(j == 1 || eH->isOver())
            {
                text->SetText(std::to_string(teams[teamorder[j - 1]].teamAmounts[i]));

                // White color at game end to mark good types that the team has won
                if(eH->isOver() && teams[teamorder[j - 1]].teamAmounts[i] >= eH->GetMaxTeamAmount(i))
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

    // Refresh game progress tracker
    unsigned elapsed = 100;
    if(!eH->isOver() && eH->GetEndFrame() > 0)
    {
        elapsed = (100 * world.GetEvMgr().GetCurrentGF()) / eH->GetEndFrame();
    }
    elapsedTime->SetText(std::to_string(elapsed) + "%");
}
