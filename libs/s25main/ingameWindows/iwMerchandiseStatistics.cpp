// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwMerchandiseStatistics.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "WindowManager.h"
#include "controls/ctrlMultiSelectGroup.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlText.h"
#include "iwHelp.h"
#include "ogl/FontStyle.h"
#include "gameData/const_gui_ids.h"

// Farben für die einzelnen Balken
const std::array<unsigned, 14> iwMerchandiseStatistics::BarColors = {
  0xFF00D3F7, // türkis
  0xFFFB9E49, // gelb
  0xFFDF6161, // orange
  0xFF71B63C, // hellgrün
  0xFFAE71CB, // rosa
  0xFF9A75BE, // pink
  0xFF9EA6AE, // grau
  0xFF1038A6, // blau
  0xFF714520, // braun
  0xFFAE0000, // rot
  0xFF045514, // dunkelgrün
  0xFF61245D, // dunkellila
  0xFF5D4175, // blasslila
  0xFF202420  // schwarz
};

iwMerchandiseStatistics::iwMerchandiseStatistics(const GamePlayer& player)
    : IngameWindow(CGI_MERCHANDISE_STATISTICS, IngameWindow::posLastOrCenter, Extent(252, 310), _("Merchandise"),
                   LOADER.GetImageN("resource", 41)),
      player(player), currentTime(StatisticTime::T1Hour)
{
    // Statistikfeld
    AddImage(0, DrawPoint(10 + 115, 23 + 81), LOADER.GetImageN("io", 228));

    // Waren-Buttons
    // obere Reihe
    ctrlMultiSelectGroup* types = AddMultiSelectGroup(22, GroupSelectType::Illuminate);
    types->AddImageButton(1, DrawPoint(17, 192), Extent(30, 30), TextureColor::Grey, LOADER.GetWareTex(GoodType::Wood),
                          _("Wood"));
    types->AddImageButton(2, DrawPoint(48, 192), Extent(30, 30), TextureColor::Grey,
                          LOADER.GetWareTex(GoodType::Boards), _("Boards"));
    types->AddImageButton(3, DrawPoint(79, 192), Extent(30, 30), TextureColor::Grey,
                          LOADER.GetWareTex(GoodType::Stones), _("Stones"));
    types->AddImageButton(4, DrawPoint(110, 192), Extent(30, 30), TextureColor::Grey, LOADER.GetImageN("io", 80),
                          _("Food"));
    types->AddImageButton(5, DrawPoint(141, 192), Extent(30, 30), TextureColor::Grey,
                          LOADER.GetWareTex(GoodType::Water), _("Water"));
    types->AddImageButton(6, DrawPoint(172, 192), Extent(30, 30), TextureColor::Grey, LOADER.GetWareTex(GoodType::Beer),
                          _("Beer"));
    types->AddImageButton(7, DrawPoint(203, 192), Extent(30, 30), TextureColor::Grey, LOADER.GetWareTex(GoodType::Coal),
                          _("Coal"));

    // untere Reihe
    types->AddImageButton(8, DrawPoint(17, 227), Extent(30, 30), TextureColor::Grey,
                          LOADER.GetWareTex(GoodType::IronOre), _("Ironore"));
    types->AddImageButton(9, DrawPoint(48, 227), Extent(30, 30), TextureColor::Grey, LOADER.GetWareTex(GoodType::Gold),
                          _("Gold"));
    types->AddImageButton(10, DrawPoint(79, 227), Extent(30, 30), TextureColor::Grey, LOADER.GetWareTex(GoodType::Iron),
                          _("Iron"));
    types->AddImageButton(11, DrawPoint(110, 227), Extent(30, 30), TextureColor::Grey,
                          LOADER.GetWareTex(GoodType::Coins), _("Coins"));
    types->AddImageButton(12, DrawPoint(141, 227), Extent(30, 30), TextureColor::Grey,
                          LOADER.GetWareTex(GoodType::Hammer), _("Tools"));
    types->AddImageButton(13, DrawPoint(172, 227), Extent(30, 30), TextureColor::Grey, LOADER.GetImageN("io", 111),
                          _("Weapons"));
    types->AddImageButton(14, DrawPoint(203, 227), Extent(30, 30), TextureColor::Grey,
                          LOADER.GetWareTex(GoodType::Boat), _("Boats"));

    // Hilfe
    AddImageButton(16, DrawPoint(17, 261), Extent(30, 32), TextureColor::Grey, LOADER.GetImageN("io", 225), _("Help"));

    // Mülleimer
    AddImageButton(17, DrawPoint(49, 263), Extent(30, 28), TextureColor::Grey, LOADER.GetImageN("io", 106),
                   _("Delete all"));

    // Zeiten
    ctrlOptionGroup* times = AddOptionGroup(23, GroupSelectType::Illuminate);
    times->AddTextButton(18, DrawPoint(81, 263), Extent(36, 28), TextureColor::Grey, _("15 m"), NormalFont);
    times->AddTextButton(19, DrawPoint(119, 263), Extent(36, 28), TextureColor::Grey, _("1 h"), NormalFont);
    times->AddTextButton(20, DrawPoint(155, 263), Extent(36, 28), TextureColor::Grey, _("4 h"), NormalFont);
    times->AddTextButton(21, DrawPoint(191, 263), Extent(36, 28), TextureColor::Grey, _("16 h"), NormalFont);
    times->SetSelection(19);

    // Zeit-Werte an der x-Achse
    timeAnnotations = std::vector<ctrlText*>(7);
    for(unsigned i = 0; i < 7; ++i)
    {
        timeAnnotations[i] = AddText(32 + i, DrawPoint(211 + i, 125 + i), "", MakeColor(255, 136, 96, 52),
                                     FontStyle::CENTER | FontStyle::TOP | FontStyle::NO_OUTLINE, NormalFont);
    }

    // Aktueller Maximalwert an der y-Achse
    maxValue = AddText(31, DrawPoint(211, 55), "1", MakeColor(255, 136, 96, 52),
                       FontStyle::RIGHT | FontStyle::VCENTER | FontStyle::NO_OUTLINE, NormalFont);
}

