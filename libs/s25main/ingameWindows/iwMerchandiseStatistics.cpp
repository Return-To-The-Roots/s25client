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
                   LOADER.getImageN("resource", 41)),
      player(player), currentTime(StatisticTime::T1Hour)
{
    // Statistikfeld
    addImage(0, DrawPoint(10 + 115, 23 + 81), LOADER.getImageN("io", 228));

    // Waren-Buttons
    // obere Reihe
    ctrlMultiSelectGroup* types = addMultiSelectGroup(22, GroupSelectType::Illuminate);
    types->addImageButton(1, DrawPoint(17, 192), Extent(30, 30), TextureColor::Grey, LOADER.getWareTex(GoodType::Wood),
                          _("Wood"));
    types->addImageButton(2, DrawPoint(48, 192), Extent(30, 30), TextureColor::Grey,
                          LOADER.getWareTex(GoodType::Boards), _("Boards"));
    types->addImageButton(3, DrawPoint(79, 192), Extent(30, 30), TextureColor::Grey,
                          LOADER.getWareTex(GoodType::Stones), _("Stones"));
    types->addImageButton(4, DrawPoint(110, 192), Extent(30, 30), TextureColor::Grey, LOADER.getImageN("io", 80),
                          _("Food"));
    types->addImageButton(5, DrawPoint(141, 192), Extent(30, 30), TextureColor::Grey,
                          LOADER.getWareTex(GoodType::Water), _("Water"));
    types->addImageButton(6, DrawPoint(172, 192), Extent(30, 30), TextureColor::Grey, LOADER.getWareTex(GoodType::Beer),
                          _("Beer"));
    types->addImageButton(7, DrawPoint(203, 192), Extent(30, 30), TextureColor::Grey, LOADER.getWareTex(GoodType::Coal),
                          _("Coal"));

    // untere Reihe
    types->addImageButton(8, DrawPoint(17, 227), Extent(30, 30), TextureColor::Grey,
                          LOADER.getWareTex(GoodType::IronOre), _("Ironore"));
    types->addImageButton(9, DrawPoint(48, 227), Extent(30, 30), TextureColor::Grey, LOADER.getWareTex(GoodType::Gold),
                          _("Gold"));
    types->addImageButton(10, DrawPoint(79, 227), Extent(30, 30), TextureColor::Grey, LOADER.getWareTex(GoodType::Iron),
                          _("Iron"));
    types->addImageButton(11, DrawPoint(110, 227), Extent(30, 30), TextureColor::Grey,
                          LOADER.getWareTex(GoodType::Coins), _("Coins"));
    types->addImageButton(12, DrawPoint(141, 227), Extent(30, 30), TextureColor::Grey,
                          LOADER.getWareTex(GoodType::Hammer), _("Tools"));
    types->addImageButton(13, DrawPoint(172, 227), Extent(30, 30), TextureColor::Grey, LOADER.getImageN("io", 111),
                          _("Weapons"));
    types->addImageButton(14, DrawPoint(203, 227), Extent(30, 30), TextureColor::Grey,
                          LOADER.getWareTex(GoodType::Boat), _("Boats"));

    // Hilfe
    addImageButton(16, DrawPoint(17, 261), Extent(30, 32), TextureColor::Grey, LOADER.getImageN("io", 225), _("Help"));

    // Mülleimer
    addImageButton(17, DrawPoint(49, 263), Extent(30, 28), TextureColor::Grey, LOADER.getImageN("io", 106),
                   _("Delete all"));

    // Zeiten
    ctrlOptionGroup* times = addOptionGroup(23, GroupSelectType::Illuminate);
    times->addTextButton(18, DrawPoint(81, 263), Extent(36, 28), TextureColor::Grey, _("15 m"), NormalFont);
    times->addTextButton(19, DrawPoint(119, 263), Extent(36, 28), TextureColor::Grey, _("1 h"), NormalFont);
    times->addTextButton(20, DrawPoint(155, 263), Extent(36, 28), TextureColor::Grey, _("4 h"), NormalFont);
    times->addTextButton(21, DrawPoint(191, 263), Extent(36, 28), TextureColor::Grey, _("16 h"), NormalFont);
    times->setSelection(19);

    // Zeit-Werte an der x-Achse
    timeAnnotations = std::vector<ctrlText*>(7);
    for(unsigned i = 0; i < 7; ++i)
    {
        timeAnnotations[i] = addText(32 + i, DrawPoint(211 + i, 125 + i), "", MakeColor(255, 136, 96, 52),
                                     FontStyle::CENTER | FontStyle::TOP | FontStyle::NO_OUTLINE, NormalFont);
    }

    // Aktueller Maximalwert an der y-Achse
    maxValue = addText(31, DrawPoint(211, 55), "1", MakeColor(255, 136, 96, 52),
                       FontStyle::RIGHT | FontStyle::VCENTER | FontStyle::NO_OUTLINE, NormalFont);
}

iwMerchandiseStatistics::~iwMerchandiseStatistics() = default;

