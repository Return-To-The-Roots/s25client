// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
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
#include <limits>

namespace {

/// Size of the diagram area
constexpr Extent diagramSize(180, 80);
/// Starting point of the diagram (relative to window position)
constexpr DrawPoint topLeftRel(37, 124);

/// (Horizontal) center position of all player portrait buttons
constexpr DrawPoint playerButtonsCenterPos(126, 22);
constexpr Extent playerBtSize(34, 47);

/// Distance between statistic values on the x-axis
constexpr int stepX = diagramSize.x / NUM_STAT_STEPS;

std::string getPlayerStatus(const GamePlayer& player)
{
    if(player.IsDefeated())
        return "---";
    else if(player.isHuman())
        return "#" + std::to_string(player.getPlayerId() + 1);
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
                   LOADER.getImageN("resource", 41)),
      gwv(gwv), showStatistic(MAX_PLAYERS)
{
    // Count active players
    numPlayingPlayers = 0;
    const GameWorldBase& world = gwv.getWorld();
    for(const auto& player : world.getPlayers())
    {
        if(player.isUsed())
            numPlayingPlayers++;
    }

    DrawPoint curBtPos = playerButtonsCenterPos - DrawPoint(numPlayingPlayers * playerBtSize.x / 2, 0);

    for(const auto i : helpers::range(world.getNumPlayers()))
    {
        // Ignore unused players
        const GamePlayer& curPlayer = world.getPlayer(i);
        if(!curPlayer.isUsed())
            continue;

        // Set visibility based on addon setting
        switch(GAMECLIENT.isReplayModeOn() ? 0 : world.getGGS().getSelection(AddonId::STATISTICS_VISIBILITY))
        {
            default:
                RTTR_Assert_Msg(false, "Invalid Addon Setting");
                showStatistic[i] = false;
                break;
            case 0: // Visible for all
                showStatistic[i] = true;
                break;
            case 1: // Only for allies
                showStatistic[i] = gwv.getPlayer().isAlly(i);
                break;
            case 2: // Only own statistic
                showStatistic[i] = (gwv.getPlayerId() == i);
                break;
        }

        RTTR_Assert(curPlayer.portraitIndex < Portraits.size());
        const auto& portrait = Portraits[curPlayer.portraitIndex];
        auto* btPortrait =
          addImageButton(ID_btPlayer_Start + i, curBtPos, playerBtSize, TextureColor::Green1,
                         LOADER.getImageN(portrait.resourceId, portrait.resourceIndex), curPlayer.name);
        curBtPos.x += playerBtSize.x;
        btPortrait->setBorder(false);
        btPortrait->setEnabled(showStatistic[i]);
    }

    addImage(ID_imgStatistic, DrawPoint(126, 165), LOADER.getImageN("io", 228));

    // Buttons for changing the statistic type shown
    ctrlOptionGroup* statChanger = addOptionGroup(ID_grpStatType, GroupSelectType::Illuminate);
    statChanger->addImageButton(ID_btTypeSize, DrawPoint(18, 250), Extent(26, 30), TextureColor::Grey,
                                LOADER.getImageN("io", 167), _("Size of country"));
    statChanger->addImageButton(ID_btTypeBuildings, DrawPoint(45, 250), Extent(26, 30), TextureColor::Grey,
                                LOADER.getImageN("io", 168), _("Buildings"));
    statChanger->addImageButton(ID_btTypeInhabitants, DrawPoint(72, 250), Extent(26, 30), TextureColor::Grey,
                                LOADER.getImageN("io", 169), _("Inhabitants"));
    statChanger->addImageButton(ID_btTypeMerchandise, DrawPoint(99, 250), Extent(26, 30), TextureColor::Grey,
                                LOADER.getImageN("io", 170), _("Merchandise"));
    statChanger->addImageButton(ID_btTypeMilitary, DrawPoint(126, 250), Extent(26, 30), TextureColor::Grey,
                                LOADER.getImageN("io", 171), _("Military strength"));
    statChanger->addImageButton(ID_btTypeGold, DrawPoint(153, 250), Extent(26, 30), TextureColor::Grey,
                                LOADER.getImageN("io", 172), _("Gold"));
    statChanger->addImageButton(ID_btTypeProductivity, DrawPoint(180, 250), Extent(26, 30), TextureColor::Grey,
                                LOADER.getImageN("io", 173), _("Productivity"));
    statChanger->addImageButton(ID_btTypeVanquishedEnemies, DrawPoint(207, 250), Extent(26, 30), TextureColor::Grey,
                                LOADER.getImageN("io", 217), _("Vanquished enemies"));

    // Buttons for duration
    constexpr Extent timeBtSize(43, 28);
    ctrlOptionGroup* timeChanger = addOptionGroup(ID_grpTime, GroupSelectType::Illuminate);
    timeChanger->addTextButton(ID_time15m, DrawPoint(51, 288), timeBtSize, TextureColor::Grey, _("15 m"), NormalFont);
    timeChanger->addTextButton(ID_time1h, DrawPoint(96, 288), timeBtSize, TextureColor::Grey, _("1 h"), NormalFont);
    timeChanger->addTextButton(ID_time4h, DrawPoint(141, 288), timeBtSize, TextureColor::Grey, _("4 h"), NormalFont);
    timeChanger->addTextButton(ID_time16h, DrawPoint(186, 288), timeBtSize, TextureColor::Grey, _("16 h"), NormalFont);

    addImageButton(ID_btHelp, DrawPoint(18, 288), Extent(30, 32), TextureColor::Grey, LOADER.getImageN("io", 225),
                   _("Help"));

    const auto labelColor = MakeColor(255, 136, 96, 52);
    // Current header
    headline = addText(ID_txtHeader, DrawPoint(130, 120), _("Size of country"), labelColor,
                       FontStyle::CENTER | FontStyle::BOTTOM | FontStyle::NO_OUTLINE, NormalFont);

    // Current maximum of y-axis
    txtMaxValueY = addText(ID_txtMaxY, DrawPoint(211, 125), "1", labelColor,
                           FontStyle::RIGHT | FontStyle::VCENTER | FontStyle::NO_OUTLINE, NormalFont);

    // Current minimum of y-axis
    txtMinValueY = addText(ID_txtMinY, DrawPoint(211, 200), "0", labelColor,
                           FontStyle::RIGHT | FontStyle::VCENTER | FontStyle::NO_OUTLINE, NormalFont);

    // Time values on the x-axis
    for(const auto i : helpers::range(timeAnnotations.size()))
    {
        timeAnnotations[i] = addText(ID_txtXAxisValuesStart + i, DrawPoint(0, 0), "", labelColor,
                                     FontStyle::CENTER | FontStyle::TOP | FontStyle::NO_OUTLINE, NormalFont);
    }

    // Default values:
    statChanger->setSelection(ID_btTypeSize);
    currentView = StatisticType::Country;
    timeChanger->setSelection(ID_time15m, true);
    currentTime = StatisticTime::T15Minutes;

    if(!SETTINGS.ingame.scaleStatistics)
        txtMinValueY->setVisible(false);
}

iwStatistics::~iwStatistics() = default;

void iwStatistics::msgButtonClick(const unsigned ctrl_id)
{
    const unsigned playerIndex = ctrl_id - ID_btPlayer_Start;
    if(playerIndex < showStatistic.size())
        showStatistic[playerIndex] = !showStatistic[playerIndex];
    else if(ctrl_id == ID_btHelp)
    {
        WINDOWMANAGER.replaceWindow(
          std::make_unique<iwHelp>(_("This window allows a direct comparison with the enemies. "
                                     "Factors such as the wealth, territorial area, inhabitants, "
                                     "etc. of all parties can be compared. This data can be shown "
                                     "over four different time periods.")));
    }
}

void iwStatistics::msgOptionGroupChange(const unsigned ctrl_id, const unsigned selection)
{
    switch(ctrl_id)
    {
        case ID_grpStatType:
            switch(selection)
            {
                case ID_btTypeSize:
                    currentView = StatisticType::Country;
                    headline->setText(_("Size of country"));
                    break;
                case ID_btTypeBuildings:
                    currentView = StatisticType::Buildings;
                    headline->setText(_("Buildings"));
                    break;
                case ID_btTypeInhabitants:
                    currentView = StatisticType::Inhabitants;
                    headline->setText(_("Inhabitants"));
                    break;
                case ID_btTypeMerchandise:
                    currentView = StatisticType::Merchandise;
                    headline->setText(_("Merchandise"));
                    break;
                case ID_btTypeMilitary:
                    currentView = StatisticType::Military;
                    headline->setText(_("Military strength"));
                    break;
                case ID_btTypeGold:
                    currentView = StatisticType::Gold;
                    headline->setText(_("Gold"));
                    break;
                case ID_btTypeProductivity:
                    currentView = StatisticType::Productivity;
                    headline->setText(_("Productivity"));
                    break;
                case ID_btTypeVanquishedEnemies:
                    currentView = StatisticType::Vanquished;
                    headline->setText(_("Vanquished enemies"));
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

void iwStatistics::drawContent()
{
    // Draw the alliances and the colored boxes under the portraits
    drawPlayerOverlays();

    drawAxis();

    // Draw lines making up the statistic
    drawStatistic(currentView);
}

void iwStatistics::drawPlayerOverlays()
{
    DrawPoint drawPt = getDrawPos() + playerButtonsCenterPos - DrawPoint(numPlayingPlayers * playerBtSize.x / 2, 0);

    for(const auto i : helpers::range(gwv.getWorld().getNumPlayers()))
    {
        const GamePlayer& player = gwv.getWorld().getPlayer(i);
        if(!player.isUsed())
            continue;

        if(showStatistic[i])
        {
            // Draw the alliances at top of the player image
            drawPlayerAlliances(drawPt, player);
            // Draw player boxes and player status at bottom
            drawPlayerBox(drawPt, player);
        }
        drawPt.x += playerBtSize.x;
    }
}

void iwStatistics::drawPlayerBox(DrawPoint const& drawPt, const GamePlayer& player)
{
    auto const playerBoxRect = Rect(DrawPoint(drawPt.x, drawPt.y + 47), Extent(34, 12));
    auto const playerStatusPosition =
      DrawPoint(playerBoxRect.getOrigin() + playerBoxRect.getSize() / 2 + DrawPoint(0, 1));
    drawRectangle(playerBoxRect, player.color);
    SmallFont->draw(playerStatusPosition, getPlayerStatus(player), FontStyle::CENTER | FontStyle::VCENTER,
                    COLOR_YELLOW);
}

void iwStatistics::drawPlayerAlliances(DrawPoint const& drawPt, const GamePlayer& player)
{
    constexpr unsigned spacingAllianceRects = 1;
    constexpr Extent allianceRectExtent(4, 6);
    constexpr unsigned allianceRectOffset = allianceRectExtent.x + spacingAllianceRects;

    auto pactPos = drawPt + DrawPoint::all(spacingAllianceRects);
    for(auto idxOther : helpers::range(gwv.getWorld().getNumPlayers()))
    {
        if(idxOther != player.getPlayerId() && player.isAlly(idxOther))
        {
            drawRectangle(Rect(pactPos, allianceRectExtent), gwv.getWorld().getPlayer(idxOther).color);
            pactPos.x += allianceRectOffset;
        }
    }
}

void iwStatistics::drawStatistic(StatisticType type)
{
    // Find min and max value
    unsigned max = 1;
    auto min = std::numeric_limits<unsigned>::max();
    const GameWorldBase& world = gwv.getWorld();
    for(const auto p : helpers::range(world.getNumPlayers()))
    {
        if(!showStatistic[p])
            continue;
        const GamePlayer::Statistic& stat = world.getPlayer(p).getStatistic(currentTime);

        const auto currentIndex = stat.currentIndex;
        for(const auto i : helpers::range(NUM_STAT_STEPS))
        {
            const auto idx = (currentIndex >= i) ? (currentIndex - i) : (NUM_STAT_STEPS - i + currentIndex);
            max = std::max(max, stat.data[type][idx]);
            if(SETTINGS.ingame.scaleStatistics) //-V807
                min = std::min(min, stat.data[type][idx]);
        }
    }

    if(max == min)
    {
        --min;
        ++max;
    }
    // Add values to axis
    txtMaxValueY->setText(std::to_string(max));
    if(SETTINGS.ingame.scaleStatistics)
        txtMinValueY->setText(std::to_string(min));

    // Draw the line diagram
    const DrawPoint topLeft = getPos() + topLeftRel;
    DrawPoint previousPos;

    for(const auto p : helpers::range(world.getNumPlayers()))
    {
        if(!showStatistic[p])
            continue;
        const GamePlayer::Statistic& stat = world.getPlayer(p).getStatistic(currentTime);
        const auto playerColor = world.getPlayer(p).color;

        const auto currentIndex = stat.currentIndex;
        for(const auto i : helpers::range(NUM_STAT_STEPS))
        {
            DrawPoint curPos = topLeft + DrawPoint((NUM_STAT_STEPS - i) * stepX, diagramSize.y);
            unsigned curStatVal =
              stat.data[type][(currentIndex >= i) ? (currentIndex - i) : (NUM_STAT_STEPS - i + currentIndex)];
            if(SETTINGS.ingame.scaleStatistics)
                curPos.y -= ((curStatVal - min) * diagramSize.y) / (max - min);
            else
                curPos.y -= (curStatVal * diagramSize.y) / max;
            if(i != 0)
                drawLine(curPos, previousPos, 2, playerColor);
            previousPos = curPos;
        }
    }
}

void iwStatistics::drawAxis()
{
    const DrawPoint topLeft = getPos() + topLeftRel;
    const auto axisColor = MakeColor(255, 88, 44, 16);

    // X-axis, horizontal
    // slightly lower to see zero line, offset so it starts with last (leftmost) value of right-aligned graph
    const auto startPoint = topLeft + DrawPoint(stepX, diagramSize.y + 2);
    drawLine(startPoint, topLeft + diagramSize + DrawPoint(0, 2), 1, axisColor);

    // Y-axis, vertical
    drawLine(topLeft + DrawPoint(diagramSize.x, 0), topLeft + diagramSize + DrawPoint(0, 5), 1, axisColor);

    // Marks on y-axis
    drawLine(topLeft + DrawPoint(diagramSize.x - 3, 0), topLeft + DrawPoint(diagramSize.x + 4, 0), 1, axisColor);
    drawLine(topLeft + DrawPoint(diagramSize.x - 3, diagramSize.y / 2),
             topLeft + DrawPoint(diagramSize.x + 4, diagramSize.y / 2), 1, axisColor);

    // Marks on x-axis
    for(const auto* lbl : timeAnnotations)
    {
        if(lbl->isVisible())
        {
            const auto lblPos = lbl->getDrawPos();
            drawLine(lblPos - DrawPoint(0, 4), lblPos - DrawPoint(0, 2), 1, axisColor);
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
            timeAnnotations[1]->setText("-15");
            timeAnnotations[2]->setText("-12");
            timeAnnotations[3]->setText("-9");
            timeAnnotations[4]->setText("-6");
            timeAnnotations[5]->setText("-3");
            numMarks = 6;
            break;
        case StatisticTime::T1Hour:
            timeAnnotations[1]->setText("-60");
            timeAnnotations[2]->setText("-50");
            timeAnnotations[3]->setText("-40");
            timeAnnotations[4]->setText("-30");
            timeAnnotations[5]->setText("-20");
            timeAnnotations[6]->setText("-10");
            numMarks = 7;
            break;
        case StatisticTime::T4Hours:
            timeAnnotations[1]->setText("-240");
            timeAnnotations[2]->setText("-180");
            timeAnnotations[3]->setText("-120");
            timeAnnotations[4]->setText("-60");
            numMarks = 5;
            break;
        case StatisticTime::T16Hours:
            timeAnnotations[1]->setText("-960");
            timeAnnotations[2]->setText("-720");
            timeAnnotations[3]->setText("-480");
            timeAnnotations[4]->setText("-240");
            numMarks = 5;
            break;
    }
    constexpr DrawPoint offset(stepX, 6);
    DrawPoint curPos = topLeftRel + offset + DrawPoint(0, diagramSize.y);
    for(const auto i : helpers::range(1, numMarks))
    {
        timeAnnotations[i]->setPos(curPos);
        timeAnnotations[i]->setVisible(true);
        curPos.x += (diagramSize.x - offset.x) / (numMarks - 1);
    }
    for(const auto i : helpers::range<unsigned>(numMarks, timeAnnotations.size()))
        timeAnnotations[i]->setVisible(false);

    // Zero is used for all time periods
    timeAnnotations[0]->setPos(topLeftRel + diagramSize + DrawPoint(0, offset.y)); // Exactly at the end of the x-axis
    timeAnnotations[0]->setText("0");
}