iwMerchandiseStatistics::~iwMerchandiseStatistics() = default;

void iwMerchandiseStatistics::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 16: // Hilfe
        {
            WINDOWMANAGER.ReplaceWindow(std::make_unique<iwHelp>(
              _("The merchandise statistics window allows you to check the quantities "
                "of your merchandise. By clicking the left mouse button you can switch "
                "the display of individual goods on and off. These can displayed over "
                "four different time periods. To delete all displays, click on the wastebasket button.")));
        }
        break;
        case 17: // Alle abwählen
        {
            const std::set<unsigned> active = GetCtrl<ctrlMultiSelectGroup>(22)->GetSelection();
            for(unsigned sel : active)
                GetCtrl<ctrlMultiSelectGroup>(22)->RemoveSelection(sel);
        }
        break;
    }
}

void iwMerchandiseStatistics::Msg_OptionGroupChange(const unsigned ctrl_id, const unsigned selection)
{
    switch(ctrl_id)
    {
        case 23: // Zeitbereich wählen
            switch(selection)
            {
                case 18: currentTime = StatisticTime::T15Minutes; break;
                case 19: currentTime = StatisticTime::T1Hour; break;
                case 20: currentTime = StatisticTime::T4Hours; break;
                case 21: currentTime = StatisticTime::T16Hours; break;
            }
            break;
    }
}

void iwMerchandiseStatistics::Draw_()
{
    IngameWindow::Draw_();

    if(IsMinimized())
        return;

    DrawRectangles();
    DrawAxis();
    DrawStatistic();
}

// TODO
void iwMerchandiseStatistics::DrawStatistic()
{
    // Ein paar benötigte Werte...
    const int sizeX = 180;
    const int sizeY = 80;
    const DrawPoint topLeft = GetPos() + DrawPoint(34, 64);
    const int stepX = sizeX / NUM_STAT_STEPS; // 6

    // Aktive Buttons holen (Achtung ID == BarColor + 1)
    const std::set<unsigned>& active = GetCtrl<ctrlMultiSelectGroup>(22)->GetSelection();

    // Statistik holen
    const GamePlayer::Statistic stat = player.GetStatistic(currentTime);

    // Maximalwert suchen
    unsigned short max = 1;
    for(unsigned it : active)
    {
        for(unsigned i = 0; i < NUM_STAT_STEPS; ++i)
        {
            if(max < stat.merchandiseData[it - 1][i])
            {
                max = stat.merchandiseData[it - 1][i];
            }
        }
    }

    // Maximalen Wert an die Achse schreiben
    maxValue->SetText(std::to_string(max));

    DrawPoint previous(0, 0);
    unsigned short currentIndex = stat.currentIndex;

    for(unsigned short it : active)
    {
        // Testing only:
        // DrawLine(topLeft.x, topLeft.y + 3 * (*it), topLeft + DrawPoint(sizeX, 3 * (*it)), 2, BarColors[(*it) - 1]);

        for(unsigned i = 0; i < NUM_STAT_STEPS; ++i)
        {
            DrawPoint drawPos = topLeft;
            drawPos.x += (NUM_STAT_STEPS - i) * stepX;
            drawPos.y += sizeY
                         - ((stat.merchandiseData[it - 1][(currentIndex >= i) ? (currentIndex - i) :
                                                                                (NUM_STAT_STEPS - i + currentIndex)])
                            * sizeY)
                             / max;
            if(i != 0)
            {
                DrawLine(drawPos, previous, 2, BarColors[it - 1]);
            }
            previous = drawPos;
        }
    }
}

