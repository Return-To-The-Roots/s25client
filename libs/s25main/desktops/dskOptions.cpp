// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dskOptions.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "MusicPlayer.h"
#include "Settings.h"
#include "WindowManager.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlGroup.h"
#include "controls/ctrlImageButton.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlProgress.h"
#include "controls/ctrlTextButton.h"
#include "driver/VideoDriver.h"
#include "drivers/AudioDriverWrapper.h"
#include "drivers/VideoDriverWrapper.h"
#include "dskMainMenu.h"
#include "helpers/containerUtils.h"
#include "helpers/format.hpp"
#include "helpers/mathFuncs.h"
#include "helpers/toString.h"
#include "ingameWindows/iwAddons.h"
#include "ingameWindows/iwMsgbox.h"
#include "ingameWindows/iwMusicPlayer.h"
#include "ingameWindows/iwTextfile.h"
#include "languages.h"
#include "ogl/FontStyle.h"
#include "gameData/PortraitConsts.h"
#include "s25util/StringConversion.h"
#include "s25util/colors.h"
#include <mygettext/mygettext.h>
#include <sstream>

namespace {
using Offset = DrawPoint;

enum
{
    ID_btBack = dskMenuBase::ID_FIRST_FREE,
    ID_txtOptions,
    ID_btAddons,
    ID_grpOptions,
    ID_btCommon,
    ID_btGraphics,
    ID_btSound,
    ID_grpCommon,
    ID_grpGraphics,
    ID_grpSound,
    ID_txtName,
    ID_edtName,
    ID_txtLanguage,
    ID_cbLanguage,
    ID_txtKeyboardLayout,
    ID_btKeyboardLayout,
    ID_txtPort,
    ID_edtPort,
    ID_txtIpv6,
    ID_grpIpv6,
    ID_txtProxy,
    ID_edtProxy,
    ID_edtProxyPort,
    ID_txtProxyType,
    ID_cbProxyType,
    ID_txtDebugData,
    ID_grpDebugData,
    ID_grpUPNP,
    ID_txtMapScrollMode,
    ID_cbMapScrollMode,
    ID_grpSmartCursor,
    ID_grpWindowPinning,
    ID_grpGFInfo,
    ID_grpResolution,
    ID_txtResolution,
    ID_cbResolution,
    ID_grpWindowSize,
    ID_txtWindowSize,
    ID_cbWindowSize,
    ID_txtDisplayMode,
    ID_cbDisplayMode,
    ID_grpLockWindowSize,
    ID_txtFramerate,
    ID_cbFramerate,
    ID_grpVBO,
    ID_txtVideoDriver,
    ID_cbVideoDriver,
    ID_grpOptTextures,
    ID_txtGuiScale,
    ID_cbGuiScale,
    ID_txtAudioDriver,
    ID_cbAudioDriver,
    ID_grpMusic,
    ID_pgMusicVol,
    ID_grpEffects,
    ID_pgEffectsVol,
    ID_btMusicPlayer,
    ID_txtCommonPortrait,
    ID_btCommonPortrait,
    ID_cbCommonPortrait,
    ID_grpBirdSounds,
    ID_txtsStart, // First ID for texts used by options (ID of option is used as offset on top)
};
// Use these as IDs in dedicated groups
constexpr auto ID_btOn = 1;
constexpr auto ID_btOff = 0;
// Special case: Submit debug data uses "2" for "ask user" and "0" for "unset, ask at start"
constexpr auto ID_btSubmitDebugOn = 1;
constexpr auto ID_btSubmitDebugAsk = 2;

constexpr auto rowHeight = 30;
constexpr auto sectionSpacing = 20;
constexpr auto sectionSpacingCommon = 10;
constexpr auto optionRowsStartPosition = DrawPoint(80, 75);
constexpr auto tabButtonsStartPosition = optionRowsStartPosition + Offset(0, static_cast<int>(rowHeight * 15.5));

constexpr Offset ctrlOffset(200, -5);                       // Offset of control to its description text
constexpr Offset ctrlOffset2 = ctrlOffset + Offset(200, 0); // Offset of 2nd control to its description text
constexpr Extent ctrlSize(190, 22);
constexpr Extent ctrlSizeLarge = ctrlSize + Extent(ctrlOffset2 - ctrlOffset);

VideoMode getAspectRatio(const VideoMode vm)
{
    // First some a bit off values where the aspect ratio is defined by convention
    if(vm == VideoMode(1360, 1024))
        return VideoMode(4, 3);
    else if(vm == VideoMode(1360, 768) || vm == VideoMode(1366, 768))
        return VideoMode(16, 9);

    // Normally Aspect ration is simply width/height as integer numbers (e.g. 4:3)
    int divisor = helpers::gcd(vm.width, vm.height);
    VideoMode ratio(vm.width / divisor, vm.height / divisor);
    // But there are some special cases:
    if(ratio == VideoMode(8, 5))
        return VideoMode(16, 10);
    else if(ratio == VideoMode(5, 3))
        return VideoMode(15, 9);
    else
        return ratio;
}

enum class ButtonSize
{
    Normal,
    Half /// Both buttons take up the same space as a single button
};

ctrlOptionGroup& addOnOffOption(ctrlGroup& parentGroup, DrawPoint pos, unsigned id, const std::string& title,
                                bool value, ButtonSize btSize = ButtonSize::Normal)
{
    constexpr auto spacing = 10;
    auto size = ctrlSize;
    if(btSize == ButtonSize::Half)
        size.x = (ctrlSize.x - spacing) / 2;

    parentGroup.addText(ID_txtsStart + id, pos, title, COLOR_YELLOW, FontStyle{}, NormalFont);
    auto* option = parentGroup.addOptionGroup(id, GroupSelectType::Check);
    pos += ctrlOffset;
    option->addTextButton(ID_btOn, pos, size, TextureColor::Grey, _("On"), NormalFont);
    pos.x += size.x + spacing;
    option->addTextButton(ID_btOff, pos, size, TextureColor::Grey, _("Off"), NormalFont);
    option->setSelection(value);
    return *option;
}

} // namespace

