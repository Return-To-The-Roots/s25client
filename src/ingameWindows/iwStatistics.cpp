// Copyright (c) 2005-2010 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Siedler II.5 RTTR.
//
// Siedler II.5 RTTR is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Siedler II.5 RTTR is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Siedler II.5 RTTR. If not, see <http://www.gnu.org/licenses/>.

#include "defines.h" // IWYU pragma: keep
#include "iwStatistics.h"

#include "Loader.h"
#include "Settings.h"
#include "GameClient.h"
#include "GamePlayer.h"
#include "WindowManager.h"
#include "iwHelp.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlText.h"
#include "ogl/glArchivItem_Font.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldViewer.h"
#include "addons/const_addons.h"
#include "gameData/const_gui_ids.h"

iwStatistics::iwStatistics(const GameWorldViewer& gwv):
    IngameWindow(CGI_STATISTICS, IngameWindow::posAtMouse,  252, 336, _("Statistics"), LOADER.GetImageN("resource", 41)),
    gwv(gwv)
{
    activePlayers = std::vector<bool>(MAX_PLAYERS);

    // Spieler zählen
    numPlayingPlayers = 0;
    const GameWorldBase& world = gwv.GetWorld();
    for (unsigned i = 0; i < world.GetPlayerCount(); ++i)
    {
        if (world.GetPlayer(i).isUsed())
            numPlayingPlayers++;
    }

    // Bilder für die spielenden Spieler malen (nur vier in Gebrauch, da kein einzelner Anführer auswählbar)
    unsigned short startX = 126 - (numPlayingPlayers - 1) * 17;
    unsigned pos = 0;

    for (unsigned i = 0; i < world.GetPlayerCount(); ++i)
    {
        // nicht belegte Spielplätze rauswerfen
        const GamePlayer& curPlayer = world.GetPlayer(i);
        if (!curPlayer.isUsed())
            continue;

        switch(curPlayer.nation)
        {
            case NAT_AFRICANS: AddImageButton(1+i, startX + pos * 34 - 17, 45-23, 34, 47, TC_GREEN1, LOADER.GetImageN("io", 257), curPlayer.name)->SetBorder(false);
                break;
            case NAT_JAPANESE: AddImageButton(1+i, startX + pos * 34 - 17, 45-23, 34, 47, TC_GREEN1, LOADER.GetImageN("io", 253), curPlayer.name)->SetBorder(false);
                break;
            case NAT_ROMANS: AddImageButton(1+i, startX + pos * 34 - 17, 45-23, 34, 47, TC_GREEN1, LOADER.GetImageN("io", 252), curPlayer.name)->SetBorder(false);
                break;
            case NAT_VIKINGS: AddImageButton(1+i, startX + pos * 34 - 17, 45-23, 34, 47, TC_GREEN1, LOADER.GetImageN("io", 256), curPlayer.name)->SetBorder(false);
                break;
            case NAT_BABYLONIANS: AddImageButton(1+i, startX + pos * 34 - 17, 45-23, 34, 47, TC_GREEN1, LOADER.GetImageN("io_new", 7), curPlayer.name)->SetBorder(false);
                break;
            case NAT_COUNT:
            case NAT_INVALID:
                break;
        }

        // Statistik-Sichtbarkeit abhängig von Auswahl
        switch (GAMECLIENT.IsReplayModeOn() ? 0 : world.GetGGS().getSelection(AddonId::STATISTICS_VISIBILITY))
        {
            default: // Passiert eh nicht, nur zur Sicherheit
                activePlayers[i] = false;

            case 0: // Alle sehen alles
            {
                activePlayers[i] = true;
            } break;
            case 1: // Nur Verbündete teilen Sicht
            {
                const bool visible = gwv.GetPlayer().IsAlly(i);
                activePlayers[i] = visible;
                GetCtrl<ctrlImageButton>(1 + i)->SetEnabled(visible);
            } break;
            case 2: // Nur man selber
            {
                const bool visible = (gwv.GetPlayerId() == i);
                activePlayers[i] = visible;
                GetCtrl<ctrlImageButton>(1 + i)->SetEnabled(visible);
            } break;
        }

        pos++;
    }

    // Statistikfeld
    AddImage(10, 11 + 115, 84 + 81, LOADER.GetImageN("io", 228));

    // Die Buttons zum Wechseln der Statistiken
    ctrlOptionGroup* statChanger = AddOptionGroup(19, ctrlOptionGroup::ILLUMINATE);
    statChanger->AddImageButton(11, 18, 250, 26, 30, TC_GREY, LOADER.GetImageN("io", 167), _("Size of country"));
    statChanger->AddImageButton(12, 45, 250, 26, 30, TC_GREY, LOADER.GetImageN("io", 168), _("Buildings"));
    statChanger->AddImageButton(13, 72, 250, 26, 30, TC_GREY, LOADER.GetImageN("io", 169), _("Inhabitants"));
    statChanger->AddImageButton(14, 99, 250, 26, 30, TC_GREY, LOADER.GetImageN("io", 170), _("Merchandise"));
    statChanger->AddImageButton(15, 126, 250, 26, 30, TC_GREY, LOADER.GetImageN("io", 171), _("Military strength"));
    statChanger->AddImageButton(16, 153, 250, 26, 30, TC_GREY, LOADER.GetImageN("io", 172), _("Gold"));
    statChanger->AddImageButton(17, 180, 250, 26, 30, TC_GREY, LOADER.GetImageN("io", 173), _("Productivity"));
    statChanger->AddImageButton(18, 207, 250, 26, 30, TC_GREY, LOADER.GetImageN("io", 217), _("Vanquished enemies"));

    // Zeit-Buttons
    ctrlOptionGroup* timeChanger = AddOptionGroup(20, ctrlOptionGroup::ILLUMINATE);
    timeChanger->AddTextButton(21, 51, 288, 43, 28, TC_GREY, _("15 m"), NormalFont);
    timeChanger->AddTextButton(22, 96, 288, 43, 28, TC_GREY, _("1 h"), NormalFont);
    timeChanger->AddTextButton(23, 141, 288, 43, 28, TC_GREY, _("4 h"), NormalFont);
    timeChanger->AddTextButton(24, 186, 288, 43, 28, TC_GREY, _("16 h"), NormalFont);

    // Hilfe-Button
    AddImageButton(25, 18, 288, 30, 32, TC_GREY, LOADER.GetImageN("io", 225), _("Help"));

    // Aktuelle Überschrift über der Statistik
    headline = AddText(30, 130, 120, _("Size of country"), MakeColor(255, 136, 96, 52),
                       glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM | glArchivItem_Font::DF_NO_OUTLINE, NormalFont); // qx: fix for bug #1106952

    // Aktueller Maximalwert an der y-Achse
    maxValue = AddText(31, 211, 125, "1", MakeColor(255, 136, 96, 52),
                       glArchivItem_Font::DF_RIGHT | glArchivItem_Font::DF_VCENTER, LOADER.GetFontN("resource", 0));

    // Aktueller Minimalwert an der y-Achse
    minValue = AddText(40, 211, 200, "0", MakeColor(255, 136, 96, 52),
                       glArchivItem_Font::DF_RIGHT | glArchivItem_Font::DF_VCENTER, LOADER.GetFontN("resource", 0));

    // Zeit-Werte an der x-Achse
    timeAnnotations = std::vector<ctrlText*>(7); // TODO nach oben
    for (unsigned i = 0; i < 7; ++i)
    {
        timeAnnotations[i] = AddText(32 + i, 211 + i, 125 + i, "", MakeColor(255, 136, 96, 52),
                                     glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_TOP, LOADER.GetFontN("resource", 0));
    }

    // Standardansicht: 15min / Landesgröße
    statChanger->SetSelection(11);
    currentView = STAT_COUNTRY;
    timeChanger->SetSelection(21);
    currentTime = STAT_15M;

    if (!SETTINGS.ingame.scale_statistics)
        minValue->SetVisible(false);
}

