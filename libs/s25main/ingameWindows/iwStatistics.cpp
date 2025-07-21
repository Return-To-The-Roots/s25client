// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwStatistics.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "Settings.h"
#include "WindowManager.h"
#include "addons/const_addons.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlText.h"
#include "helpers/Range.h"
#include "iwHelp.h"
#include "network/GameClient.h"
#include "ogl/FontStyle.h"
#include "ogl/glFont.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldViewer.h"
#include "gameData/PortraitConsts.h"
#include "gameData/const_gui_ids.h"

namespace {

/// Size of the diagram area
constexpr Extent diagramSize(180, 80);
/// Starting point of the diagram (relative to window position)
constexpr DrawPoint topLeftRel(37, 124);

/// (Horizontal) center position of all player portrait buttons
constexpr DrawPoint playerButtonsCenterPos(126, 22);
constexpr Extent playerBtSize(34, 47);

std::string getPlayerStatus(const GamePlayer& player)
{
    if(player.IsDefeated())
        return "---";
    else if(player.isHuman())
        return "#" + std::to_string(player.GetPlayerId() + 1);
    else
        return _("COMP");
}

enum
{
    ID_btPlayer_Start,
    ID_imgStatistic = ID_btPlayer_Start + MAX_PLAYERS,
    ID_grpStatType,
    ID_btTypeSize,
    ID_btTypeBuildings,
    ID_btTypeInhabitants,
    ID_btTypeMerchandise,
    ID_btTypeMilitary,
    ID_btTypeGold,
    ID_btTypeProductivity,
    ID_btTypeVanquishedEnemies,
    ID_grpTime,
    ID_time15m,
    ID_time1h,
    ID_time4h,
    ID_time16h,
    ID_txtHeader,
    ID_txtXAxisValuesStart,
    ID_txtMaxY = ID_txtXAxisValuesStart + iwStatistics::MAX_TIME_LABELS,
    ID_txtMinY,
    ID_btHelp,
};

} // namespace

