// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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
#include "gameData/const_gui_ids.h"

namespace {

std::string getPlayerStatus(const GamePlayer& player)
{
    if(player.IsDefeated())
        return "---";
    else if(player.isHuman())
        return "#" + std::to_string(player.GetPlayerId() + 1);
    else
        return _("COMP");
}

} // namespace

iwStatistics::iwStatistics(const GameWorldViewer& gwv)
    : IngameWindow(CGI_STATISTICS, IngameWindow::posLastOrCenter, Extent(252, 336), _("Statistics"),
                   LOADER.GetImageN("resource", 41)),
      gwv(gwv)
{
    activePlayers = std::vector<bool>(MAX_PLAYERS);

    // Spieler zählen
    numPlayingPlayers = 0;
    const GameWorldBase& world = gwv.GetWorld();
    for(unsigned i = 0; i < world.GetNumPlayers(); ++i)
    {
        if(world.GetPlayer(i).isUsed())
            numPlayingPlayers++;
    }

    // Bilder für die spielenden Spieler malen (nur vier in Gebrauch, da kein einzelner Anführer auswählbar)
    unsigned short startX = 126 - (numPlayingPlayers - 1) * 17;
    unsigned pos = 0;

    for(unsigned i = 0; i < world.GetNumPlayers(); ++i)
    {
        // nicht belegte Spielplätze rauswerfen
        const GamePlayer& curPlayer = world.GetPlayer(i);
        if(!curPlayer.isUsed())
            continue;

        switch(curPlayer.nation)
        {
            case Nation::Africans:
                AddImageButton(1 + i, DrawPoint(startX + pos * 34 - 17, 45 - 23), Extent(34, 47), TextureColor::Green1,
                               LOADER.GetImageN("io", 257), curPlayer.name)
                  ->SetBorder(false);
                break;
            case Nation::Japanese:
                AddImageButton(1 + i, DrawPoint(startX + pos * 34 - 17, 45 - 23), Extent(34, 47), TextureColor::Green1,
                               LOADER.GetImageN("io", 253), curPlayer.name)
                  ->SetBorder(false);
                break;
            case Nation::Romans:
                AddImageButton(1 + i, DrawPoint(startX + pos * 34 - 17, 45 - 23), Extent(34, 47), TextureColor::Green1,
                               LOADER.GetImageN("io", 252), curPlayer.name)
                  ->SetBorder(false);
                break;
            case Nation::Vikings:
                AddImageButton(1 + i, DrawPoint(startX + pos * 34 - 17, 45 - 23), Extent(34, 47), TextureColor::Green1,
                               LOADER.GetImageN("io", 256), curPlayer.name)
                  ->SetBorder(false);
                break;
            case Nation::Babylonians:
                AddImageButton(1 + i, DrawPoint(startX + pos * 34 - 17, 45 - 23), Extent(34, 47), TextureColor::Green1,
                               LOADER.GetImageN("io_new", 7), curPlayer.name)
                  ->SetBorder(false);
                break;
        }

        // Statistik-Sichtbarkeit abhängig von Auswahl
        switch(GAMECLIENT.IsReplayModeOn() ? 0 : world.GetGGS().getSelection(AddonId::STATISTICS_VISIBILITY))
        {
            default: // Passiert eh nicht, nur zur Sicherheit
                activePlayers[i] = false;
                break;
            case 0: // Alle sehen alles
                activePlayers[i] = true;
                break;
            case 1: // Nur Verbündete teilen Sicht
            {
                const bool visible = gwv.GetPlayer().IsAlly(i);
                activePlayers[i] = visible;
                GetCtrl<ctrlButton>(1 + i)->SetEnabled(visible);
            }
            break;
            case 2: // Nur man selber
            {
                const bool visible = (gwv.GetPlayerId() == i);
                activePlayers[i] = visible;
                GetCtrl<ctrlButton>(1 + i)->SetEnabled(visible);
            }
            break;
        }

        pos++;
    }

    // Statistikfeld
    AddImage(10, DrawPoint(11 + 115, 84 + 81), LOADER.GetImageN("io", 228));

    // Die Buttons zum Wechseln der Statistiken
    ctrlOptionGroup* statChanger = AddOptionGroup(19, GroupSelectType::Illuminate);
    statChanger->AddImageButton(11, DrawPoint(18, 250), Extent(26, 30), TextureColor::Grey, LOADER.GetImageN("io", 167),
                                _("Size of country"));
    statChanger->AddImageButton(12, DrawPoint(45, 250), Extent(26, 30), TextureColor::Grey, LOADER.GetImageN("io", 168),
                                _("Buildings"));
    statChanger->AddImageButton(13, DrawPoint(72, 250), Extent(26, 30), TextureColor::Grey, LOADER.GetImageN("io", 169),
                                _("Inhabitants"));
    statChanger->AddImageButton(14, DrawPoint(99, 250), Extent(26, 30), TextureColor::Grey, LOADER.GetImageN("io", 170),
                                _("Merchandise"));
    statChanger->AddImageButton(15, DrawPoint(126, 250), Extent(26, 30), TextureColor::Grey,
                                LOADER.GetImageN("io", 171), _("Military strength"));
    statChanger->AddImageButton(16, DrawPoint(153, 250), Extent(26, 30), TextureColor::Grey,
                                LOADER.GetImageN("io", 172), _("Gold"));
    statChanger->AddImageButton(17, DrawPoint(180, 250), Extent(26, 30), TextureColor::Grey,
                                LOADER.GetImageN("io", 173), _("Productivity"));
    statChanger->AddImageButton(18, DrawPoint(207, 250), Extent(26, 30), TextureColor::Grey,
                                LOADER.GetImageN("io", 217), _("Vanquished enemies"));

    // Zeit-Buttons
    ctrlOptionGroup* timeChanger = AddOptionGroup(20, GroupSelectType::Illuminate);
    timeChanger->AddTextButton(21, DrawPoint(51, 288), Extent(43, 28), TextureColor::Grey, _("15 m"), NormalFont);
    timeChanger->AddTextButton(22, DrawPoint(96, 288), Extent(43, 28), TextureColor::Grey, _("1 h"), NormalFont);
    timeChanger->AddTextButton(23, DrawPoint(141, 288), Extent(43, 28), TextureColor::Grey, _("4 h"), NormalFont);
    timeChanger->AddTextButton(24, DrawPoint(186, 288), Extent(43, 28), TextureColor::Grey, _("16 h"), NormalFont);

    // Hilfe-Button
    AddImageButton(25, DrawPoint(18, 288), Extent(30, 32), TextureColor::Grey, LOADER.GetImageN("io", 225), _("Help"));

    // Aktuelle Überschrift über der Statistik
    headline = AddText(30, DrawPoint(130, 120), _("Size of country"), MakeColor(255, 136, 96, 52),
                       FontStyle::CENTER | FontStyle::BOTTOM | FontStyle::NO_OUTLINE,
                       NormalFont); // qx: fix for bug #1106952

    // Aktueller Maximalwert an der y-Achse
    maxValue = AddText(31, DrawPoint(211, 125), "1", MakeColor(255, 136, 96, 52),
                       FontStyle::RIGHT | FontStyle::VCENTER | FontStyle::NO_OUTLINE, NormalFont);

    // Aktueller Minimalwert an der y-Achse
    minValue = AddText(40, DrawPoint(211, 200), "0", MakeColor(255, 136, 96, 52),
                       FontStyle::RIGHT | FontStyle::VCENTER | FontStyle::NO_OUTLINE, NormalFont);

    // Zeit-Werte an der x-Achse
    timeAnnotations = std::vector<ctrlText*>(7); // TODO nach oben
    for(unsigned i = 0; i < 7; ++i)
    {
        timeAnnotations[i] = AddText(32 + i, DrawPoint(211 + i, 125 + i), "", MakeColor(255, 136, 96, 52),
                                     FontStyle::CENTER | FontStyle::TOP | FontStyle::NO_OUTLINE, NormalFont);
    }

    // Standardansicht: 15min / Landesgröße
    statChanger->SetSelection(11);
    currentView = StatisticType::Country;
    timeChanger->SetSelection(21);
    currentTime = StatisticTime::T15Minutes;

    if(!SETTINGS.ingame.scale_statistics)
        minValue->SetVisible(false);
}