iwStatistics::~iwStatistics()
{

}

void iwStatistics::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch (ctrl_id)
    {
        case 1: case 2: case 3: case 4: case 5: case 6: case 7: // Spielerportraits
            activePlayers[ctrl_id - 1] = !activePlayers[ctrl_id - 1];
            break;
        case 25: // Hilfe
        {
            WINDOWMANAGER.Show(new iwHelp(GUI_ID(CGI_HELP), _(
                "This window allows a direct comparison with the enemies. "
                "Factors such as the wealth, territorial area, inhabitants, "
                "etc. of all parties can be compared. This data can be shown "
                "over four different time periods.")));
        } break;
    }
}

void iwStatistics::Msg_OptionGroupChange(const unsigned int ctrl_id, const int selection)
{
    switch(ctrl_id)
    {
        case 19: // Statistikart wählen
            switch(selection)
            {
                case 11:
                    currentView = STAT_COUNTRY;
                    headline->SetText(_("Size of country"));
                    break;
                case 12:
                    currentView = STAT_BUILDINGS;
                    headline->SetText(_("Buildings"));
                    break;
                case 13:
                    currentView = STAT_INHABITANTS;
                    headline->SetText(_("Inhabitants"));
                    break;
                case 14:
                    currentView = STAT_MERCHANDISE;
                    headline->SetText(_("Merchandise"));
                    break;
                case 15:
                    currentView = STAT_MILITARY;
                    headline->SetText(_("Military strength"));
                    break;
                case 16:
                    currentView = STAT_GOLD;
                    headline->SetText(_("Gold"));
                    break;
                case 17:
                    currentView = STAT_PRODUCTIVITY;
                    headline->SetText(_("Productivity"));
                    break;
                case 18:
                    currentView = STAT_VANQUISHED;
                    headline->SetText(_("Vanquished enemies"));
                    break;
            }
            break;
        case 20: // Zeitbereich wählen
            switch(selection)
            {
                case 21:
                    currentTime = STAT_15M;
                    break;
                case 22:
                    currentTime = STAT_1H;
                    break;
                case 23:
                    currentTime = STAT_4H;
                    break;
                case 24:
                    currentTime = STAT_16H;
                    break;
            }
            break;
    }
}