void iwMerchandiseStatistics::msgButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 16: // Hilfe
        {
            WINDOWMANAGER.replaceWindow(std::make_unique<iwHelp>(
              _("The merchandise statistics window allows you to check the quantities "
                "of your merchandise. By clicking the left mouse button you can switch "
                "the display of individual goods on and off. These can displayed over "
                "four different time periods. To delete all displays, click on the wastebasket button.")));
        }
        break;
        case 17: // Alle abwählen
        {
            const std::set<unsigned> active = getCtrl<ctrlMultiSelectGroup>(22)->getSelection();
            for(unsigned sel : active)
                getCtrl<ctrlMultiSelectGroup>(22)->removeSelection(sel);
        }
        break;
    }
}

void iwMerchandiseStatistics::msgOptionGroupChange(const unsigned ctrl_id, const unsigned selection)
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

void iwMerchandiseStatistics::drawContent()
{
    drawRectangles();
    drawAxis();
    drawStatistic();
}

// TODO
void iwMerchandiseStatistics::drawStatistic()
{
    // Ein paar benötigte Werte...
    const int sizeX = 180;
    const int sizeY = 80;
    const DrawPoint topLeft = getPos() + DrawPoint(34, 64);
    const int stepX = sizeX / NUM_STAT_STEPS; // 6

    // Aktive Buttons holen (Achtung ID == BarColor + 1)
    const std::set<unsigned>& active = getCtrl<ctrlMultiSelectGroup>(22)->getSelection();

    // Statistik holen
    const GamePlayer::Statistic stat = player.getStatistic(currentTime);

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
    maxValue->setText(std::to_string(max));

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
                drawLine(drawPos, previous, 2, BarColors[it - 1]);
            }
            previous = drawPos;
        }
    }
}

// Lustige bunte Kästchen über den Buttons malen
void iwMerchandiseStatistics::drawRectangles()
{
    const Extent size(30, 4);
    const DrawPoint step(31, 35);

    const DrawPoint pos = getDrawPos();
    DrawPoint curOffset(17, 187);

    for(unsigned i = 0; i < 14; ++i)
    {
        drawRectangle(Rect(pos + curOffset, size), BarColors[i]);
        if(i == 6)
        {
            curOffset.x = 17;
            curOffset.y += step.y;
        } else
            curOffset.x += step.x;
    }
}