iwStatistics::iwStatistics(const GameWorldViewer& gwv)
    : IngameWindow(CGI_STATISTICS, IngameWindow::posLastOrCenter, Extent(252, 336), _("Statistics"),
                   LOADER.GetImageN("resource", 41)),
      gwv(gwv), showStatistic(MAX_PLAYERS)
{
    // Count active players
    numPlayingPlayers = 0;
    const GameWorldBase& world = gwv.GetWorld();
    for(const auto i : helpers::range(world.GetNumPlayers()))
    {
        if(world.GetPlayer(i).isUsed())
            numPlayingPlayers++;
    }

    DrawPoint curBtPos = playerButtonsCenterPos - DrawPoint(numPlayingPlayers * playerBtSize.x / 2, 0);

    for(const auto i : helpers::range(world.GetNumPlayers()))
    {
        // Ignore unused players
        const GamePlayer& curPlayer = world.GetPlayer(i);
        if(!curPlayer.isUsed())
            continue;

        // Set visibility based on addon setting
        switch(GAMECLIENT.IsReplayModeOn() ? 0 : world.GetGGS().getSelection(AddonId::STATISTICS_VISIBILITY))
        {
            default:
                RTTR_Assert_Msg(false, "Invalid Addon Setting");
                showStatistic[i] = false;
                break;
            case 0: // Visible for all
                showStatistic[i] = true;
                break;
            case 1: // Only for allies
                showStatistic[i] = gwv.GetPlayer().IsAlly(i);
                break;
            case 2: // Only own statistic
                showStatistic[i] = (gwv.GetPlayerId() == i);
                break;
        }

        RTTR_Assert(curPlayer.portraitIndex < Portraits.size());
        const auto& portrait = Portraits[curPlayer.portraitIndex];
        auto* btPortrait =
          AddImageButton(ID_btPlayer_Start + i, curBtPos, playerBtSize, TextureColor::Green1,
                         LOADER.GetImageN(portrait.resourceId, portrait.resourceIndex), curPlayer.name);
        curBtPos.x += playerBtSize.x;
        btPortrait->SetBorder(false);
        btPortrait->SetEnabled(showStatistic[i]);
    }

    AddImage(ID_imgStatistic, DrawPoint(126, 165), LOADER.GetImageN("io", 228));

    // Buttons for changing the statistic type shown
    ctrlOptionGroup* statChanger = AddOptionGroup(ID_grpStatType, GroupSelectType::Illuminate);
    statChanger->AddImageButton(ID_btTypeSize, DrawPoint(18, 250), Extent(26, 30), TextureColor::Grey,
                                LOADER.GetImageN("io", 167), _("Size of country"));
    statChanger->AddImageButton(ID_btTypeBuildings, DrawPoint(45, 250), Extent(26, 30), TextureColor::Grey,
                                LOADER.GetImageN("io", 168), _("Buildings"));
    statChanger->AddImageButton(ID_btTypeInhabitants, DrawPoint(72, 250), Extent(26, 30), TextureColor::Grey,
                                LOADER.GetImageN("io", 169), _("Inhabitants"));
    statChanger->AddImageButton(ID_btTypeMerchandise, DrawPoint(99, 250), Extent(26, 30), TextureColor::Grey,
                                LOADER.GetImageN("io", 170), _("Merchandise"));
    statChanger->AddImageButton(ID_btTypeMilitary, DrawPoint(126, 250), Extent(26, 30), TextureColor::Grey,
                                LOADER.GetImageN("io", 171), _("Military strength"));
    statChanger->AddImageButton(ID_btTypeGold, DrawPoint(153, 250), Extent(26, 30), TextureColor::Grey,
                                LOADER.GetImageN("io", 172), _("Gold"));
    statChanger->AddImageButton(ID_btTypeProductivity, DrawPoint(180, 250), Extent(26, 30), TextureColor::Grey,
                                LOADER.GetImageN("io", 173), _("Productivity"));
    statChanger->AddImageButton(ID_btTypeVanquishedEnemies, DrawPoint(207, 250), Extent(26, 30), TextureColor::Grey,
                                LOADER.GetImageN("io", 217), _("Vanquished enemies"));

    // Buttons for duration
    constexpr Extent timeBtSize(43, 28);
    ctrlOptionGroup* timeChanger = AddOptionGroup(ID_grpTime, GroupSelectType::Illuminate);
    timeChanger->AddTextButton(ID_time15m, DrawPoint(51, 288), timeBtSize, TextureColor::Grey, _("15 m"), NormalFont);
    timeChanger->AddTextButton(ID_time1h, DrawPoint(96, 288), timeBtSize, TextureColor::Grey, _("1 h"), NormalFont);
    timeChanger->AddTextButton(ID_time4h, DrawPoint(141, 288), timeBtSize, TextureColor::Grey, _("4 h"), NormalFont);
    timeChanger->AddTextButton(ID_time16h, DrawPoint(186, 288), timeBtSize, TextureColor::Grey, _("16 h"), NormalFont);

    AddImageButton(ID_btHelp, DrawPoint(18, 288), Extent(30, 32), TextureColor::Grey, LOADER.GetImageN("io", 225),
                   _("Help"));

    const auto labelColor = MakeColor(255, 136, 96, 52);
    // Current header
    headline = AddText(ID_txtHeader, DrawPoint(130, 120), _("Size of country"), labelColor,
                       FontStyle::CENTER | FontStyle::BOTTOM | FontStyle::NO_OUTLINE, NormalFont);

    // Current maximum of y-axis
    txtMaxValueY = AddText(ID_txtMaxY, DrawPoint(211, 125), "1", labelColor,
                           FontStyle::RIGHT | FontStyle::VCENTER | FontStyle::NO_OUTLINE, NormalFont);

    // Current minimum of y-axis
    txtMinValueY = AddText(ID_txtMinY, DrawPoint(211, 200), "0", labelColor,
                           FontStyle::RIGHT | FontStyle::VCENTER | FontStyle::NO_OUTLINE, NormalFont);

    // Time values on the x-axis
    for(unsigned i = 0; i < timeAnnotations.size(); ++i)
    {
        timeAnnotations[i] = AddText(ID_txtXAxisValuesStart + i, DrawPoint(211 + i, 125 + i), "", labelColor,
                                     FontStyle::CENTER | FontStyle::TOP | FontStyle::NO_OUTLINE, NormalFont);
    }

    // Default values:
    statChanger->SetSelection(ID_btTypeSize);
    currentView = StatisticType::Country;
    timeChanger->SetSelection(ID_time15m, true);
    currentTime = StatisticTime::T15Minutes;

    if(!SETTINGS.ingame.scale_statistics)
        txtMinValueY->SetVisible(false);
}