iwStatistics::~iwStatistics() = default;

void iwStatistics::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7: // Spielerportraits
            activePlayers[ctrl_id - 1] = !activePlayers[ctrl_id - 1];
            break;
        case 25: // Hilfe
        {
            WINDOWMANAGER.ReplaceWindow(
              std::make_unique<iwHelp>(_("This window allows a direct comparison with the enemies. "
                                         "Factors such as the wealth, territorial area, inhabitants, "
                                         "etc. of all parties can be compared. This data can be shown "
                                         "over four different time periods.")));
        }
        break;
    }
}

void iwStatistics::Msg_OptionGroupChange(const unsigned ctrl_id, const unsigned selection)
{
    switch(ctrl_id)
    {
        case 19: // Statistikart wählen
            switch(selection)
            {
                case 11:
                    currentView = StatisticType::Country;
                    headline->SetText(_("Size of country"));
                    break;
                case 12:
                    currentView = StatisticType::Buildings;
                    headline->SetText(_("Buildings"));
                    break;
                case 13:
                    currentView = StatisticType::Inhabitants;
                    headline->SetText(_("Inhabitants"));
                    break;
                case 14:
                    currentView = StatisticType::Merchandise;
                    headline->SetText(_("Merchandise"));
                    break;
                case 15:
                    currentView = StatisticType::Military;
                    headline->SetText(_("Military strength"));
                    break;
                case 16:
                    currentView = StatisticType::Gold;
                    headline->SetText(_("Gold"));
                    break;
                case 17:
                    currentView = StatisticType::Productivity;
                    headline->SetText(_("Productivity"));
                    break;
                case 18:
                    currentView = StatisticType::Vanquished;
                    headline->SetText(_("Vanquished enemies"));
                    break;
            }
            break;
        case 20: // Zeitbereich wählen
            switch(selection)
            {
                case 21: currentTime = StatisticTime::T15Minutes; break;
                case 22: currentTime = StatisticTime::T1Hour; break;
                case 23: currentTime = StatisticTime::T4Hours; break;
                case 24: currentTime = StatisticTime::T16Hours; break;
            }
            break;
    }
}