void iwMerchandiseStatistics::drawAxis()
{
    // Ein paar benötigte Werte...
    const int sizeX = 180;
    const int sizeY = 80;
    const DrawPoint topLeft = getPos() + DrawPoint(34, 64);
    const DrawPoint topLeftRel(37, 64);

    // X-Achse, horizontal, war irgendwie zu lang links :S
    drawLine(topLeft + DrawPoint(6, sizeY + 2), // bisschen tiefer, damit man nulllinien noch sieht
             topLeft + DrawPoint(sizeX, sizeY + 1), 1, MakeColor(255, 88, 44, 16));

    // Y-Achse, vertikal
    drawLine(topLeft + DrawPoint(sizeX, 0), topLeft + DrawPoint(sizeX, sizeY + 5), 1, MakeColor(255, 88, 44, 16));

    // Striche an der Y-Achse
    drawLine(topLeft + DrawPoint(sizeX - 3, 0), topLeft + DrawPoint(sizeX + 4, 0), 1, MakeColor(255, 88, 44, 16));
    drawLine(topLeft + DrawPoint(sizeX - 3, sizeY / 2), topLeft + DrawPoint(sizeX + 4, sizeY / 2), 1,
             MakeColor(255, 88, 44, 16));

    // Striche an der X-Achse + Beschriftung
    // Zunächst die 0, die haben alle
    timeAnnotations[6]->setPos(topLeftRel + DrawPoint(180, sizeY + 6));
    timeAnnotations[6]->setText("0");
    timeAnnotations[6]->setVisible(true);

    switch(currentTime)
    {
        case StatisticTime::T15Minutes:
            // -15
            drawLine(topLeft + DrawPoint(6, sizeY + 2), topLeft + DrawPoint(6, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[0]->setPos(topLeftRel + DrawPoint(6, sizeY + 6));
            timeAnnotations[0]->setText("-15");
            timeAnnotations[0]->setVisible(true);

            // -12
            drawLine(topLeft + DrawPoint(40, sizeY + 2), topLeft + DrawPoint(40, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[1]->setPos(topLeftRel + DrawPoint(40, sizeY + 6));
            timeAnnotations[1]->setText("-12");
            timeAnnotations[1]->setVisible(true);

            // -9
            drawLine(topLeft + DrawPoint(75, sizeY + 2), topLeft + DrawPoint(75, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[2]->setPos(topLeftRel + DrawPoint(75, sizeY + 6));
            timeAnnotations[2]->setText("-9");
            timeAnnotations[2]->setVisible(true);

            // -6
            drawLine(topLeft + DrawPoint(110, sizeY + 2), topLeft + DrawPoint(110, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[3]->setPos(topLeftRel + DrawPoint(110, sizeY + 6));
            timeAnnotations[3]->setText("-6");
            timeAnnotations[3]->setVisible(true);

            // -3
            drawLine(topLeft + DrawPoint(145, sizeY + 2), topLeft + DrawPoint(145, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[4]->setPos(topLeftRel + DrawPoint(145, sizeY + 6));
            timeAnnotations[4]->setText("-3");
            timeAnnotations[4]->setVisible(true);

            timeAnnotations[5]->setVisible(false);
            break;
        case StatisticTime::T1Hour:
            // -60
            drawLine(topLeft + DrawPoint(6, sizeY + 2), topLeft + DrawPoint(6, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[0]->setPos(topLeftRel + DrawPoint(6, sizeY + 6));
            timeAnnotations[0]->setText("-60");
            timeAnnotations[0]->setVisible(true);

            // -50
            drawLine(topLeft + DrawPoint(35, sizeY + 2), topLeft + DrawPoint(35, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[1]->setPos(topLeftRel + DrawPoint(35, sizeY + 6));
            timeAnnotations[1]->setText("-50");
            timeAnnotations[1]->setVisible(true);

            // -40
            drawLine(topLeft + DrawPoint(64, sizeY + 2), topLeft + DrawPoint(64, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[2]->setPos(topLeftRel + DrawPoint(64, sizeY + 6));
            timeAnnotations[2]->setText("-40");
            timeAnnotations[2]->setVisible(true);

            // -30
            drawLine(topLeft + DrawPoint(93, sizeY + 2), topLeft + DrawPoint(93, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[3]->setPos(topLeftRel + DrawPoint(93, sizeY + 6));
            timeAnnotations[3]->setText("-30");
            timeAnnotations[3]->setVisible(true);

            // -20
            drawLine(topLeft + DrawPoint(122, sizeY + 2), topLeft + DrawPoint(122, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[4]->setPos(topLeftRel + DrawPoint(122, sizeY + 6));
            timeAnnotations[4]->setText("-20");
            timeAnnotations[4]->setVisible(true);

            // -10
            drawLine(topLeft + DrawPoint(151, sizeY + 2), topLeft + DrawPoint(151, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[5]->setPos(topLeftRel + DrawPoint(151, sizeY + 6));
            timeAnnotations[5]->setText("-10");
            timeAnnotations[5]->setVisible(true);
            break;
        case StatisticTime::T4Hours:
            // -240
            drawLine(topLeft + DrawPoint(6, sizeY + 2), topLeft + DrawPoint(6, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[0]->setPos(topLeftRel + DrawPoint(6, sizeY + 6));
            timeAnnotations[0]->setText("-240");
            timeAnnotations[0]->setVisible(true);

            // -180
            drawLine(topLeft + DrawPoint(49, sizeY + 2), topLeft + DrawPoint(49, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[1]->setPos(topLeftRel + DrawPoint(49, sizeY + 6));
            timeAnnotations[1]->setText("-180");
            timeAnnotations[1]->setVisible(true);

            // -120
            drawLine(topLeft + DrawPoint(93, sizeY + 2), topLeft + DrawPoint(93, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[2]->setPos(topLeftRel + DrawPoint(93, sizeY + 6));
            timeAnnotations[2]->setText("-120");
            timeAnnotations[2]->setVisible(true);

            // -60
            drawLine(topLeft + DrawPoint(136, sizeY + 2), topLeft + DrawPoint(136, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[3]->setPos(topLeftRel + DrawPoint(136, sizeY + 6));
            timeAnnotations[3]->setText("-60");
            timeAnnotations[3]->setVisible(true);

            timeAnnotations[4]->setVisible(false);
            timeAnnotations[5]->setVisible(false);
            break;
        case StatisticTime::T16Hours:
            // -960
            drawLine(topLeft + DrawPoint(6, sizeY + 2), topLeft + DrawPoint(6, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[0]->setPos(topLeftRel + DrawPoint(6, sizeY + 6));
            timeAnnotations[0]->setText("-960");
            timeAnnotations[0]->setVisible(true);

            // -720
            drawLine(topLeft + DrawPoint(49, sizeY + 2), topLeft + DrawPoint(49, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[1]->setPos(topLeftRel + DrawPoint(49, sizeY + 6));
            timeAnnotations[1]->setText("-720");
            timeAnnotations[1]->setVisible(true);

            // -480
            drawLine(topLeft + DrawPoint(93, sizeY + 2), topLeft + DrawPoint(93, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[2]->setPos(topLeftRel + DrawPoint(93, sizeY + 6));
            timeAnnotations[2]->setText("-480");
            timeAnnotations[2]->setVisible(true);

            // -240
            drawLine(topLeft + DrawPoint(136, sizeY + 2), topLeft + DrawPoint(136, sizeY + 4), 1,
                     MakeColor(255, 88, 44, 16));
            timeAnnotations[3]->setPos(topLeftRel + DrawPoint(136, sizeY + 6));
            timeAnnotations[3]->setText("-240");
            timeAnnotations[3]->setVisible(true);

            timeAnnotations[4]->setVisible(false);
            timeAnnotations[5]->setVisible(false);
            break;
    }
}
