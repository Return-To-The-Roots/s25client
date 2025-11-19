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
#include "controls/ctrlCheck.h"
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
    ID_btLoad,
    ID_txtLoad,
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

constexpr auto windowSize = Extent(300, 545);
constexpr auto centerPosition = windowSize.x / 2;
constexpr auto rowHeight = 40;
constexpr auto rowHeightSmall = 31;
constexpr auto textSpacing = 17;
constexpr auto textPadding = 35;
constexpr auto headerStartPosition = DrawPoint(centerPosition, 10);
} // namespace

iwOptionsWindow::iwOptionsWindow(SoundManager& soundManager)
    : IngameWindow(CGI_OPTIONSWINDOW, IngameWindow::posLastOrCenter, windowSize, _("Game menu"),
                   LOADER.GetImageN("resource", 41)),
      soundManager(soundManager)
{
    using Offset = DrawPoint;
    constexpr Extent buttonImageSize(35, 35);
    constexpr Extent optionSizeSmall(160, 22);
    constexpr Extent optionSizeBig(168, 24);
    constexpr Offset textOffset(50, 24);
    constexpr Offset optionOffset(65, 6);

    DrawPoint curPos = headerStartPosition;

    // The soldier on top
    constexpr Offset soldierOffset(0, 26);
    AddImage(ID_imgSoldier, curPos + soldierOffset, LOADER.GetImageN("io", 30));
    curPos.y += textSpacing + soldierOffset.y;

    AddText(ID_txtRttr, curPos, "Return To The Roots", COLOR_YELLOW, FontStyle::CENTER, NormalFont);
    curPos.y += textSpacing;

    AddText(ID_txtVersion, curPos, rttr::version::GetReadableVersion(), COLOR_YELLOW, FontStyle::CENTER, NormalFont);
    curPos.y += textSpacing;

    AddFormattedText(ID_txtCopyright, curPos,
                     "\xC2\xA9"
                     "2005 - %s Settlers Freaks",
                     COLOR_YELLOW, FontStyle::CENTER, NormalFont)
      % rttr::version::GetYear();
    curPos.y += rowHeightSmall;
    curPos.x = textPadding;

    AddImageButton(ID_btKeyboardLayout, curPos, buttonImageSize, TextureColor::Green2, LOADER.GetImageN("io", 79));
    AddText(ID_txtKeyboardLayout, curPos + textOffset, _("Keyboard layout"), COLOR_YELLOW, FontStyle::BOTTOM,
            NormalFont);
    curPos.y += rowHeight;

    AddImageButton(ID_btReadme, curPos, buttonImageSize, TextureColor::Green2, LOADER.GetImageN("io", 79));
    AddText(ID_txtReadme, curPos + textOffset, _("Load 'ReadMe' file"), COLOR_YELLOW, FontStyle::BOTTOM, NormalFont);
    curPos.y += rowHeight;

    // TODO: Implement
    // AddImageButton(ID_btLoad, curPos, buttonImageSize, TextureColor::Green2, LOADER.GetImageN("io", 48));
    // AddText(ID_txtLoad, curPos + textOffset, _("Load game!"), COLOR_YELLOW, FontStyle::BOTTOM, NormalFont);
    // curPos.y += rowHeight;

    curPos.y += rowHeight / 2; // TODO: Delete this row, if the Load button is implemented
    AddImageButton(ID_btSave, curPos, buttonImageSize, TextureColor::Green2, LOADER.GetImageN("io", 47));
    AddText(ID_txtSave, curPos + textOffset, _("Save game!"), COLOR_YELLOW, FontStyle::BOTTOM, NormalFont);
    curPos.y += rowHeight / 2; // TODO: Delete this row, if the Load button is implemented
    curPos.y += rowHeight;

    // Sound on/off + volume
    AddImageButton(ID_btSoundEffects, curPos, buttonImageSize, TextureColor::Green2,
                   LOADER.GetImageN("io", 114 + !SETTINGS.sound.effectsEnabled)); //-V807
    AddProgress(ID_pgEffectVol, curPos + optionOffset, optionSizeSmall, TextureColor::Green2, 139, 138, 100)
      ->SetPosition((SETTINGS.sound.effectsVolume * 100) / 255);
    curPos.y += rowHeight;

    AddCheckBox(ID_cpBirdSounds, curPos + optionOffset, optionSizeSmall, TextureColor::Green2, _("Bird sounds"),
                NormalFont, false)
      ->setChecked(SETTINGS.sound.birdsEnabled);
    curPos.y += rowHeight;

    // Music on/off + volume
    AddImageButton(ID_btMusic, curPos, buttonImageSize, TextureColor::Green2,
                   LOADER.GetImageN("io", 116 + !SETTINGS.sound.musicEnabled));
    AddProgress(ID_pgMusicVol, curPos + optionOffset, optionSizeSmall, TextureColor::Green2, 139, 138, 100)
      ->SetPosition((SETTINGS.sound.musicVolume * 100) / 255);
    curPos.y += rowHeight;

    AddTextButton(ID_btMusicPlayer, curPos + optionOffset, optionSizeSmall, TextureColor::Green2, _("Music player"),
                  NormalFont);
    curPos.y += rowHeight;

    // Buttons at the bottom
    curPos.x = centerPosition;
    constexpr Offset btOffset(-(optionSizeBig.x / 2), 0);
    AddTextButton(ID_btAdvanced, curPos + btOffset, optionSizeBig, TextureColor::Green2, _("Advanced"), NormalFont);
    curPos.y += rowHeightSmall;
    AddTextButton(ID_btSurrender, curPos + btOffset, optionSizeBig, TextureColor::Red1, _("Surrender"), NormalFont);
    curPos.y += rowHeightSmall;
    AddTextButton(ID_btEndGame, curPos + btOffset, optionSizeBig, TextureColor::Red1, _("End game"), NormalFont);
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