iwStatistics::~iwStatistics() = default;

void iwStatistics::Msg_ButtonClick(const unsigned ctrl_id)
{
    const unsigned playerIndex = ctrl_id - ID_btPlayer_Start;
    if(playerIndex < showStatistic.size())
        showStatistic[playerIndex] = !showStatistic[playerIndex];
    else if(ctrl_id == ID_btHelp)
    {
        WINDOWMANAGER.ReplaceWindow(
          std::make_unique<iwHelp>(_("This window allows a direct comparison with the enemies. "
                                     "Factors such as the wealth, territorial area, inhabitants, "
                                     "etc. of all parties can be compared. This data can be shown "
                                     "over four different time periods.")));
    }
}

void iwStatistics::Msg_OptionGroupChange(const unsigned ctrl_id, const unsigned selection)
{
    switch(ctrl_id)
    {
        case ID_grpStatType:
            switch(selection)
            {
                case ID_btTypeSize:
                    currentView = StatisticType::Country;
                    headline->SetText(_("Size of country"));
                    break;
                case ID_btTypeBuildings:
                    currentView = StatisticType::Buildings;
                    headline->SetText(_("Buildings"));
                    break;
                case ID_btTypeInhabitants:
                    currentView = StatisticType::Inhabitants;
                    headline->SetText(_("Inhabitants"));
                    break;
                case ID_btTypeMerchandise:
                    currentView = StatisticType::Merchandise;
                    headline->SetText(_("Merchandise"));
                    break;
                case ID_btTypeMilitary:
                    currentView = StatisticType::Military;
                    headline->SetText(_("Military strength"));
                    break;
                case ID_btTypeGold:
                    currentView = StatisticType::Gold;
                    headline->SetText(_("Gold"));
                    break;
                case ID_btTypeProductivity:
                    currentView = StatisticType::Productivity;
                    headline->SetText(_("Productivity"));
                    break;
                case ID_btTypeVanquishedEnemies:
                    currentView = StatisticType::Vanquished;
                    headline->SetText(_("Vanquished enemies"));
                    break;
            }
            break;
        case ID_grpTime:
            switch(selection)
            {
                case ID_time15m: currentTime = StatisticTime::T15Minutes; break;
                case ID_time1h: currentTime = StatisticTime::T1Hour; break;
                case ID_time4h: currentTime = StatisticTime::T4Hours; break;
                case ID_time16h: currentTime = StatisticTime::T16Hours; break;
            }
            updateTimeAxisLabels();
            break;
    }
}

void iwStatistics::DrawContent()
{
    // Draw the alliances and the colored boxes under the portraits
    DrawPlayerOverlays();

    DrawAxis();

    // Draw lines making up the statistic
    DrawStatistic(currentView);
}