// Lustige bunte Kästchen über den Buttons malen
void iwMerchandiseStatistics::DrawRectangles()
{
    const Extent size(30, 4);
    const DrawPoint step(31, 35);

    const DrawPoint pos = GetDrawPos();
    DrawPoint curOffset(17, 187);

    for(unsigned i = 0; i < 14; ++i)
    {
        DrawRectangle(Rect(pos + curOffset, size), BarColors[i]);
        if(i == 6)
        {
            curOffset.x = 17;
            curOffset.y += step.y;
        } else
            curOffset.x += step.x;
    }
}

void iwMerchandiseStatistics::DrawAxis()
{
    // Ein paar benötigte Werte...
    const int sizeX = 180;
    const int sizeY = 80;
    const DrawPoint topLeft = GetPos() + DrawPoint(34, 64);
    const DrawPoint topLeftRel(37, 64);

    // X-Achse, horizontal, war irgendwie zu lang links :S
    DrawLine(topLeft + DrawPoint(6, sizeY + 2), // bisschen tiefer, damit man nulllinien noch sieht
             topLeft + DrawPoint(sizeX, sizeY + 1), 1, MakeColor(255, 88, 44, 16));

    // Y-Achse, vertikal
    DrawLine(topLeft + DrawPoint(sizeX, 0), topLeft + DrawPoint(sizeX, sizeY + 5), 1, MakeColor(255, 88, 44, 16));

    // Striche an der Y-Achse
    DrawLine(topLeft + DrawPoint(sizeX - 3, 0), topLeft + DrawPoint(sizeX + 4, 0), 1, MakeColor(255, 88, 44, 16));
    DrawLine(topLeft + DrawPoint(sizeX - 3, sizeY / 2), topLeft + DrawPoint(sizeX + 4, sizeY / 2), 1,
             MakeColor(255, 88, 44, 16));

    // Striche an der X-Achse + Beschriftung
    // Zunächst die 0, die haben alle
    timeAnnotations[6]->SetPos(topLeftRel + DrawPoint(180, sizeY + 6));
    timeAnnotations[6]->SetText("0");
    timeAnnotations[6]->SetVisible(true);

    switch(currentTime)
    {
        case StatisticTime::T15Minutes:
            // -15
            DrawLine(topLeft + DrawPoint(6, sizeY + 2), topLeft + DrawPoint(6, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[0]->SetPos(topLeftRel + DrawPoint(6, sizeY + 6));
            timeAnnotations[0]->SetText("-15");
            timeAnnotations[0]->SetVisible(true);

            // -12
            DrawLine(topLeft + DrawPoint(40, sizeY + 2), topLeft + DrawPoint(40, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[1]->SetPos(topLeftRel + DrawPoint(40, sizeY + 6));
            timeAnnotations[1]->SetText("-12");
            timeAnnotations[1]->SetVisible(true);

            // -9
            DrawLine(topLeft + DrawPoint(75, sizeY + 2), topLeft + DrawPoint(75, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[2]->SetPos(topLeftRel + DrawPoint(75, sizeY + 6));
            timeAnnotations[2]->SetText("-9");
            timeAnnotations[2]->SetVisible(true);

            // -6
            DrawLine(topLeft + DrawPoint(110, sizeY + 2), topLeft + DrawPoint(110, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[3]->SetPos(topLeftRel + DrawPoint(110, sizeY + 6));
            timeAnnotations[3]->SetText("-6");
            timeAnnotations[3]->SetVisible(true);

            // -3
            DrawLine(topLeft + DrawPoint(145, sizeY + 2), topLeft + DrawPoint(145, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[4]->SetPos(topLeftRel + DrawPoint(145, sizeY + 6));
            timeAnnotations[4]->SetText("-3");
            timeAnnotations[4]->SetVisible(true);

            timeAnnotations[5]->SetVisible(false);
            break;
        case StatisticTime::T1Hour:
            // -60
            DrawLine(topLeft + DrawPoint(6, sizeY + 2), topLeft + DrawPoint(6, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[0]->SetPos(topLeftRel + DrawPoint(6, sizeY + 6));
            timeAnnotations[0]->SetText("-60");
            timeAnnotations[0]->SetVisible(true);

            // -50
            DrawLine(topLeft + DrawPoint(35, sizeY + 2), topLeft + DrawPoint(35, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[1]->SetPos(topLeftRel + DrawPoint(35, sizeY + 6));
            timeAnnotations[1]->SetText("-50");
            timeAnnotations[1]->SetVisible(true);

            // -40
            DrawLine(topLeft + DrawPoint(64, sizeY + 2), topLeft + DrawPoint(64, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[2]->SetPos(topLeftRel + DrawPoint(64, sizeY + 6));
            timeAnnotations[2]->SetText("-40");
            timeAnnotations[2]->SetVisible(true);

            // -30
            DrawLine(topLeft + DrawPoint(93, sizeY + 2), topLeft + DrawPoint(93, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[3]->SetPos(topLeftRel + DrawPoint(93, sizeY + 6));
            timeAnnotations[3]->SetText("-30");
            timeAnnotations[3]->SetVisible(true);

            // -20
            DrawLine(topLeft + DrawPoint(122, sizeY + 2), topLeft + DrawPoint(122, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[4]->SetPos(topLeftRel + DrawPoint(122, sizeY + 6));
            timeAnnotations[4]->SetText("-20");
            timeAnnotations[4]->SetVisible(true);

            // -10
            DrawLine(topLeft + DrawPoint(151, sizeY + 2), topLeft + DrawPoint(151, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[5]->SetPos(topLeftRel + DrawPoint(151, sizeY + 6));
            timeAnnotations[5]->SetText("-10");
            timeAnnotations[5]->SetVisible(true);
            break;
        case StatisticTime::T4Hours:
            // -240
            DrawLine(topLeft + DrawPoint(6, sizeY + 2), topLeft + DrawPoint(6, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[0]->SetPos(topLeftRel + DrawPoint(6, sizeY + 6));
            timeAnnotations[0]->SetText("-240");
            timeAnnotations[0]->SetVisible(true);

            // -180
            DrawLine(topLeft + DrawPoint(49, sizeY + 2), topLeft + DrawPoint(49, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[1]->SetPos(topLeftRel + DrawPoint(49, sizeY + 6));
            timeAnnotations[1]->SetText("-180");
            timeAnnotations[1]->SetVisible(true);

            // -120
            DrawLine(topLeft + DrawPoint(93, sizeY + 2), topLeft + DrawPoint(93, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[2]->SetPos(topLeftRel + DrawPoint(93, sizeY + 6));
            timeAnnotations[2]->SetText("-120");
            timeAnnotations[2]->SetVisible(true);

            // -60
            DrawLine(topLeft + DrawPoint(136, sizeY + 2), topLeft + DrawPoint(136, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[3]->SetPos(topLeftRel + DrawPoint(136, sizeY + 6));
            timeAnnotations[3]->SetText("-60");
            timeAnnotations[3]->SetVisible(true);

            timeAnnotations[4]->SetVisible(false);
            timeAnnotations[5]->SetVisible(false);
            break;
        case StatisticTime::T16Hours:
            // -960
            DrawLine(topLeft + DrawPoint(6, sizeY + 2), topLeft + DrawPoint(6, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[0]->SetPos(topLeftRel + DrawPoint(6, sizeY + 6));
            timeAnnotations[0]->SetText("-960");
            timeAnnotations[0]->SetVisible(true);

            // -720
            DrawLine(topLeft + DrawPoint(49, sizeY + 2), topLeft + DrawPoint(49, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[1]->SetPos(topLeftRel + DrawPoint(49, sizeY + 6));
            timeAnnotations[1]->SetText("-720");
            timeAnnotations[1]->SetVisible(true);

            // -480
            DrawLine(topLeft + DrawPoint(93, sizeY + 2), topLeft + DrawPoint(93, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[2]->SetPos(topLeftRel + DrawPoint(93, sizeY + 6));
            timeAnnotations[2]->SetText("-480");
            timeAnnotations[2]->SetVisible(true);

            // -240
            DrawLine(topLeft + DrawPoint(136, sizeY + 2), topLeft + DrawPoint(136, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[3]->SetPos(topLeftRel + DrawPoint(136, sizeY + 6));
            timeAnnotations[3]->SetText("-240");
            timeAnnotations[3]->SetVisible(true);

            timeAnnotations[4]->SetVisible(false);
            timeAnnotations[5]->SetVisible(false);
            break;
    }
}
