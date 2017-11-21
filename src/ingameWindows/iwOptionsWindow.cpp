// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "rttrDefines.h" // IWYU pragma: keep
#include "iwOptionsWindow.h"
#include "Loader.h"
#include "MusicPlayer.h"
#include "RTTR_Version.h"
#include "Settings.h"
#include "SoundManager.h"
#include "WindowManager.h"
#include "controls/ctrlImageButton.h"
#include "controls/ctrlProgress.h"
#include "drivers/AudioDriverWrapper.h"
#include "iwEndgame.h"
#include "iwMusicPlayer.h"
#include "iwSave.h"
#include "iwSettings.h"
#include "iwSurrender.h"
#include "iwTextfile.h"
#include "ogl/FontStyle.h"
#include "gameData/const_gui_ids.h"

iwOptionsWindow::iwOptionsWindow()
    : IngameWindow(CGI_OPTIONSWINDOW, IngameWindow::posLastOrCenter, Extent(300, 515), _("Game menu"), LOADER.GetImageN("resource", 41))
{
    // Der Soldat oben
    AddImage(1, DrawPoint(150, 36), LOADER.GetImageN("io", 30));

    // Versionszeile
    AddFormattedText(2, DrawPoint(150, 76), _("Return To The Roots - %1%"), COLOR_YELLOW, FontStyle::CENTER | FontStyle::BOTTOM, NormalFont)
      % RTTR_Version::GetReadableVersion();
    // Copyright
    AddFormattedText(3, DrawPoint(150, 96), _("© 2005 - %s Settlers Freaks"), COLOR_YELLOW, FontStyle::CENTER | FontStyle::BOTTOM,
                     NormalFont)
      % RTTR_Version::GetYear();

    // "Tastaturbelegung"
    AddImageButton(4, DrawPoint(35, 120), Extent(35, 35), TC_GREEN2, LOADER.GetImageN("io", 79));
    AddText(5, DrawPoint(85, 140), _("Keyboard layout"), COLOR_YELLOW, 0 | FontStyle::BOTTOM, NormalFont);

    // "'Lies mich'-Datei laden"
    AddImageButton(6, DrawPoint(35, 160), Extent(35, 35), TC_GREEN2, LOADER.GetImageN("io", 79));
    AddText(7, DrawPoint(85, 180), _("Load 'ReadMe' file"), COLOR_YELLOW, 0 | FontStyle::BOTTOM, NormalFont);

    // "Spiel laden!"
    // TODO: Implement
    // AddImageButton( 8, DrawPoint(35, 210), Extent(35, 35), TC_GREEN2, LOADER.GetImageN("io", 48));
    // AddText(9, DrawPoint(85, 230), _("Load game!"), COLOR_YELLOW, 0 | FontStyle::BOTTOM, NormalFont);

    // "Spiel speichern!"
    // TODO: Move back down to y=250 (Button) 270 (Text) after Load button is implemented
    AddImageButton(10, DrawPoint(35, 230), Extent(35, 35), TC_GREEN2, LOADER.GetImageN("io", 47));
    AddText(11, DrawPoint(85, 255), _("Save game!"), COLOR_YELLOW, 0 | FontStyle::BOTTOM, NormalFont);

    // Geräusche an/aus
    AddImageButton(12, DrawPoint(35, 300), Extent(35, 35), TC_GREEN2, LOADER.GetImageN("io", 114 + !SETTINGS.sound.effekte)); //-V807

    // Musik an/aus
    AddImageButton(13, DrawPoint(35, 340), Extent(35, 35), TC_GREEN2, LOADER.GetImageN("io", 116 + !SETTINGS.sound.musik));

    // Geräuschlautstärke
    AddProgress(14, DrawPoint(100, 306), Extent(160, 22), TC_GREEN2, 139, 138, 10)->SetPosition(SETTINGS.sound.effekte_volume * 10 / 255);

    // Musiklautstärke
    AddProgress(15, DrawPoint(100, 346), Extent(160, 22), TC_GREEN2, 139, 138, 10)->SetPosition(SETTINGS.sound.musik_volume * 10 / 255);

    //// Music Player
    AddTextButton(16, DrawPoint(100, 380), Extent(160, 22), TC_GREEN2, _("Music player"), NormalFont);

    // Advanced Options
    AddTextButton(18, DrawPoint(67, 412), Extent(168, 24), TC_GREEN2, _("Advanced"), NormalFont);

    // "Spiel aufgeben"
    AddTextButton(17, DrawPoint(67, 443), Extent(168, 24), TC_RED1, _("Surrender"), NormalFont);
    // "Spiel beenden"
    AddTextButton(0, DrawPoint(67, 474), Extent(168, 24), TC_RED1, _("End game"), NormalFont);
}

void iwOptionsWindow::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 0: // "Spiel beenden"
        {
            WINDOWMANAGER.Show(new iwEndgame);
            Close();
        }
        break;
        case 4: // "Tastaturbelegung laden"
        {
            WINDOWMANAGER.Show(new iwTextfile("keyboardlayout.txt", _("Keyboard layout")));
        }
        break;
        case 6: // "'Lies mich'-Datei laden"
        {
            WINDOWMANAGER.Show(new iwTextfile("readme.txt", _("Readme!")));
        }
        break;
        case 10: // "Spiel speichern"
        {
            WINDOWMANAGER.Show(new iwSave);
        }
        break;

        case 12: // Geräusche an/aus
        {
            SETTINGS.sound.effekte = !SETTINGS.sound.effekte; //-V807
            GetCtrl<ctrlImageButton>(12)->SetImage(LOADER.GetImageN("io", 114 + !SETTINGS.sound.effekte));

            if(!SETTINGS.sound.effekte)
                SOUNDMANAGER.StopAll();
        }
        break;

        case 13: // Musik an/aus
        {
            SETTINGS.sound.musik = !SETTINGS.sound.musik;
            GetCtrl<ctrlImageButton>(13)->SetImage(LOADER.GetImageN("io", 116 + !SETTINGS.sound.musik));
            if(SETTINGS.sound.musik)
                MUSICPLAYER.Play();
            else
                MUSICPLAYER.Stop();
        }
        break;
        case 16: // Music player
        {
            WINDOWMANAGER.Show(new iwMusicPlayer);
        }
        break;
        case 17: // Aufgeben
        {
            WINDOWMANAGER.Show(new iwSurrender);
            Close();
        }
        break;
        case 18: // Advanced
        {
            WINDOWMANAGER.Show(new iwSettings());
            Close();
        }
        break;
    }
}

void iwOptionsWindow::Msg_ProgressChange(const unsigned ctrl_id, const unsigned short position)
{
    switch(ctrl_id)
    {
        case 14:
        {
            SETTINGS.sound.effekte_volume = (unsigned char)position * 255 / 10 + (position < 10 ? 1 : 0);
            AUDIODRIVER.SetMasterEffectVolume(SETTINGS.sound.effekte_volume);
        }
        break;
        case 15:
        {
            SETTINGS.sound.musik_volume = (unsigned char)position * 255 / 10 + (position < 10 ? 1 : 0);
            AUDIODRIVER.SetMusicVolume(SETTINGS.sound.musik_volume);
        }
        break;
    }
}