void iwStatistics::DrawPlayerOverlays()
{
    DrawPoint drawPt = playerButtonsCenterPos - DrawPoint(numPlayingPlayers * playerBtSize.x / 2, 0);

    for(const auto i : helpers::range(gwv.GetWorld().GetNumPlayers()))
    {
        const GamePlayer& player = gwv.GetWorld().GetPlayer(i);
        if(!player.isUsed())
            continue;

        if(showStatistic[i])
        {
            // Draw the alliances at top of the player image
            DrawPlayerAlliances(drawPt, player);
            // Draw player boxes and player status at bottom
            DrawPlayerBox(drawPt, player);
        }
        drawPt.x += playerBtSize.x;
    }
}

void iwStatistics::DrawPlayerBox(DrawPoint const& drawPt, const GamePlayer& player)
{
    auto const playerBoxRect = Rect(DrawPoint(drawPt.x, drawPt.y + 47), Extent(34, 12));
    auto const playerStatusPosition =
      DrawPoint(playerBoxRect.getOrigin() + playerBoxRect.getSize() / 2 + DrawPoint(0, 1));
    DrawRectangle(playerBoxRect, player.color);
    SmallFont->Draw(playerStatusPosition, getPlayerStatus(player), FontStyle::CENTER | FontStyle::VCENTER,
                    COLOR_YELLOW);
}

void iwStatistics::DrawPlayerAlliances(DrawPoint const& drawPt, const GamePlayer& player)
{
    constexpr unsigned spacingAllianceRects = 1;
    constexpr Extent allianceRectExtent(4, 6);
    constexpr unsigned allianceRectOffset = allianceRectExtent.x + spacingAllianceRects;

    auto pactPos = drawPt + DrawPoint::all(spacingAllianceRects);
    for(auto idxOther : helpers::range(gwv.GetWorld().GetNumPlayers()))
    {
        if(idxOther != player.GetPlayerId() && player.IsAlly(idxOther))
        {
            DrawRectangle(Rect(pactPos, allianceRectExtent), gwv.GetWorld().GetPlayer(idxOther).color);
            pactPos.x += allianceRectOffset;
        }
    }
}

void iwStatistics::DrawStatistic(StatisticType type)
{
    constexpr int stepX = diagramSize.x / NUM_STAT_STEPS;

    unsigned currentIndex;
    unsigned max = 1;
    unsigned min = unsigned(-1);

    // Find min and max value
    const GameWorldBase& world = gwv.GetWorld();
    for(const auto p : helpers::range(world.GetNumPlayers()))
    {
        if(!showStatistic[p])
            continue;
        const GamePlayer::Statistic& stat = world.GetPlayer(p).GetStatistic(currentTime);

        currentIndex = stat.currentIndex;
        for(const auto i : helpers::range(NUM_STAT_STEPS))
        {
            const auto idx = (currentIndex >= i) ? (currentIndex - i) : (NUM_STAT_STEPS - i + currentIndex);
            max = std::max(max, stat.data[type][idx]);
            if(SETTINGS.ingame.scale_statistics) //-V807
                min = std::min(min, stat.data[type][idx]);
        }
    }

    if(max == min)
    {
        --min;
        ++max;
    }
    // Add values to axis
    txtMaxValueY->SetText(std::to_string(max));
    if(SETTINGS.ingame.scale_statistics)
        txtMinValueY->SetText(std::to_string(min));

    // Draw the line diagram
    const DrawPoint topLeft = GetPos() + topLeftRel;
    DrawPoint previousPos(0, 0);

    for(const auto p : helpers::range(world.GetNumPlayers()))
    {
        if(!showStatistic[p])
            continue;
        const GamePlayer::Statistic& stat = world.GetPlayer(p).GetStatistic(currentTime);

        currentIndex = stat.currentIndex;
        for(unsigned i = 0; i < NUM_STAT_STEPS; ++i)
        {
            DrawPoint curPos = topLeft + DrawPoint((NUM_STAT_STEPS - i) * stepX, diagramSize.y);
            unsigned curStatVal =
              stat.data[type][(currentIndex >= i) ? (currentIndex - i) : (NUM_STAT_STEPS - i + currentIndex)];
            if(SETTINGS.ingame.scale_statistics)
                curPos.y -= ((curStatVal - min) * diagramSize.y) / (max - min);
            else
                curPos.y -= (curStatVal * diagramSize.y) / max;
            if(i != 0)
                DrawLine(curPos, previousPos, 2, world.GetPlayer(p).color);
            previousPos = curPos;
        }
    }
}

