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

namespace {
enum
{
    ID_imgSoldier,
    ID_txtRttr,
    ID_txtVersion,
    ID_txtCopyright,
    ID_btKeyboardLayout,
    ID_txtKeyboardLayout,
    ID_btReadme,
    ID_txtReadme,
    ID_btSave,
    ID_txtSave,
    ID_btSoundEffects,
    ID_btMusic,
    ID_pgEffectVol,
    ID_pgMusicVol,
    ID_btMusicPlayer,
    ID_btAdvanced,
    ID_btSurrender,
    ID_btEndGame
};
}

iwOptionsWindow::iwOptionsWindow()
    : IngameWindow(CGI_OPTIONSWINDOW, IngameWindow::posLastOrCenter, Extent(300, 515), _("Game menu"),
                   LOADER.GetImageN("resource", 41))
{
    // Der Soldat oben
    AddImage(ID_imgSoldier, DrawPoint(150, 36), LOADER.GetImageN("io", 30));

    AddText(ID_txtRttr, DrawPoint(150, 60), "Return To The Roots", COLOR_YELLOW, FontStyle::CENTER, NormalFont);
    AddText(ID_txtVersion, DrawPoint(150, 77), RTTR_Version::GetReadableVersion(), COLOR_YELLOW, FontStyle::CENTER,
            NormalFont);
    AddFormattedText(ID_txtCopyright, DrawPoint(150, 94),
                     "\xC2\xA9"
                     "2005 - %s Settlers Freaks",
                     COLOR_YELLOW, FontStyle::CENTER, NormalFont)
      % RTTR_Version::GetYear();

    AddImageButton(ID_btKeyboardLayout, DrawPoint(35, 120), Extent(35, 35), TC_GREEN2, LOADER.GetImageN("io", 79));
    AddText(ID_txtKeyboardLayout, DrawPoint(85, 140), _("Keyboard layout"), COLOR_YELLOW, FontStyle::BOTTOM,
            NormalFont);
    AddImageButton(ID_btReadme, DrawPoint(35, 160), Extent(35, 35), TC_GREEN2, LOADER.GetImageN("io", 79));
    AddText(ID_txtReadme, DrawPoint(85, 180), _("Load 'ReadMe' file"), COLOR_YELLOW, FontStyle::BOTTOM, NormalFont);

    // "Spiel laden!"
    // TODO: Implement
    // AddImageButton( 8, DrawPoint(35, 210), Extent(35, 35), TC_GREEN2, LOADER.GetImageN("io", 48));
    // AddText(9, DrawPoint(85, 230), _("Load game!"), COLOR_YELLOW, 0 | FontStyle::BOTTOM, NormalFont);

    // "Spiel speichern!"
    // TODO: Move back down to y=250 (Button) 270 (Text) after Load button is implemented
    AddImageButton(ID_btSave, DrawPoint(35, 230), Extent(35, 35), TC_GREEN2, LOADER.GetImageN("io", 47));
    AddText(ID_txtSave, DrawPoint(85, 255), _("Save game!"), COLOR_YELLOW, FontStyle::BOTTOM, NormalFont);

    // Geräusche an/aus
    AddImageButton(ID_btSoundEffects, DrawPoint(35, 300), Extent(35, 35), TC_GREEN2,
                   LOADER.GetImageN("io", 114 + !SETTINGS.sound.effectsEnabled)); //-V807

    // Musik an/aus
    AddImageButton(ID_btMusic, DrawPoint(35, 340), Extent(35, 35), TC_GREEN2,
                   LOADER.GetImageN("io", 116 + !SETTINGS.sound.musicEnabled));

    // Geräuschlautstärke
    AddProgress(ID_pgEffectVol, DrawPoint(100, 306), Extent(160, 22), TC_GREEN2, 139, 138, 100)
      ->SetPosition((SETTINGS.sound.effectsVolume * 100) / 255);

    // Musiklautstärke
    AddProgress(ID_pgMusicVol, DrawPoint(100, 346), Extent(160, 22), TC_GREEN2, 139, 138, 100)
      ->SetPosition((SETTINGS.sound.musicVolume * 100) / 255);

    AddTextButton(ID_btMusicPlayer, DrawPoint(100, 380), Extent(160, 22), TC_GREEN2, _("Music player"), NormalFont);
    AddTextButton(ID_btAdvanced, DrawPoint(67, 412), Extent(168, 24), TC_GREEN2, _("Advanced"), NormalFont);
    AddTextButton(ID_btSurrender, DrawPoint(67, 443), Extent(168, 24), TC_RED1, _("Surrender"), NormalFont);
    AddTextButton(ID_btEndGame, DrawPoint(67, 474), Extent(168, 24), TC_RED1, _("End game"), NormalFont);
}

void iwOptionsWindow::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_btEndGame:
            WINDOWMANAGER.ToggleWindow(std::make_unique<iwEndgame>());
            Close();
            break;
        case ID_btKeyboardLayout:
            WINDOWMANAGER.ToggleWindow(std::make_unique<iwTextfile>("keyboardlayout.txt", _("Keyboard layout")));
            break;
        case ID_btReadme: WINDOWMANAGER.ToggleWindow(std::make_unique<iwTextfile>("readme.txt", _("Readme!"))); break;
        case ID_btSave: WINDOWMANAGER.ToggleWindow(std::make_unique<iwSave>()); break;

        case ID_btSoundEffects:
            SETTINGS.sound.effectsEnabled = !SETTINGS.sound.effectsEnabled; //-V807
            GetCtrl<ctrlImageButton>(ID_btSoundEffects)
              ->SetImage(LOADER.GetTextureN("io", 114 + !SETTINGS.sound.effectsEnabled));

            if(!SETTINGS.sound.effectsEnabled)
                SOUNDMANAGER.StopAll();
            break;

        case ID_btMusic:
            SETTINGS.sound.musicEnabled = !SETTINGS.sound.musicEnabled;
            GetCtrl<ctrlImageButton>(ID_btMusic)
              ->SetImage(LOADER.GetTextureN("io", 116 + !SETTINGS.sound.musicEnabled));
            if(SETTINGS.sound.musicEnabled)
                MUSICPLAYER.Play();
            else
                MUSICPLAYER.Stop();
            break;
        case ID_btMusicPlayer: WINDOWMANAGER.ToggleWindow(std::make_unique<iwMusicPlayer>()); break;
        case ID_btSurrender:
            WINDOWMANAGER.ReplaceWindow(std::make_unique<iwSurrender>());
            Close();
            break;
        case ID_btAdvanced:
            WINDOWMANAGER.ReplaceWindow(std::make_unique<iwSettings>());
            Close();
            break;
    }
}

void iwOptionsWindow::Msg_ProgressChange(const unsigned ctrl_id, const unsigned short position)
{
    switch(ctrl_id)
    {
        case ID_pgEffectVol:
            SETTINGS.sound.effectsVolume = static_cast<uint8_t>((position * 255) / 100);
            AUDIODRIVER.SetMasterEffectVolume(SETTINGS.sound.effectsVolume);
            break;
        case ID_pgMusicVol:
            SETTINGS.sound.musicVolume = static_cast<uint8_t>((position * 255) / 100);
            AUDIODRIVER.SetMusicVolume(SETTINGS.sound.musicVolume);
            break;
    }
}
