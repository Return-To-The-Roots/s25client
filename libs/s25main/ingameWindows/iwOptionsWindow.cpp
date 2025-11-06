// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
#include "controls/ctrlCheck.h"

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
    ID_btEndGame,
    ID_cpBirdSounds
};
}

iwOptionsWindow::iwOptionsWindow(SoundManager& soundManager)
    : IngameWindow(CGI_OPTIONSWINDOW, IngameWindow::posLastOrCenter, Extent(300, 545), _("Game menu"),
                   LOADER.GetImageN("resource", 41)),
      soundManager(soundManager)
{
    // The soldier on top
    AddImage(ID_imgSoldier, DrawPoint(150, 36), LOADER.GetImageN("io", 30));

    AddText(ID_txtRttr, DrawPoint(150, 60), "Return To The Roots", COLOR_YELLOW, FontStyle::CENTER, NormalFont);
    AddText(ID_txtVersion, DrawPoint(150, 77), rttr::version::GetReadableVersion(), COLOR_YELLOW, FontStyle::CENTER,
            NormalFont);
    AddFormattedText(ID_txtCopyright, DrawPoint(150, 94),
                     "\xC2\xA9"
                     "2005 - %s Settlers Freaks",
                     COLOR_YELLOW, FontStyle::CENTER, NormalFont)
      % rttr::version::GetYear();

    AddImageButton(ID_btKeyboardLayout, DrawPoint(35, 120), Extent(35, 35), TextureColor::Green2,
                   LOADER.GetImageN("io", 79));
    AddText(ID_txtKeyboardLayout, DrawPoint(85, 140), _("Keyboard layout"), COLOR_YELLOW, FontStyle::BOTTOM,
            NormalFont);
    AddImageButton(ID_btReadme, DrawPoint(35, 160), Extent(35, 35), TextureColor::Green2, LOADER.GetImageN("io", 79));
    AddText(ID_txtReadme, DrawPoint(85, 180), _("Load 'ReadMe' file"), COLOR_YELLOW, FontStyle::BOTTOM, NormalFont);

    // "Load game!"
    // TODO: Implement
    // AddImageButton( 8, DrawPoint(35, 210), Extent(35, 35), TextureColor::Green2, LOADER.GetImageN("io", 48));
    // AddText(9, DrawPoint(85, 230), _("Load game!"), COLOR_YELLOW, 0 | FontStyle::BOTTOM, NormalFont);

    // "Save game!"
    // TODO: Move back down to y=250 (Button) 270 (Text) after Load button is implemented
    AddImageButton(ID_btSave, DrawPoint(35, 230), Extent(35, 35), TextureColor::Green2, LOADER.GetImageN("io", 47));
    AddText(ID_txtSave, DrawPoint(85, 255), _("Save game!"), COLOR_YELLOW, FontStyle::BOTTOM, NormalFont);

    // Sound on/off
    AddImageButton(ID_btSoundEffects, DrawPoint(35, 300), Extent(35, 35), TextureColor::Green2,
                   LOADER.GetImageN("io", 114 + !SETTINGS.sound.effectsEnabled)); //-V807

    // Sound volume
    AddProgress(ID_pgEffectVol, DrawPoint(100, 306), Extent(160, 22), TextureColor::Green2, 139, 138, 100)
      ->SetPosition((SETTINGS.sound.effectsVolume * 100) / 255);

    // Bird sounds on/off
    AddCheckBox(ID_cpBirdSounds, DrawPoint(100, 342), Extent(160, 22), TextureColor::Green2, _("Bird sounds"), NormalFont, false)
      ->setChecked(SETTINGS.sound.birdsEnabled);

    // Music on/off
    AddImageButton(ID_btMusic, DrawPoint(35, 371), Extent(35, 35), TextureColor::Green2,
                   LOADER.GetImageN("io", 116 + !SETTINGS.sound.musicEnabled));

    // Music volume
    AddProgress(ID_pgMusicVol, DrawPoint(100, 377), Extent(160, 22), TextureColor::Green2, 139, 138, 100)
      ->SetPosition((SETTINGS.sound.musicVolume * 100) / 255);

    AddTextButton(ID_btMusicPlayer, DrawPoint(100, 413), Extent(160, 22), TextureColor::Green2, _("Music player"), NormalFont);
    AddTextButton(ID_btAdvanced, DrawPoint(67, 442), Extent(168, 24), TextureColor::Green2, _("Advanced"), NormalFont);
    AddTextButton(ID_btSurrender, DrawPoint(67, 473), Extent(168, 24), TextureColor::Red1, _("Surrender"), NormalFont);
    AddTextButton(ID_btEndGame, DrawPoint(67, 504), Extent(168, 24), TextureColor::Red1, _("End game"), NormalFont);
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
                soundManager.stopAll();
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

void iwOptionsWindow::Msg_CheckboxChange(const unsigned ctrl_id, const bool checked)
{
    switch(ctrl_id)
    {
        case ID_cpBirdSounds:
        {
            SETTINGS.sound.birdsEnabled = checked;
            break;
        }
    }
}
