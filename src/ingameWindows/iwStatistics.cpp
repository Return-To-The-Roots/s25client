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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h" // IWYU pragma: keep
#include "iwStatistics.h"

#include "Loader.h"
#include "Settings.h"
#include "GameClient.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlText.h"
#include "ogl/glArchivItem_Font.h"
#include "gameData/const_gui_ids.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

iwStatistics::iwStatistics()
    : IngameWindow(CGI_STATISTICS, 0xFFFE, 0xFFFE, 252, 336, _("Statistics"), LOADER.GetImageN("resource", 41))
{
    activePlayers = std::vector<bool>(MAX_PLAYERS);

    // Spieler zählen
    numPlayingPlayers = 0;
    for (unsigned i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
    {
        if (GAMECLIENT.GetPlayer(i).isUsed())
            numPlayingPlayers++;
    }

    // Bilder für die spielenden Spieler malen (nur vier in Gebrauch, da kein einzelner Anführer auswählbar)
    unsigned short startX = 126 - (numPlayingPlayers - 1) * 17;
    unsigned pos = 0;

    for (unsigned i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
    {
        // nicht belegte Spielplätze rauswerfen
        if (!GAMECLIENT.GetPlayer(i).isUsed())
        {
            activePlayers[i] = false;
            continue;
        }
        switch(GAMECLIENT.GetPlayer(i).nation)
        {
            case NAT_AFRICANS: AddImageButton(1+i, startX + pos * 34 - 17, 45-23, 34, 47, TC_GREEN1, LOADER.GetImageN("io", 257), GAMECLIENT.GetPlayer(i).name)->SetBorder(false);
                break;
            case NAT_JAPANESE: AddImageButton(1+i, startX + pos * 34 - 17, 45-23, 34, 47, TC_GREEN1, LOADER.GetImageN("io", 253), GAMECLIENT.GetPlayer(i).name)->SetBorder(false);
                break;
            case NAT_ROMANS: AddImageButton(1+i, startX + pos * 34 - 17, 45-23, 34, 47, TC_GREEN1, LOADER.GetImageN("io", 252), GAMECLIENT.GetPlayer(i).name)->SetBorder(false);
                break;
            case NAT_VIKINGS: AddImageButton(1+i, startX + pos * 34 - 17, 45-23, 34, 47, TC_GREEN1, LOADER.GetImageN("io", 256), GAMECLIENT.GetPlayer(i).name)->SetBorder(false);
                break;
            case NAT_BABYLONIANS: AddImageButton(1+i, startX + pos * 34 - 17, 45-23, 34, 47, TC_GREEN1, LOADER.GetImageN("io_new", 7), GAMECLIENT.GetPlayer(i).name)->SetBorder(false);
                break;
            case NAT_COUNT:
            case NAT_INVALID:
                break;
        }

        // Statistik-Sichtbarkeit abhängig von Auswahl
        switch (GAMECLIENT.IsReplayModeOn() ? 0 : GAMECLIENT.GetGGS().getSelection(AddonId::STATISTICS_VISIBILITY))
        {
            default: // Passiert eh nicht, nur zur Sicherheit
                activePlayers[i] = false;

            case 0: // Alle sehen alles
            {
                activePlayers[i] = true;
            } break;
            case 1: // Nur Verbündete teilen Sicht
            {
                const bool visible = GAMECLIENT.GetLocalPlayer().IsAlly(i);
                activePlayers[i] = visible;
                GetCtrl<ctrlImageButton>(1 + i)->Enable(visible);
            } break;
            case 2: // Nur man selber
            {
                const bool visible = (GAMECLIENT.GetPlayerID() == i);
                activePlayers[i] = visible;
                GetCtrl<ctrlImageButton>(1 + i)->Enable(visible);
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
    AddImageButton(25, 18, 288, 30, 32, TC_GREY, LOADER.GetImageN("io", 21), _("Help"));

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

        case 25: // Hilfe
        {
            // TODO Help!
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
    unsigned pos = 0;
    for (unsigned i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
    {
        GameClientPlayer& player = GAMECLIENT.GetPlayer(i);
        if (!player.isUsed())
            continue;

        if (activePlayers[i])
        {
            DrawRectangle(this->x_ + startX + pos * 34, this->y_ + 68, 34, 12, player.color);
        }
        pos++;
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
    const int topLeftX = this->x_ + 34;
    const int topLeftY = this->y_ + 124;
    const int stepX = sizeX / STAT_STEP_COUNT; // 6

    unsigned short currentIndex;
    unsigned int max = 1;
    unsigned int min = 65000;

    // Maximal- und Minimalwert suchen
    for (unsigned int p = 0; p < GAMECLIENT.GetPlayerCount(); ++p)
    {
        if (!activePlayers[p])
            continue;
        const GameClientPlayer::Statistic& stat = GAMECLIENT.GetPlayer(p).GetStatistic(currentTime);

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
    for (unsigned p = 0; p < GAMECLIENT.GetPlayerCount(); ++p)
    {
        if (!activePlayers[p])
            continue;
        const GameClientPlayer::Statistic& stat = GAMECLIENT.GetPlayer(p).GetStatistic(currentTime);

        currentIndex = stat.currentIndex;
        for (unsigned int i = 0; i < STAT_STEP_COUNT; ++i)
        {
            if (i != 0)
            {
                if(SETTINGS.ingame.scale_statistics)
                {
                    DrawLine(topLeftX + (STAT_STEP_COUNT - i) * stepX,
                             topLeftY + sizeY - ((stat.data[type][(currentIndex >= i) ? (currentIndex - i) : (STAT_STEP_COUNT - i + currentIndex)] - min)*sizeY) / (max - min),
                             previousX, previousY, 2, GAMECLIENT.GetPlayer(p).color);
                }
                else
                {
                    DrawLine(topLeftX + (STAT_STEP_COUNT - i) * stepX,
                             topLeftY + sizeY - ((stat.data[type][(currentIndex >= i) ? (currentIndex - i) : (STAT_STEP_COUNT - i + currentIndex)])*sizeY) / max,
                             previousX, previousY, 2, GAMECLIENT.GetPlayer(p).color);
                }
            }
            previousX = topLeftX + (STAT_STEP_COUNT - i) * stepX;
            if(SETTINGS.ingame.scale_statistics)
            {
                previousY = topLeftY + sizeY - ((stat.data[type][(currentIndex >= i) ? (currentIndex - i) : (STAT_STEP_COUNT - i + currentIndex)] - min) * sizeY) / (max - min);
            }
            else
            {
                previousY = topLeftY + sizeY - ((stat.data[type][(currentIndex >= i) ? (currentIndex - i) : (STAT_STEP_COUNT - i + currentIndex)]) * sizeY) / max;
            }
        }
    }
}

void iwStatistics::DrawAxis()
{
    // Ein paar benötigte Werte...
    const int sizeX = 180;
    const int sizeY = 80;
    const int topLeftX = this->x_ + 34;
    const int topLeftY = this->y_ + 124;
    const int topLeftXrel = 37;
    const int topLeftYrel = 124;

    // X-Achse, horizontal, war irgendwie zu lang links :S
    DrawLine(topLeftX + 6, topLeftY + sizeY + 2, // bisschen tiefer, damit man nulllinien noch sieht
             topLeftX + sizeX, topLeftY + sizeY + 1, 1, MakeColor(255, 88, 44, 16));

    // Y-Achse, vertikal
    DrawLine(topLeftX + sizeX, topLeftY,
             topLeftX + sizeX, topLeftY + sizeY + 5, 1, MakeColor(255, 88, 44, 16));

    // Striche an der Y-Achse
    DrawLine(topLeftX + sizeX - 3, topLeftY, topLeftX + sizeX + 4, topLeftY, 1, MakeColor(255, 88, 44, 16));
    DrawLine(topLeftX + sizeX - 3, topLeftY + sizeY / 2, topLeftX + sizeX + 4, topLeftY + sizeY / 2, 1, MakeColor(255, 88, 44, 16));

    // Striche an der X-Achse + Beschriftung
    // Zunächst die 0, die haben alle
    timeAnnotations[6]->Move(topLeftXrel + 180, topLeftYrel + sizeY + 6);
    timeAnnotations[6]->SetText("0");
    timeAnnotations[6]->SetVisible(true);

    switch(currentTime)
    {
        case STAT_15M:
            // -15
            DrawLine(topLeftX + 6, topLeftY + sizeY + 2,
                     topLeftX + 6, topLeftY + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[0]->Move(topLeftXrel + 6, topLeftYrel + sizeY + 6);
            timeAnnotations[0]->SetText("-15");
            timeAnnotations[0]->SetVisible(true);

            // -12
            DrawLine(topLeftX + 40, topLeftY + sizeY + 2,
                     topLeftX + 40, topLeftY + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[1]->Move(topLeftXrel + 40, topLeftYrel + sizeY + 6);
            timeAnnotations[1]->SetText("-12");
            timeAnnotations[1]->SetVisible(true);

            // -9
            DrawLine(topLeftX + 75, topLeftY + sizeY + 2,
                     topLeftX + 75, topLeftY + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[2]->Move(topLeftXrel + 75, topLeftYrel + sizeY + 6);
            timeAnnotations[2]->SetText("-9");
            timeAnnotations[2]->SetVisible(true);

            // -6
            DrawLine(topLeftX + 110, topLeftY + sizeY + 2,
                     topLeftX + 110, topLeftY + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[3]->Move(topLeftXrel + 110, topLeftYrel + sizeY + 6);
            timeAnnotations[3]->SetText("-6");
            timeAnnotations[3]->SetVisible(true);

            // -3
            DrawLine(topLeftX + 145, topLeftY + sizeY + 2,
                     topLeftX + 145, topLeftY + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[4]->Move(topLeftXrel + 145, topLeftYrel + sizeY + 6);
            timeAnnotations[4]->SetText("-3");
            timeAnnotations[4]->SetVisible(true);

            timeAnnotations[5]->SetVisible(false);
            break;
        case STAT_1H:
            // -60
            DrawLine(topLeftX + 6, topLeftY + sizeY + 2,
                     topLeftX + 6, topLeftY + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[0]->Move(topLeftXrel + 6, topLeftYrel + sizeY + 6);
            timeAnnotations[0]->SetText("-60");
            timeAnnotations[0]->SetVisible(true);

            // -50
            DrawLine(topLeftX + 35, topLeftY + sizeY + 2,
                     topLeftX + 35, topLeftY + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[1]->Move(topLeftXrel + 35, topLeftYrel + sizeY + 6);
            timeAnnotations[1]->SetText("-50");
            timeAnnotations[1]->SetVisible(true);

            // -40
            DrawLine(topLeftX + 64, topLeftY + sizeY + 2,
                     topLeftX + 64, topLeftY + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[2]->Move(topLeftXrel + 64, topLeftYrel + sizeY + 6);
            timeAnnotations[2]->SetText("-40");
            timeAnnotations[2]->SetVisible(true);

            // -30
            DrawLine(topLeftX + 93, topLeftY + sizeY + 2,
                     topLeftX + 93, topLeftY + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[3]->Move(topLeftXrel + 93, topLeftYrel + sizeY + 6);
            timeAnnotations[3]->SetText("-30");
            timeAnnotations[3]->SetVisible(true);

            // -20
            DrawLine(topLeftX + 122, topLeftY + sizeY + 2,
                     topLeftX + 122, topLeftY + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[4]->Move(topLeftXrel + 122, topLeftYrel + sizeY + 6);
            timeAnnotations[4]->SetText("-20");
            timeAnnotations[4]->SetVisible(true);

            // -10
            DrawLine(topLeftX + 151, topLeftY + sizeY + 2,
                     topLeftX + 151, topLeftY + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[5]->Move(topLeftXrel + 151, topLeftYrel + sizeY + 6);
            timeAnnotations[5]->SetText("-10");
            timeAnnotations[5]->SetVisible(true);
            break;
        case STAT_4H:
            // -240
            DrawLine(topLeftX + 6, topLeftY + sizeY + 2,
                     topLeftX + 6, topLeftY + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[0]->Move(topLeftXrel + 6, topLeftYrel + sizeY + 6);
            timeAnnotations[0]->SetText("-240");
            timeAnnotations[0]->SetVisible(true);

            // -180
            DrawLine(topLeftX + 49, topLeftY + sizeY + 2,
                     topLeftX + 49, topLeftY + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[1]->Move(topLeftXrel + 49, topLeftYrel + sizeY + 6);
            timeAnnotations[1]->SetText("-180");
            timeAnnotations[1]->SetVisible(true);

            // -120
            DrawLine(topLeftX + 93, topLeftY + sizeY + 2,
                     topLeftX + 93, topLeftY + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[2]->Move(topLeftXrel + 93, topLeftYrel + sizeY + 6);
            timeAnnotations[2]->SetText("-120");
            timeAnnotations[2]->SetVisible(true);

            // -60
            DrawLine(topLeftX + 136, topLeftY + sizeY + 2,
                     topLeftX + 136, topLeftY + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[3]->Move(topLeftXrel + 136, topLeftYrel + sizeY + 6);
            timeAnnotations[3]->SetText("-60");
            timeAnnotations[3]->SetVisible(true);

            timeAnnotations[4]->SetVisible(false);
            timeAnnotations[5]->SetVisible(false);
            break;
        case STAT_16H:
            // -960
            DrawLine(topLeftX + 6, topLeftY + sizeY + 2,
                     topLeftX + 6, topLeftY + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[0]->Move(topLeftXrel + 6, topLeftYrel + sizeY + 6);
            timeAnnotations[0]->SetText("-960");
            timeAnnotations[0]->SetVisible(true);

            // -720
            DrawLine(topLeftX + 49, topLeftY + sizeY + 2,
                     topLeftX + 49, topLeftY + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[1]->Move(topLeftXrel + 49, topLeftYrel + sizeY + 6);
            timeAnnotations[1]->SetText("-720");
            timeAnnotations[1]->SetVisible(true);

            // -480
            DrawLine(topLeftX + 93, topLeftY + sizeY + 2,
                     topLeftX + 93, topLeftY + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[2]->Move(topLeftXrel + 93, topLeftYrel + sizeY + 6);
            timeAnnotations[2]->SetText("-480");
            timeAnnotations[2]->SetVisible(true);

            // -240
            DrawLine(topLeftX + 136, topLeftY + sizeY + 2,
                     topLeftX + 136, topLeftY + sizeY + 4, 1, MakeColor(255, 88, 44, 16));
            timeAnnotations[3]->Move(topLeftXrel + 136, topLeftYrel + sizeY + 6);
            timeAnnotations[3]->SetText("-240");
            timeAnnotations[3]->SetVisible(true);

            timeAnnotations[4]->SetVisible(false);
            timeAnnotations[5]->SetVisible(false);
            break;
    }
}