void iwStatistics::Msg_PaintAfter()
{
    // Die farbigen Boxen unter den Spielerportraits malen
    unsigned short startX = 126 - numPlayingPlayers * 17;
    DrawPoint drawPt = GetDrawPos() + DrawPoint(startX, 68);
    for (unsigned i = 0; i < gwv.GetWorld().GetPlayerCount(); ++i)
    {
        const GamePlayer& player = gwv.GetWorld().GetPlayer(i);
        if (!player.isUsed())
            continue;

        if (activePlayers[i])
            DrawRectangle(drawPt, 34, 12, player.color);
        drawPt.x += 34;
    }

    // Koordinatenachsen malen
    DrawAxis();

    // Statistiklinien malen
    DrawStatistic(currentView);
}

void iwStatistics::DrawStatistic(StatisticType type)
{
    // Ein paar benötigte Werte...
    const int sizeX = 180;
    const int sizeY = 80;
    const DrawPoint topLeft = pos_ + DrawPoint(34, 124);
    const int stepX = sizeX / STAT_STEP_COUNT; // 6

    unsigned short currentIndex;
    unsigned int max = 1;
    unsigned int min = 65000;

    // Maximal- und Minimalwert suchen
    const GameWorldBase& world = gwv.GetWorld();
    for (unsigned int p = 0; p < world.GetPlayerCount(); ++p)
    {
        if (!activePlayers[p])
            continue;
        const GamePlayer::Statistic& stat = world.GetPlayer(p).GetStatistic(currentTime);

        currentIndex = stat.currentIndex;
        for (unsigned int i = 0; i < STAT_STEP_COUNT; ++i)
        {
            if (max < stat.data[type][(currentIndex >= i) ? (currentIndex - i) : (STAT_STEP_COUNT - i + currentIndex)])
            {
                max = stat.data[type][(currentIndex >= i) ? (currentIndex - i) : (STAT_STEP_COUNT - i + currentIndex)];
            }
            if (SETTINGS.ingame.scale_statistics && min > stat.data[type][(currentIndex >= i) ? (currentIndex - i) : (STAT_STEP_COUNT - i + currentIndex)]) //-V807
            {
                min = stat.data[type][(currentIndex >= i) ? (currentIndex - i) : (STAT_STEP_COUNT - i + currentIndex)];
            }
        }
    }

    // Maximalen/Minimalen Wert an die Achse schreiben
    std::stringstream ss;
    if (SETTINGS.ingame.scale_statistics)
    {
        if (max - min == 0)
        {
            --min;
            ++max;
        };
    }
    ss << max;
    maxValue->SetText(ss.str());
    if(SETTINGS.ingame.scale_statistics)
    {
        ss.str("");
        ss << min;
        minValue->SetText(ss.str());
    }

    // Statistiklinien zeichnen
    unsigned short previousX = 0, previousY = 0;
    for (unsigned p = 0; p < world.GetPlayerCount(); ++p)
    {
        if (!activePlayers[p])
            continue;
        const GamePlayer::Statistic& stat = world.GetPlayer(p).GetStatistic(currentTime);

        currentIndex = stat.currentIndex;
        for (unsigned int i = 0; i < STAT_STEP_COUNT; ++i)
        {
            if (i != 0)
            {
                if(SETTINGS.ingame.scale_statistics)
                {
                    DrawLine(topLeft.x + (STAT_STEP_COUNT - i) * stepX,
                             topLeft.y + sizeY - ((stat.data[type][(currentIndex >= i) ? (currentIndex - i) : (STAT_STEP_COUNT - i + currentIndex)] - min)*sizeY) / (max - min),
                             previousX, previousY, 2, world.GetPlayer(p).color);
                }
                else
                {
                    DrawLine(topLeft.x + (STAT_STEP_COUNT - i) * stepX,
                             topLeft.y + sizeY - ((stat.data[type][(currentIndex >= i) ? (currentIndex - i) : (STAT_STEP_COUNT - i + currentIndex)])*sizeY) / max,
                             previousX, previousY, 2, world.GetPlayer(p).color);
                }
            }
            previousX = topLeft.x + (STAT_STEP_COUNT - i) * stepX;
            if(SETTINGS.ingame.scale_statistics)
            {
                previousY = topLeft.y + sizeY - ((stat.data[type][(currentIndex >= i) ? (currentIndex - i) : (STAT_STEP_COUNT - i + currentIndex)] - min) * sizeY) / (max - min);
            }
            else
            {
                previousY = topLeft.y + sizeY - ((stat.data[type][(currentIndex >= i) ? (currentIndex - i) : (STAT_STEP_COUNT - i + currentIndex)]) * sizeY) / max;
            }
        }
    }
}