void iwStatistics::DrawAxis()
{
    const DrawPoint topLeft = GetPos() + topLeftRel;
    const auto axisColor = MakeColor(255, 88, 44, 16);

    // X-axis, horizontal
    DrawLine(topLeft + DrawPoint(6, diagramSize.y + 2), // slightly lower to see zero line and start a bit to the right
             topLeft + diagramSize + DrawPoint(0, 2), 1, axisColor);

    // Y-axis, vertical
    DrawLine(topLeft + DrawPoint(diagramSize.x, 0), topLeft + diagramSize + DrawPoint(0, 5), 1, axisColor);

    // Marks on y-axis
    DrawLine(topLeft + DrawPoint(diagramSize.x - 3, 0), topLeft + DrawPoint(diagramSize.x + 4, 0), 1, axisColor);
    DrawLine(topLeft + DrawPoint(diagramSize.x - 3, diagramSize.y / 2),
             topLeft + DrawPoint(diagramSize.x + 4, diagramSize.y / 2), 1, axisColor);

    // Marks on x-axis
    for(const auto* lbl : timeAnnotations)
    {
        if(lbl->IsVisible())
        {
            const auto lblPos = lbl->GetDrawPos();
            DrawLine(lblPos - DrawPoint(0, 4), lblPos - DrawPoint(0, 2), 1, axisColor);
        }
    }
}

void iwStatistics::updateTimeAxisLabels()
{
    // Labels on x-axis

    int numMarks = 0;
    switch(currentTime)
    {
        case StatisticTime::T15Minutes:
            timeAnnotations[1]->SetText("-15");
            timeAnnotations[2]->SetText("-12");
            timeAnnotations[3]->SetText("-9");
            timeAnnotations[4]->SetText("-6");
            timeAnnotations[5]->SetText("-3");
            numMarks = 6;
            break;
        case StatisticTime::T1Hour:
            timeAnnotations[1]->SetText("-60");
            timeAnnotations[2]->SetText("-50");
            timeAnnotations[3]->SetText("-40");
            timeAnnotations[4]->SetText("-30");
            timeAnnotations[5]->SetText("-20");
            timeAnnotations[6]->SetText("-10");
            numMarks = 7;
            break;
        case StatisticTime::T4Hours:
            timeAnnotations[1]->SetText("-240");
            timeAnnotations[2]->SetText("-180");
            timeAnnotations[3]->SetText("-120");
            timeAnnotations[4]->SetText("-60");
            numMarks = 5;
            break;
        case StatisticTime::T16Hours:
            timeAnnotations[1]->SetText("-960");
            timeAnnotations[2]->SetText("-720");
            timeAnnotations[3]->SetText("-480");
            timeAnnotations[4]->SetText("-240");
            numMarks = 5;
            break;
    }
    constexpr DrawPoint offset(6, 6);
    DrawPoint curPos = topLeftRel + offset + DrawPoint(0, diagramSize.y);
    for(const auto i : helpers::range(1, numMarks))
    {
        timeAnnotations[i]->SetPos(curPos);
        timeAnnotations[i]->SetVisible(true);
        curPos.x += (diagramSize.x - offset.x) / (numMarks - 1);
    }
    for(const auto i : helpers::range<unsigned>(numMarks, timeAnnotations.size()))
        timeAnnotations[i]->SetVisible(false);

    // Zero is used for all time periods
    timeAnnotations[0]->SetPos(topLeftRel + diagramSize + DrawPoint(0, offset.y)); // Exactly at the end of the x-axis
    timeAnnotations[0]->SetText("0");
}