void iwStatistics::Draw_()
{
    IngameWindow::Draw_();

    if(IsMinimized())
        return;

    // Draw the alliances and the colored boxes under the portraits
    DrawPlayerOverlays();

    // Koordinatenachsen malen
    DrawAxis();

    // Statistiklinien malen
    DrawStatistic(currentView);
}

void iwStatistics::DrawPlayerOverlays()
{
    DrawPoint drawPt = GetDrawPos() + DrawPoint(126 - numPlayingPlayers * 17, 22);
    for(unsigned i = 0; i < gwv.GetWorld().GetNumPlayers(); ++i)
    {
        const GamePlayer& player = gwv.GetWorld().GetPlayer(i);
        if(!player.isUsed())
            continue;

        if(activePlayers[i])
        {
            // Draw the alliances at top of the player image
            DrawPlayerAlliances(drawPt, player);
            // Draw player boxes and player status at bottom
            DrawPlayerBox(drawPt, player);
        }
        drawPt.x += 34;
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
    // Ein paar benötigte Werte...
    const Extent size(180, 80);
    const int stepX = size.x / NUM_STAT_STEPS;

    unsigned short currentIndex;
    unsigned max = 1;
    unsigned min = 65000;

    // Maximal- und Minimalwert suchen
    const GameWorldBase& world = gwv.GetWorld();
    for(unsigned p = 0; p < world.GetNumPlayers(); ++p)
    {
        if(!activePlayers[p])
            continue;
        const GamePlayer::Statistic& stat = world.GetPlayer(p).GetStatistic(currentTime);

        currentIndex = stat.currentIndex;
        for(unsigned i = 0; i < NUM_STAT_STEPS; ++i)
        {
            if(max < stat.data[type][(currentIndex >= i) ? (currentIndex - i) : (NUM_STAT_STEPS - i + currentIndex)])
            {
                max = stat.data[type][(currentIndex >= i) ? (currentIndex - i) : (NUM_STAT_STEPS - i + currentIndex)];
            }
            if(SETTINGS.ingame.scale_statistics //-V807
               && min > stat.data[type][(currentIndex >= i) ? (currentIndex - i) : (NUM_STAT_STEPS - i + currentIndex)])
            {
                min = stat.data[type][(currentIndex >= i) ? (currentIndex - i) : (NUM_STAT_STEPS - i + currentIndex)];
            }
        }
    }

    if(SETTINGS.ingame.scale_statistics && max == min)
    {
        --min;
        ++max;
    }
    // Maximalen/Minimalen Wert an die Achse schreiben
    maxValue->SetText(std::to_string(max));
    if(SETTINGS.ingame.scale_statistics)
        minValue->SetText(std::to_string(min));

    // Statistiklinien zeichnen
    const DrawPoint topLeft = GetPos() + DrawPoint(34, 124);
    DrawPoint previousPos(0, 0);

    for(unsigned p = 0; p < world.GetNumPlayers(); ++p)
    {
        if(!activePlayers[p])
            continue;
        const GamePlayer::Statistic& stat = world.GetPlayer(p).GetStatistic(currentTime);

        currentIndex = stat.currentIndex;
        for(unsigned i = 0; i < NUM_STAT_STEPS; ++i)
        {
            DrawPoint curPos = topLeft + DrawPoint((NUM_STAT_STEPS - i) * stepX, size.y);
            unsigned curStatVal =
              stat.data[type][(currentIndex >= i) ? (currentIndex - i) : (NUM_STAT_STEPS - i + currentIndex)];
            if(SETTINGS.ingame.scale_statistics)
                curPos.y -= ((curStatVal - min) * size.y) / (max - min);
            else
                curPos.y -= (curStatVal * size.y) / max;
            if(i != 0)
                DrawLine(curPos, previousPos, 2, world.GetPlayer(p).color);
            previousPos = curPos;
        }
    }
}

void iwStatistics::DrawAxis()
{
    // constants / locals
    const int sizeX = 180;
    const int sizeY = 80;
    const DrawPoint topLeft   = GetPos() + DrawPoint(34, 124);
    const DrawPoint topLeftRel(37, 124);
    const auto col = MakeColor(255, 88, 44, 16);

    // lambdas
    auto drawTick = [&](int x){ DrawLine(topLeft + DrawPoint(x, sizeY + 2),
                                         topLeft + DrawPoint(x, sizeY + 4), 1, col); };

    auto setAnno = [&](int slot, int x, const char* text, bool vis=true){
        timeAnnotations[slot]->SetPos(topLeftRel + DrawPoint(x, sizeY + 6));
        timeAnnotations[slot]->SetText(text);
        timeAnnotations[slot]->SetVisible(vis);
    };

    // 1) Axes
    DrawLine(topLeft + DrawPoint(6, sizeY + 2),        // X axis
             topLeft + DrawPoint(sizeX, sizeY + 1), 1, col);
    DrawLine(topLeft + DrawPoint(sizeX, 0),            // Y axis
             topLeft + DrawPoint(sizeX, sizeY + 5), 1, col);

    // Y tick marks
    DrawLine(topLeft + DrawPoint(sizeX - 3, 0),            topLeft + DrawPoint(sizeX + 4, 0),            1, col);
    DrawLine(topLeft + DrawPoint(sizeX - 3, sizeY / 2),    topLeft + DrawPoint(sizeX + 4, sizeY / 2),    1, col);

    // 2) Build X ticks for currentTime
    // Each entry: {pixel_x, label}
    std::vector<std::pair<int, const char*>> ticks;
    switch(currentTime)
    {
        case StatisticTime::T15Minutes:
            ticks = { {6,"-15"}, {40,"-12"}, {75,"-9"}, {110,"-6"}, {145,"-3"} };
            break;
        case StatisticTime::T1Hour:
            ticks = { {6,"-60"}, {35,"-50"}, {64,"-40"}, {93,"-30"}, {122,"-20"}, {151,"-10"} };
            break;
        case StatisticTime::T4Hours:
            ticks = { {6,"-240"}, {49,"-180"}, {93,"-120"}, {136,"-60"} };
            break;
        case StatisticTime::T16Hours:
            ticks = { {6,"-960"}, {49,"-720"}, {93,"-480"}, {136,"-240"} };
            break;
    }

    // 3) Hide all annotations first (0..6)
    for(int i = 0; i <= 6; ++i) timeAnnotations[i]->SetVisible(false);

    // 4) Draw ticks + labels (use slots 0..n-1)
    int slot = 0;
    for(const auto& [x, label] : ticks)
    {
        drawTick(x);
        setAnno(slot++, x, label, true);
    }

    // 5) Draw and show the zero at the end (fixed at x=180, slot 6)
    setAnno(6, 180, "0", true);
}