void iwStatistics::DrawAxis()
{
    // Ein paar benötigte Werte...
    const int sizeX = 180;
    const int sizeY = 80;
    const DrawPoint topLeft = pos_ + DrawPoint(34, 124);
    const DrawPoint topLeftRel(37, 124);

    // X-Achse, horizontal, war irgendwie zu lang links :S
    DrawLine(topLeft.x + 6, topLeft.y + sizeY + 2, // bisschen tiefer, damit man nulllinien noch sieht
             topLeft.x + sizeX, topLeft.y + sizeY + 1, 1, MakeColor(255, 88, 44, 16));

    // Y-Achse, vertikal
    DrawLine(topLeft.x + sizeX, topLeft.y,
             topLeft.x + sizeX, topLeft.y + sizeY + 5, 1, MakeColor(255, 88, 44, 16));

    // Striche an der Y-Achse
    DrawLine(topLeft.x + sizeX - 3, topLeft.y, topLeft.x + sizeX + 4, topLeft.y, 1, MakeColor(255, 88, 44, 16));
    DrawLine(topLeft.x + sizeX - 3, topLeft.y + sizeY / 2, topLeft.x + sizeX + 4, topLeft.y + sizeY / 2, 1, MakeColor(255, 88, 44, 16));

    // Striche an der X-Achse + Beschriftung
    // Zunächst die 0, die haben alle
    timeAnnotations[6]->Move(topLeftRel.x + 180, topLeftRel.y + sizeY + 6);
    timeAnnotations[6]->SetText("0");
    timeAnnotations[6]->SetVisible(true);

    switch(currentTime)
    {
        case STAT_15M:
            // -15
            DrawLine(topLeft.x + 6, topLeft.y + sizeY + 2,
                     topLeft.x + 6, topLeft.y + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[0]->Move(topLeftRel.x + 6, topLeftRel.y + sizeY + 6);
            timeAnnotations[0]->SetText("-15");
            timeAnnotations[0]->SetVisible(true);

            // -12
            DrawLine(topLeft.x + 40, topLeft.y + sizeY + 2,
                     topLeft.x + 40, topLeft.y + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[1]->Move(topLeftRel.x + 40, topLeftRel.y + sizeY + 6);
            timeAnnotations[1]->SetText("-12");
            timeAnnotations[1]->SetVisible(true);

            // -9
            DrawLine(topLeft.x + 75, topLeft.y + sizeY + 2,
                     topLeft.x + 75, topLeft.y + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[2]->Move(topLeftRel.x + 75, topLeftRel.y + sizeY + 6);
            timeAnnotations[2]->SetText("-9");
            timeAnnotations[2]->SetVisible(true);

            // -6
            DrawLine(topLeft.x + 110, topLeft.y + sizeY + 2,
                     topLeft.x + 110, topLeft.y + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[3]->Move(topLeftRel.x + 110, topLeftRel.y + sizeY + 6);
            timeAnnotations[3]->SetText("-6");
            timeAnnotations[3]->SetVisible(true);

            // -3
            DrawLine(topLeft.x + 145, topLeft.y + sizeY + 2,
                     topLeft.x + 145, topLeft.y + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[4]->Move(topLeftRel.x + 145, topLeftRel.y + sizeY + 6);
            timeAnnotations[4]->SetText("-3");
            timeAnnotations[4]->SetVisible(true);

            timeAnnotations[5]->SetVisible(false);
            break;
        case STAT_1H:
            // -60
            DrawLine(topLeft.x + 6, topLeft.y + sizeY + 2,
                     topLeft.x + 6, topLeft.y + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[0]->Move(topLeftRel.x + 6, topLeftRel.y + sizeY + 6);
            timeAnnotations[0]->SetText("-60");
            timeAnnotations[0]->SetVisible(true);

            // -50
            DrawLine(topLeft.x + 35, topLeft.y + sizeY + 2,
                     topLeft.x + 35, topLeft.y + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[1]->Move(topLeftRel.x + 35, topLeftRel.y + sizeY + 6);
            timeAnnotations[1]->SetText("-50");
            timeAnnotations[1]->SetVisible(true);

            // -40
            DrawLine(topLeft.x + 64, topLeft.y + sizeY + 2,
                     topLeft.x + 64, topLeft.y + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[2]->Move(topLeftRel.x + 64, topLeftRel.y + sizeY + 6);
            timeAnnotations[2]->SetText("-40");
            timeAnnotations[2]->SetVisible(true);

            // -30
            DrawLine(topLeft.x + 93, topLeft.y + sizeY + 2,
                     topLeft.x + 93, topLeft.y + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[3]->Move(topLeftRel.x + 93, topLeftRel.y + sizeY + 6);
            timeAnnotations[3]->SetText("-30");
            timeAnnotations[3]->SetVisible(true);

            // -20
            DrawLine(topLeft.x + 122, topLeft.y + sizeY + 2,
                     topLeft.x + 122, topLeft.y + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[4]->Move(topLeftRel.x + 122, topLeftRel.y + sizeY + 6);
            timeAnnotations[4]->SetText("-20");
            timeAnnotations[4]->SetVisible(true);

            // -10
            DrawLine(topLeft.x + 151, topLeft.y + sizeY + 2,
                     topLeft.x + 151, topLeft.y + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[5]->Move(topLeftRel.x + 151, topLeftRel.y + sizeY + 6);
            timeAnnotations[5]->SetText("-10");
            timeAnnotations[5]->SetVisible(true);
            break;
        case STAT_4H:
            // -240
            DrawLine(topLeft.x + 6, topLeft.y + sizeY + 2,
                     topLeft.x + 6, topLeft.y + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[0]->Move(topLeftRel.x + 6, topLeftRel.y + sizeY + 6);
            timeAnnotations[0]->SetText("-240");
            timeAnnotations[0]->SetVisible(true);

            // -180
            DrawLine(topLeft.x + 49, topLeft.y + sizeY + 2,
                     topLeft.x + 49, topLeft.y + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[1]->Move(topLeftRel.x + 49, topLeftRel.y + sizeY + 6);
            timeAnnotations[1]->SetText("-180");
            timeAnnotations[1]->SetVisible(true);

            // -120
            DrawLine(topLeft.x + 93, topLeft.y + sizeY + 2,
                     topLeft.x + 93, topLeft.y + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[2]->Move(topLeftRel.x + 93, topLeftRel.y + sizeY + 6);
            timeAnnotations[2]->SetText("-120");
            timeAnnotations[2]->SetVisible(true);

            // -60
            DrawLine(topLeft.x + 136, topLeft.y + sizeY + 2,
                     topLeft.x + 136, topLeft.y + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[3]->Move(topLeftRel.x + 136, topLeftRel.y + sizeY + 6);
            timeAnnotations[3]->SetText("-60");
            timeAnnotations[3]->SetVisible(true);

            timeAnnotations[4]->SetVisible(false);
            timeAnnotations[5]->SetVisible(false);
            break;
        case STAT_16H:
            // -960
            DrawLine(topLeft.x + 6, topLeft.y + sizeY + 2,
                     topLeft.x + 6, topLeft.y + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[0]->Move(topLeftRel.x + 6, topLeftRel.y + sizeY + 6);
            timeAnnotations[0]->SetText("-960");
            timeAnnotations[0]->SetVisible(true);

            // -720
            DrawLine(topLeft.x + 49, topLeft.y + sizeY + 2,
                     topLeft.x + 49, topLeft.y + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[1]->Move(topLeftRel.x + 49, topLeftRel.y + sizeY + 6);
            timeAnnotations[1]->SetText("-720");
            timeAnnotations[1]->SetVisible(true);

            // -480
            DrawLine(topLeft.x + 93, topLeft.y + sizeY + 2,
                     topLeft.x + 93, topLeft.y + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[2]->Move(topLeftRel.x + 93, topLeftRel.y + sizeY + 6);
            timeAnnotations[2]->SetText("-480");
            timeAnnotations[2]->SetVisible(true);

            // -240
            DrawLine(topLeft.x + 136, topLeft.y + sizeY + 2,
                     topLeft.x + 136, topLeft.y + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[3]->Move(topLeftRel.x + 136, topLeftRel.y + sizeY + 6);
            timeAnnotations[3]->SetText("-240");
            timeAnnotations[3]->SetVisible(true);

            timeAnnotations[4]->SetVisible(false);
            timeAnnotations[5]->SetVisible(false);
            break;
    }
}