dskOptions::dskOptions() : Desktop(LOADER.getImageN("setup013", 0))
{
    addText(ID_txtOptions, DrawPoint(400, 10), _("Options"), COLOR_YELLOW, FontStyle::CENTER, LargeFont);

    ctrlOptionGroup* mainGroup = addOptionGroup(ID_grpOptions, GroupSelectType::Check);

    DrawPoint curPos = tabButtonsStartPosition;
    mainGroup->addTextButton(ID_btCommon, DrawPoint(curPos.x, curPos.y), Extent(200, 22), TextureColor::Green2,
                             _("Common"), NormalFont);
    mainGroup->addTextButton(ID_btGraphics, DrawPoint(curPos.x + 220, curPos.y), Extent(200, 22), TextureColor::Green2,
                             _("Graphics"), NormalFont);
    mainGroup->addTextButton(ID_btSound, DrawPoint(curPos.x + 440, curPos.y), Extent(200, 22), TextureColor::Green2,
                             _("Sound/Music"), NormalFont);
    curPos.y += rowHeight;

    addTextButton(ID_btBack, DrawPoint(curPos.x + 220, curPos.y), Extent(200, 22), TextureColor::Red1, _("Back"),
                  NormalFont);
    addTextButton(ID_btAddons, DrawPoint(curPos.x + 440, curPos.y), Extent(200, 22), TextureColor::Green2, _("Addons"),
                  NormalFont);

    ctrlGroup* groupCommon = addGroup(ID_grpCommon);
    ctrlGroup* groupGraphics = addGroup(ID_grpGraphics);
    ctrlGroup* groupSound = addGroup(ID_grpSound);
    ctrlComboBox* combo;

    // Common
    // {

    curPos = optionRowsStartPosition;

    groupCommon->addText(ID_txtName, curPos, _("Name in Game:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    ctrlEdit* name =
      groupCommon->addEdit(ID_edtName, curPos + ctrlOffset, ctrlSize, TextureColor::Grey, NormalFont, 15);
    name->setText(SETTINGS.lobby.name);

    const auto& currentPortrait = Portraits[SETTINGS.lobby.portraitIndex];
    groupCommon->addImageButton(ID_btCommonPortrait, DrawPoint(500, curPos.y + ctrlOffset.y), Extent(40, rowHeight * 2),
                                TextureColor::Grey,
                                LOADER.getImageN(currentPortrait.resourceId, currentPortrait.resourceIndex));
    curPos.y += rowHeight;

    groupCommon->addText(ID_txtCommonPortrait, curPos, _("Portrait:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo =
      groupCommon->addComboBox(ID_cbCommonPortrait, curPos + ctrlOffset, ctrlSize, TextureColor::Grey, NormalFont, 100);

    for(unsigned i = 0; i < Portraits.size(); ++i)
    {
        combo->addItem(_(Portraits[i].name));
        if(SETTINGS.lobby.portraitIndex == i)
            combo->setSelection(i);
    }
    curPos.y += rowHeight;

    groupCommon->addText(ID_txtLanguage, curPos, _("Language:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo = groupCommon->addComboBox(ID_cbLanguage, curPos + ctrlOffset, ctrlSize, TextureColor::Grey, NormalFont, 100);

    bool selected = false;
    for(unsigned i = 0; i < LANGUAGES.size(); ++i)
    {
        const Language& l = LANGUAGES.getLanguage(i);

        combo->addItem(_(l.name));
        if(SETTINGS.language.language == l.code)
        {
            combo->setSelection(static_cast<unsigned short>(i));
            selected = true;
        }
    }
    if(!selected)
        combo->setSelection(0);
    curPos.y += rowHeight;

    groupCommon->addTextButton(ID_btKeyboardLayout, curPos + ctrlOffset, ctrlSizeLarge, TextureColor::Grey,
                               _("Keyboard layout"), NormalFont);
    curPos.y += rowHeight;

    curPos.y += sectionSpacingCommon;
    groupCommon->addText(ID_txtPort, curPos, _("Local Port:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    ctrlEdit* edtPort =
      groupCommon->addEdit(ID_edtPort, curPos + ctrlOffset, ctrlSize, TextureColor::Grey, NormalFont, 15);
    edtPort->setType(EditType::Number);
    edtPort->setText(SETTINGS.server.localPort);
    curPos.y += rowHeight;

    // IPv4/6
    auto& grpIPv6 = addOnOffOption(*groupCommon, curPos, ID_grpIpv6, _("Use IPv6:"), SETTINGS.server.ipv6);
    grpIPv6.getCtrl<ctrlTextButton>(ID_btOn)->setText(_("IPv6"));
    grpIPv6.getCtrl<ctrlTextButton>(ID_btOff)->setText(_("IPv4"));
    // Enable/disable the IPv6 field if necessary
    grpIPv6.getCtrl<ctrlButton>(ID_btOn)->setEnabled(SETTINGS.proxy.type != ProxyType::Socks5); //-V807
    curPos.y += rowHeight;

    curPos.y += sectionSpacingCommon;
    // Proxy server
    groupCommon->addText(ID_txtProxy, curPos, _("Proxyserver:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    ctrlEdit* proxy = groupCommon->addEdit(ID_edtProxy, curPos + ctrlOffset, ctrlSize, TextureColor::Grey, NormalFont);
    proxy->setText(SETTINGS.proxy.hostname);
    proxy =
      groupCommon->addEdit(ID_edtProxyPort, curPos + ctrlOffset2, Extent(50, 22), TextureColor::Grey, NormalFont, 5);
    proxy->setType(EditType::Number);
    proxy->setText(SETTINGS.proxy.port);
    curPos.y += rowHeight;

    addOnOffOption(*groupCommon, curPos, ID_grpUPNP, _("Use UPnP"), SETTINGS.global.useUPNP);
    curPos.y += rowHeight;

    // Proxy type
    groupCommon->addText(ID_txtProxyType, curPos, _("Proxytyp:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo =
      groupCommon->addComboBox(ID_cbProxyType, curPos + ctrlOffset, ctrlSizeLarge, TextureColor::Grey, NormalFont, 100);
    combo->addItem(_("No Proxy"));
    combo->addItem(_("Socks v4"));
    // TODO: not implemented
    // combo->AddString(_("Socks v5"));

    switch(SETTINGS.proxy.type)
    {
        default: combo->setSelection(0); break;
        case ProxyType::Socks4: combo->setSelection(1); break;
        case ProxyType::Socks5: combo->setSelection(2); break;
    }
    curPos.y += rowHeight;

    curPos.y += sectionSpacingCommon;
    groupCommon->addText(ID_txtMapScrollMode, curPos, _("Map scroll mode:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo = groupCommon->addComboBox(ID_cbMapScrollMode, curPos + ctrlOffset, ctrlSizeLarge, TextureColor::Grey,
                                     NormalFont, 100);
    combo->addItem(_("Scroll same (Map moves in the same direction the mouse is moved when scrolling/panning.)"));
    combo->addItem(
      _("Scroll opposite (Map moves in the opposite direction the mouse is moved when scrolling/panning.)"));
    combo->addItem(_("Grab and drag (Map moves with your cursor when scrolling/panning.)"));
    combo->setSelection(static_cast<int>(SETTINGS.interface.mapScrollMode));
    curPos.y += rowHeight;

    auto& grpSmartCursor =
      addOnOffOption(*groupCommon, curPos, ID_grpSmartCursor, _("Smart Cursor"), SETTINGS.global.smartCursor);
    grpSmartCursor.getCtrl<ctrlButton>(ID_btOn)->setTooltip(
      _("Place cursor on default button for new dialogs / action windows (default)"));
    grpSmartCursor.getCtrl<ctrlButton>(ID_btOff)->setTooltip(
      _("Don't move cursor automatically\nUseful e.g. for split-screen / dual-mice multiplayer (see wiki)"));
    curPos.y += rowHeight;

    auto& grpWindowPinning = addOnOffOption(*groupCommon, curPos, ID_grpWindowPinning, _("Window pinning"),
                                            SETTINGS.interface.enableWindowPinning);
    grpWindowPinning.getCtrl<ctrlButton>(ID_btOn)->setTooltip(
      _("Replace minimize button on windows by pin button avoiding closing the window with "
        "ESC.\nMinimize by double-clicking the title bar."));
    curPos.y += rowHeight;

    curPos.y += sectionSpacingCommon;
    groupCommon->addText(ID_txtDebugData, curPos, _("Submit debug data:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    mainGroup = groupCommon->addOptionGroup(ID_grpDebugData, GroupSelectType::Check);
    mainGroup->addTextButton(ID_btSubmitDebugOn, curPos + ctrlOffset, ctrlSize, TextureColor::Grey, _("On"),
                             NormalFont);
    mainGroup->addTextButton(ID_btSubmitDebugAsk, curPos + ctrlOffset2, ctrlSize, TextureColor::Grey, _("Ask always"),
                             NormalFont);

    mainGroup->setSelection((SETTINGS.global.submitDebugData == SubmitDebugData::Yes) ? ID_btSubmitDebugOn :
                                                                                        ID_btSubmitDebugAsk); //-V807
    curPos.y += rowHeight;

    addOnOffOption(*groupCommon, curPos, ID_grpGFInfo, _("Show GameFrame Info:"), SETTINGS.global.showGFInfo);

    curPos = optionRowsStartPosition;

    groupGraphics->addText(ID_txtDisplayMode, curPos, _("Mode:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    ctrlComboBox* cbDisplayMode = groupGraphics->addComboBox(ID_cbDisplayMode, curPos + ctrlOffset, ctrlSizeLarge,
                                                             TextureColor::Grey, NormalFont, 100);
    cbDisplayMode->addItem(_("Windowed"));
    cbDisplayMode->addItem(_("Fullscreen"));
    cbDisplayMode->addItem(_("Borderless window"));
    cbDisplayMode->setSelection(rttr::enumCast(SETTINGS.video.displayMode.type));
    curPos.y += rowHeight;

    auto* grpResolution = groupGraphics->addGroup(ID_grpResolution);
    grpResolution->addText(ID_txtResolution, curPos, _("Fullscreen resolution:"), COLOR_YELLOW, FontStyle{},
                           NormalFont);
    grpResolution->addComboBox(ID_cbResolution, curPos + ctrlOffset, ctrlSize, TextureColor::Grey, NormalFont, 150);
    auto* grpWindowSize = groupGraphics->addGroup(ID_grpWindowSize);
    grpWindowSize->addText(ID_txtWindowSize, curPos, _("Window size:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    grpWindowSize->addComboBox(ID_cbWindowSize, curPos + ctrlOffset, ctrlSize, TextureColor::Grey, NormalFont, 150);
    curPos.y += rowHeight;

    addOnOffOption(*grpWindowSize, curPos, ID_grpLockWindowSize, _("Lock window size:"),
                   !SETTINGS.video.displayMode.resizeable);
    curPos.y += rowHeight;

    curPos.y += sectionSpacing;
    groupGraphics->addText(ID_txtFramerate, curPos, _("Limit Framerate:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    groupGraphics->addComboBox(ID_cbFramerate, curPos + ctrlOffset, ctrlSizeLarge, TextureColor::Grey, NormalFont, 150);
    curPos.y += rowHeight;

    curPos.y += sectionSpacing;
    addOnOffOption(*groupGraphics, curPos, ID_grpVBO, _("Vertex Buffer Objects:"), SETTINGS.video.vbo);
    curPos.y += rowHeight;

    curPos.y += sectionSpacing;
    groupGraphics->addText(ID_txtVideoDriver, curPos, _("Graphics Driver"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo = groupGraphics->addComboBox(ID_cbVideoDriver, curPos + ctrlOffset, ctrlSizeLarge, TextureColor::Grey,
                                       NormalFont, 100);

    const auto video_drivers = drivers::DriverWrapper::loadDriverList(drivers::DriverType::Video);

    for(const auto& video_driver : video_drivers)
    {
        combo->addItem(video_driver.getName());
        if(video_driver.getName() == SETTINGS.driver.video)
            combo->setSelection(combo->getNumItems() - 1);
    }
    curPos.y += rowHeight;

    curPos.y += sectionSpacing;
    addOnOffOption(*groupGraphics, curPos, ID_grpOptTextures, _("Optimized Textures:"), SETTINGS.video.sharedTextures);
    curPos.y += rowHeight;

    curPos.y += sectionSpacing;
    groupGraphics->addText(ID_txtGuiScale, curPos, _("GUI Scale:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    groupGraphics->addComboBox(ID_cbGuiScale, curPos + ctrlOffset, ctrlSize, TextureColor::Grey, NormalFont, 100);
    updateGuiScale();

    curPos = optionRowsStartPosition;

    addOnOffOption(*groupSound, curPos, ID_grpEffects, _("Effects"), SETTINGS.sound.effectsEnabled, ButtonSize::Half);

    ctrlProgress* FXvolume =
      groupSound->addProgress(ID_pgEffectsVol, curPos + ctrlOffset2, ctrlSize, TextureColor::Grey, 139, 138, 100);
    FXvolume->setPosition((SETTINGS.sound.effectsVolume * 100) / 255);
    curPos.y += rowHeight;

    curPos.y += sectionSpacing;
    addOnOffOption(*groupSound, curPos, ID_grpBirdSounds, _("Bird sounds"), SETTINGS.sound.birdsEnabled,
                   ButtonSize::Half);
    curPos.y += rowHeight;

    curPos.y += sectionSpacing;
    addOnOffOption(*groupSound, curPos, ID_grpMusic, _("Music"), SETTINGS.sound.musicEnabled, ButtonSize::Half);

    ctrlProgress* Mvolume =
      groupSound->addProgress(ID_pgMusicVol, curPos + ctrlOffset2, ctrlSize, TextureColor::Grey, 139, 138, 100);
    Mvolume->setPosition((SETTINGS.sound.musicVolume * 100) / 255); //-V807
    curPos.y += rowHeight;

    curPos.y += sectionSpacing;
    groupSound->addTextButton(ID_btMusicPlayer, curPos + ctrlOffset, ctrlSize, TextureColor::Grey, _("Music player"),
                              NormalFont);
    curPos.y += rowHeight;

    curPos.y += sectionSpacing;
    groupSound->addText(ID_txtAudioDriver, curPos, _("Sounddriver"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo = groupSound->addComboBox(ID_cbAudioDriver, curPos + ctrlOffset, ctrlSizeLarge, TextureColor::Grey,
                                    NormalFont, 100);

    const auto audio_drivers = drivers::DriverWrapper::loadDriverList(drivers::DriverType::Audio);

    for(const auto& audio_driver : audio_drivers)
    {
        combo->addItem(audio_driver.getName());
        if(audio_driver.getName() == SETTINGS.driver.audio)
            combo->setSelection(combo->getNumItems() - 1);
    }

    // Select "General"
    mainGroup = getCtrl<ctrlOptionGroup>(ID_grpOptions);
    mainGroup->setSelection(ID_btCommon, true);

    // Graphics
    // {

    loadVideoModes();

    // and add to the combo box
    ctrlComboBox& cbVideoModes = *grpResolution->getCtrl<ctrlComboBox>(ID_cbResolution);
    for(const auto& videoMode : videoModes_)
    {
        VideoMode ratio = getAspectRatio(videoMode);
        s25util::ClassicImbuedStream<std::ostringstream> str;
        str << videoMode.width << "x" << videoMode.height;
        // Make the length always the same as 'iiiixiiii' to align the ratio
        int len = str.str().length();
        for(int i = len; i < 4 + 1 + 4; i++)
            str << " ";
        str << " (" << ratio.width << ":" << ratio.height << ")";

        cbVideoModes.addItem(str.str());

        // Select, if this is the current resolution
        if(videoMode == SETTINGS.video.fullscreenSize) //-V807
            cbVideoModes.setSelection(cbVideoModes.getNumItems() - 1);
    }
    ctrlComboBox& cbWindowSize = *grpWindowSize->getCtrl<ctrlComboBox>(ID_cbWindowSize);
    cbWindowSize.addItem(""); // Placeholder for current window size
    for(const auto& size : windowSizes_)
    {
        s25util::ClassicImbuedStream<std::ostringstream> str;
        str << size.width << "x" << size.height;
        cbWindowSize.addItem(str.str());
    }
    updateWindowSizeComboBox();
    updateResolutionGroups();

    // Fill "Limit Framerate"
    auto* cbFrameRate = groupGraphics->getCtrl<ctrlComboBox>(ID_cbFramerate);
    if(VIDEODRIVER.hasVSync())
        cbFrameRate->addItem(_("Dynamic (Limits to display refresh rate, works with most drivers)"));
    for(int framerate : Settings::SCREEN_REFRESH_RATES)
    {
        if(framerate == -1)
            cbFrameRate->addItem(_("Disabled"));
        else
            cbFrameRate->addItem(helpers::toString(framerate) + " FPS");
        if(SETTINGS.video.framerate == framerate)
            cbFrameRate->setSelection(cbFrameRate->getNumItems() - 1);
    }
    if(!cbFrameRate->getSelection())
        cbFrameRate->setSelection(0);

    // }

    // Sound
    // {

    groupSound->getCtrl<ctrlOptionGroup>(ID_grpEffects)->setSelection(SETTINGS.sound.effectsEnabled);
    groupSound->getCtrl<ctrlOptionGroup>(ID_grpBirdSounds)->setSelection(SETTINGS.sound.birdsEnabled);
    groupSound->getCtrl<ctrlOptionGroup>(ID_grpMusic)->setSelection(SETTINGS.sound.musicEnabled);

    // }

    // Load game settings
    ggs.loadSettings();
}

dskOptions::~dskOptions()
{
    // Save game settings
    ggs.saveSettings();
}

void dskOptions::msgGroupProgressChange(const unsigned /*group_id*/, const unsigned ctrl_id,
                                        const unsigned short position)
{
    switch(ctrl_id)
    {
        case ID_pgEffectsVol:
            SETTINGS.sound.effectsVolume = static_cast<uint8_t>((position * 255) / 100);
            AUDIODRIVER.setMasterEffectVolume(SETTINGS.sound.effectsVolume);
            break;
        case ID_pgMusicVol:
            SETTINGS.sound.musicVolume = static_cast<uint8_t>((position * 255) / 100);
            AUDIODRIVER.setMusicVolume(SETTINGS.sound.musicVolume);
            break;
    }
}

void dskOptions::msgGroupComboSelectItem(const unsigned group_id, const unsigned ctrl_id, const unsigned selection)
{
    auto* group = getCtrl<ctrlGroup>(group_id);
    auto* combo = group->getCtrl<ctrlComboBox>(ctrl_id);

    switch(ctrl_id)
    {
        case ID_cbCommonPortrait:
            SETTINGS.lobby.portraitIndex = selection;
            updatePortraitControls();
            break;
        case ID_cbLanguage:
        {
            // Language changed?
            std::string old_lang = SETTINGS.language.language; //-V807
            SETTINGS.language.language = LANGUAGES.setLanguage(selection);
            if(SETTINGS.language.language != old_lang)
                WINDOWMANAGER.switchTo(std::make_unique<dskOptions>());
        }
        break;
        case ID_cbProxyType:
            switch(selection)
            {
                case 0: SETTINGS.proxy.type = ProxyType::None; break;
                case 1: SETTINGS.proxy.type = ProxyType::Socks4; break;
                case 2: SETTINGS.proxy.type = ProxyType::Socks5; break;
            }

            // Disable IPv6 visually
            if(SETTINGS.proxy.type == ProxyType::Socks4 && SETTINGS.server.ipv6)
            {
                getCtrl<ctrlGroup>(ID_grpCommon)->getCtrl<ctrlOptionGroup>(ID_grpIpv6)->setSelection(0);
                getCtrl<ctrlGroup>(ID_grpCommon)
                  ->getCtrl<ctrlOptionGroup>(ID_grpIpv6)
                  ->getCtrl<ctrlButton>(1)
                  ->setEnabled(false);
                SETTINGS.server.ipv6 = false;
            }

            if(SETTINGS.proxy.type != ProxyType::Socks4)
                getCtrl<ctrlGroup>(ID_grpCommon)
                  ->getCtrl<ctrlOptionGroup>(ID_grpIpv6)
                  ->getCtrl<ctrlButton>(1)
                  ->setEnabled(true);
            break;
        case ID_cbMapScrollMode: SETTINGS.interface.mapScrollMode = static_cast<MapScrollMode>(selection); break;
        case ID_cbResolution: SETTINGS.video.fullscreenSize = videoModes_[selection]; break;
        case ID_cbWindowSize:
            if(selection > 0)
            {
                SETTINGS.video.windowedSize = windowSizes_[selection - 1];
                if(SETTINGS.video.displayMode == DisplayMode::Windowed
                   && VIDEODRIVER.getDisplayMode() == DisplayMode::Windowed)
                    VIDEODRIVER.resizeScreen(SETTINGS.video.windowedSize, DisplayMode::Windowed);
            }
            break;
        case ID_cbFramerate:
            if(VIDEODRIVER.hasVSync())
            {
                if(selection == 0)
                    SETTINGS.video.framerate = 0;
                else
                    SETTINGS.video.framerate = Settings::SCREEN_REFRESH_RATES[selection - 1];
            } else
                SETTINGS.video.framerate = Settings::SCREEN_REFRESH_RATES[selection];

            VIDEODRIVER.setTargetFramerate(SETTINGS.video.framerate);
            break;
        case ID_cbVideoDriver: SETTINGS.driver.video = combo->getText(selection); break;
        case ID_cbGuiScale:
            SETTINGS.video.guiScale = guiScales_[selection];
            VIDEODRIVER.setGuiScalePercent(SETTINGS.video.guiScale);
            break;
        case ID_cbAudioDriver: SETTINGS.driver.audio = combo->getText(selection); break;
        case ID_cbDisplayMode:
            SETTINGS.video.displayMode = DisplayMode(selection);
            updateResolutionGroups();
            break;
    }
}

void dskOptions::msgGroupOptionGroupChange(const unsigned /*group_id*/, const unsigned ctrl_id,
                                           const unsigned selection)
{
    const bool enabled = selection == ID_btOn;
    switch(ctrl_id)
    {
        case ID_grpIpv6: SETTINGS.server.ipv6 = enabled; break;
        case ID_grpLockWindowSize:
        {
            SETTINGS.video.displayMode.resizeable = !enabled;
            const auto newDisplayMode = SETTINGS.video.displayMode;
            if(newDisplayMode == DisplayMode::Windowed && VIDEODRIVER.getDisplayMode() == DisplayMode::Windowed)
                VIDEODRIVER.resizeScreen(VIDEODRIVER.getWindowSize(), newDisplayMode);
        }
        break;
        case ID_grpVBO: SETTINGS.video.vbo = enabled; break;
        case ID_grpOptTextures: SETTINGS.video.sharedTextures = enabled; break;
        case ID_grpEffects: SETTINGS.sound.effectsEnabled = enabled; break;
        case ID_grpBirdSounds: SETTINGS.sound.birdsEnabled = enabled; break;
        case ID_grpMusic:
            SETTINGS.sound.musicEnabled = enabled;
            if(enabled)
                MUSICPLAYER.play();
            else
                MUSICPLAYER.stop();
            break;
        case ID_grpDebugData:
            // Special case: Uses e.g. ID_btSubmitDebugOn directly
            SETTINGS.global.submitDebugData = SubmitDebugData(selection);
            break;
        case ID_grpUPNP: SETTINGS.global.useUPNP = enabled; break;
        case ID_grpSmartCursor:
            SETTINGS.global.smartCursor = enabled;
            VIDEODRIVER.setMouseWarping(enabled);
            break;
        case ID_grpWindowPinning: SETTINGS.interface.enableWindowPinning = enabled; break;
        case ID_grpGFInfo: SETTINGS.global.showGFInfo = enabled; break;
        default: RTTR_Assert(false);
    }
}

void dskOptions::msgOptionGroupChange(const unsigned ctrl_id, const unsigned selection)
{
    if(ctrl_id == ID_grpOptions)
    {
        const auto visGrp = selection + ID_grpCommon - ID_btCommon;
        for(const unsigned id : {ID_grpCommon, ID_grpGraphics, ID_grpSound})
            getCtrl<ctrlGroup>(id)->setVisible(id == visGrp);
    }
}

/// Check that the port is valid and sets outPort to it. Shows an error otherwise
static bool validatePort(const std::string& sPort, uint16_t& outPort)
{
    std::optional<uint16_t> port = validate::checkPort(sPort);
    if(port)
        outPort = *port;
    else
    {
        WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Error"),
                                                      _("Invalid port. The valid port-range is 1 to 65535!"), nullptr,
                                                      MsgboxButton::Ok, MsgboxIcon::ExclamationRed, 1));
    }
    return static_cast<bool>(port);
}

void dskOptions::msgButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_btBack:
        {
            auto* groupCommon = getCtrl<ctrlGroup>(ID_grpCommon);

            // Save the name
            SETTINGS.lobby.name = groupCommon->getCtrl<ctrlEdit>(ID_edtName)->getText();
            if(!validatePort(groupCommon->getCtrl<ctrlEdit>(ID_edtPort)->getText(), SETTINGS.server.localPort))
                return;

            SETTINGS.proxy.hostname = groupCommon->getCtrl<ctrlEdit>(ID_edtProxy)->getText();
            if(!validatePort(groupCommon->getCtrl<ctrlEdit>(ID_edtProxyPort)->getText(), SETTINGS.proxy.port))
                return;

            SETTINGS.save();

            // Is the selected backend required to support GUI scaling to fulfill the user's choice?
            // If so, warn the user if the backend is unable to support GUI scaling.
            const auto requestedGuiScale = SETTINGS.video.guiScale;
            const bool autoGuiScale = requestedGuiScale == 0;
            if((!autoGuiScale && requestedGuiScale != VIDEODRIVER.getGuiScale().percent())
               || (autoGuiScale
                   && VIDEODRIVER.getGuiScaleRange().recommendedPercent != VIDEODRIVER.getGuiScale().percent()))

            {
                WINDOWMANAGER.show(std::make_unique<iwMsgbox>(
                  _("Sorry!"), _("The selected video driver does not support GUI scaling! Setting won't be used."),
                  this, MsgboxButton::Ok, MsgboxIcon::ExclamationGreen, 1));
            }

            const auto fullscreen = SETTINGS.video.displayMode == DisplayMode::Fullscreen;
            if((fullscreen && SETTINGS.video.fullscreenSize != VIDEODRIVER.getWindowSize()) //-V807
               || VIDEODRIVER.getDisplayMode() != SETTINGS.video.displayMode)
            {
                const auto screenSize = fullscreen ? SETTINGS.video.fullscreenSize : SETTINGS.video.windowedSize;
                if(!VIDEODRIVER.resizeScreen(screenSize, SETTINGS.video.displayMode))
                {
                    WINDOWMANAGER.show(std::make_unique<iwMsgbox>(
                      _("Sorry!"), _("You need to restart your game to change the screen resolution!"), this,
                      MsgboxButton::Ok, MsgboxIcon::ExclamationGreen, 1));
                    return;
                }
            }
            if(SETTINGS.driver.video != VIDEODRIVER.getName() || SETTINGS.driver.audio != AUDIODRIVER.getName())
            {
                WINDOWMANAGER.show(std::make_unique<iwMsgbox>(
                  _("Sorry!"), _("You need to restart your game to change the video or audio driver!"), this,
                  MsgboxButton::Ok, MsgboxIcon::ExclamationGreen, 1));
                return;
            }

            WINDOWMANAGER.switchTo(std::make_unique<dskMainMenu>());
        }
        break;
        case ID_btAddons: WINDOWMANAGER.toggleWindow(std::make_unique<iwAddons>(ggs)); break;
    }
}

void dskOptions::msgGroupButtonClick(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        default: break;
        case ID_btCommonPortrait:
            SETTINGS.lobby.portraitIndex = (SETTINGS.lobby.portraitIndex + 1) % Portraits.size();
            updatePortraitControls();
            break;
        case ID_btMusicPlayer: WINDOWMANAGER.toggleWindow(std::make_unique<iwMusicPlayer>()); break;
        case ID_btKeyboardLayout:
            WINDOWMANAGER.toggleWindow(std::make_unique<iwTextfile>("keyboardlayout.txt", _("Keyboard layout")));
            break;
    }
}

void dskOptions::msgMsgBoxResult(const unsigned msgbox_id, const MsgboxResult /*mbr*/)
{
    switch(msgbox_id)
    {
        default: break;
        // "You need to restart your game ..."
        // "The selected video driver does not support GUI scaling!"
        case 1: WINDOWMANAGER.switchTo(std::make_unique<dskMainMenu>()); break;
    }
}

void dskOptions::loadVideoModes()
{
    // Get available modes
    const auto videoModes = VIDEODRIVER.listVideoModes();
    // random access is need for selection
    videoModes_.assign(videoModes.begin(), videoModes.end());
    const auto windowSizes = VIDEODRIVER.getDefaultWindowSizes();
    windowSizes_.assign(windowSizes.begin(), windowSizes.end());
}

void dskOptions::msgScreenResize(const ScreenResizeEvent& sr)
{
    Desktop::msgScreenResize(sr);
    updateGuiScale();
    updateWindowSizeComboBox();
}

bool dskOptions::msgWheelUp(const MouseCoords& mc)
{
    if(VIDEODRIVER.getModKeyState().ctrl)
    {
        scrollGuiScale(true);
        return true;
    } else
        return Desktop::msgWheelUp(mc);
}

bool dskOptions::msgWheelDown(const MouseCoords& mc)
{
    if(VIDEODRIVER.getModKeyState().ctrl)
    {
        scrollGuiScale(false);
        return true;
    } else
        return Desktop::msgWheelDown(mc);
}

void dskOptions::updateGuiScale()
{
    // generate GUI scale percentages in 10% increments
    constexpr auto stepSize = 10u;
    const auto roundGuiScale = [=](unsigned percent) {
        return helpers::iround<unsigned>(static_cast<float>(percent) / stepSize) * stepSize;
    };

    const auto range = VIDEODRIVER.getGuiScaleRange();
    const auto recommendedPercentRounded = roundGuiScale(range.recommendedPercent);
    auto* combo = getCtrl<ctrlGroup>(ID_grpGraphics)->getCtrl<ctrlComboBox>(ID_cbGuiScale);

    guiScales_.clear();
    combo->deleteAllItems();

    guiScales_.push_back(0);
    combo->addItem(helpers::format(_("Auto (%u%%)"), range.recommendedPercent));
    if(SETTINGS.video.guiScale == 0)
        combo->setSelection(0);

    for(unsigned percent = roundGuiScale(range.minPercent); percent <= range.maxPercent; percent += stepSize)
    {
        if(percent == recommendedPercentRounded)
            recommendedGuiScaleIndex_ = guiScales_.size();
        guiScales_.push_back(percent);

        combo->addItem(helpers::toString(percent) + "%");
        if(percent == SETTINGS.video.guiScale)
            combo->setSelection(combo->getNumItems() - 1);
    }

    // if GUI scale exceeds maximum, lower it to keep UI elements on screen
    if(SETTINGS.video.guiScale > guiScales_.back())
    {
        combo->setSelection(combo->getNumItems() - 1);
        SETTINGS.video.guiScale = guiScales_.back();
        VIDEODRIVER.setGuiScalePercent(SETTINGS.video.guiScale);
    }
}

void dskOptions::scrollGuiScale(bool up)
{
    auto* combo = getCtrl<ctrlGroup>(ID_grpGraphics)->getCtrl<ctrlComboBox>(ID_cbGuiScale);
    const auto& selection = combo->getSelection();
    unsigned newSelection = 0;
    if(!selection || *selection == 0) // No selection or "Auto" item selected
        newSelection = recommendedGuiScaleIndex_;
    else
        newSelection = std::clamp<unsigned>(*selection + (up ? 1 : -1), 1, combo->getNumItems() - 1);

    if(newSelection != selection)
    {
        combo->setSelection(newSelection);
        SETTINGS.video.guiScale = guiScales_[newSelection];
        VIDEODRIVER.setGuiScalePercent(SETTINGS.video.guiScale);
    }
}

void dskOptions::updatePortraitControls()
{
    const auto& newPortrait = Portraits[SETTINGS.lobby.portraitIndex];
    auto* groupCommon = getCtrl<ctrlGroup>(ID_grpCommon);

    auto* portraitButton = groupCommon->getCtrl<ctrlImageButton>(ID_btCommonPortrait);
    auto* newPortraitTexture = LOADER.getTextureN(newPortrait.resourceId, newPortrait.resourceIndex);
    portraitButton->setImage(newPortraitTexture);

    auto* portraitCombo = groupCommon->getCtrl<ctrlComboBox>(ID_cbCommonPortrait);
    portraitCombo->setSelection(SETTINGS.lobby.portraitIndex);
}

void dskOptions::updateWindowSizeComboBox()
{
    auto* cbWindowSize =
      getCtrl<ctrlGroup>(ID_grpGraphics)->getCtrl<ctrlGroup>(ID_grpWindowSize)->getCtrl<ctrlComboBox>(ID_cbWindowSize);

    const VideoMode currentSize =
      VIDEODRIVER.getDisplayMode() == DisplayMode::Windowed ? VIDEODRIVER.getWindowSize() : SETTINGS.video.windowedSize;
    s25util::ClassicImbuedStream<std::ostringstream> curStr;
    curStr << currentSize.width << "x" << currentSize.height;
    cbWindowSize->setText(0, curStr.str());
    // Not found == -1 -> First entry selected
    cbWindowSize->setSelection(static_cast<unsigned>(helpers::indexOf(windowSizes_, currentSize) + 1));
}

void dskOptions::updateResolutionGroups()
{
    const auto currentDisplayMode = SETTINGS.video.displayMode;
    auto& grpGraphics = *getCtrl<ctrlGroup>(ID_grpGraphics);
    grpGraphics.getCtrl<ctrlGroup>(ID_grpResolution)->setVisible(currentDisplayMode == DisplayMode::Fullscreen);
    grpGraphics.getCtrl<ctrlGroup>(ID_grpWindowSize)->setVisible(currentDisplayMode == DisplayMode::Windowed);
}
