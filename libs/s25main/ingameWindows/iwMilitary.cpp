// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwMilitary.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "WindowManager.h"
#include "addons/const_addons.h"
#include "controls/ctrlProgress.h"
#include "iwHelp.h"
#include "network/GameClient.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldViewer.h"
#include "gameData/SettingTypeConv.h"
#include "gameData/const_gui_ids.h"

namespace {
enum
{
    ID_btHelp,
    ID_btDefault,
    ID_Offset, // ID of control corresponding to MILITARY_SETTINGS_SCALE[0]
};
}

iwMilitary::iwMilitary(const GameWorldViewer& gwv, GameCommandFactory& gcFactory)
    : TransmitSettingsIgwAdapter(CGI_MILITARY, IngameWindow::posLastOrCenter, Extent(168, 330), _("Military"),
                                 LOADER.GetImageN("io", 5)),
      gcFactory(gcFactory)
{
    // Setting bars
    constexpr Extent progSize(132, 26);
    constexpr Extent progPadding(4, 4);
    constexpr unsigned progSpacing = 6;
    DrawPoint curPos((GetSize().x - progSize.x) / 2, 25); // Same space left and right
    AddProgress(ID_Offset, curPos, progSize, TextureColor::Grey, 119, 120, MILITARY_SETTINGS_SCALE[0], "", progPadding,
                0, _("Fewer recruits"), _("More recruits")); /* pitch: progPadding */
    curPos.y += progSize.y + progSpacing;
    AddProgress(ID_Offset + 1, curPos, progSize, TextureColor::Grey, 121, 122, MILITARY_SETTINGS_SCALE[1], "",
                progPadding, 0, _("Weak defense"), _("Strong defense"));
    curPos.y += progSize.y + progSpacing;
    auto* ctrl = AddProgress(ID_Offset + 2, curPos, progSize, TextureColor::Grey, 123, 124, MILITARY_SETTINGS_SCALE[2],
                             "", progPadding, 0, _("Fewer defenders"), _("More defenders"));
    // Hide defender setting if changing defender behavior is disabled
    if(gwv.GetWorld().GetGGS().getSelection(AddonId::DEFENDER_BEHAVIOR) == 1)
        ctrl->SetVisible(false);
    else
        curPos.y += progSize.y + progSpacing;
    AddProgress(ID_Offset + 3, curPos, progSize, TextureColor::Grey, 209, 210, MILITARY_SETTINGS_SCALE[3], "",
                progPadding, 0, _("Less attackers"), _("More attackers"));
    curPos.y += progSize.y + progSpacing;
    AddProgress(ID_Offset + 4, curPos, progSize, TextureColor::Grey, 129, 130, MILITARY_SETTINGS_SCALE[4], "",
                progPadding, 0, _("Interior"), _("Interior"));
    curPos.y += progSize.y + progSpacing;
    AddProgress(ID_Offset + 5, curPos, progSize, TextureColor::Grey, 127, 128, MILITARY_SETTINGS_SCALE[5], "",
                progPadding, 0, _("Center of country"), _("Center of country"));
    curPos.y += progSize.y + progSpacing;
    ctrl = AddProgress(ID_Offset + 6, curPos, progSize, TextureColor::Grey, 1000, 1001, MILITARY_SETTINGS_SCALE[6], "",
                       progPadding, 0, _("Near harbor points"), _("Near harbor points"));
    // Hide changing harbor area if sea attacks are disabled
    if(!gwv.GetWorld().GetGGS().isEnabled(AddonId::SEA_ATTACK))
        ctrl->SetVisible(false);
    else
        curPos.y += progSize.y + progSpacing;
    AddProgress(ID_Offset + 7, curPos, progSize, TextureColor::Grey, 125, 126, MILITARY_SETTINGS_SCALE[7], "",
                progPadding, 0, _("Border areas"), _("Border areas"));

    // Buttons at bottom
    constexpr Extent buttonSize(30, 32);
    curPos.y = GetRightBottomBoundary().y - buttonSize.y - 5;
    AddImageButton(ID_btHelp, curPos, buttonSize, TextureColor::Grey, LOADER.GetImageN("io", 225), _("Help"));
    curPos.x += progSize.x - buttonSize.x;
    AddImageButton(ID_btDefault, curPos, buttonSize, TextureColor::Grey, LOADER.GetImageN("io", 191), _("Default"));

    iwMilitary::UpdateSettings();
}

void iwMilitary::TransmitSettings()
{
    if(GAMECLIENT.IsReplayModeOn())
        return;

    if(settings_changed)
    {
        // Save settings
        MilitarySettings milSettings = GAMECLIENT.visual_settings.military_settings;
        for(unsigned char i = 0; i < milSettings.size(); ++i)
            milSettings[i] = (unsigned char)GetCtrl<ctrlProgress>(ID_Offset + i)->GetPosition();

        if(gcFactory.ChangeMilitary(milSettings))
        {
            GAMECLIENT.visual_settings.military_settings = milSettings;
            settings_changed = false;
        }
    }
}

void iwMilitary::Msg_ProgressChange(const unsigned /*ctrl_id*/, const unsigned short /*position*/)
{
    settings_changed = true;
}

void iwMilitary::UpdateSettings(const MilitarySettings& military_settings)
{
    if(GAMECLIENT.IsReplayModeOn())
        GAMECLIENT.ResetVisualSettings();
    for(unsigned i = 0; i < military_settings.size(); ++i)
        GetCtrl<ctrlProgress>(ID_Offset + i)->SetPosition(military_settings[i]);
}

void iwMilitary::UpdateSettings()
{
    UpdateSettings(GAMECLIENT.visual_settings.military_settings);
}

void iwMilitary::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        default: return;
        case ID_btHelp:
        {
            WINDOWMANAGER.ReplaceWindow(
              std::make_unique<iwHelp>(_("This is where you can make adjustments to all military matters. "
                                         "The upper value corresponds to the recruiting rate of your army. "
                                         "The higher it is, the more inhabitants are recruited as soldiers. "
                                         "Below this is the setting to protect your huts. If this value is "
                                         "set at maximum, your huts are defended by the strongest unit. To "
                                         "raise the number of attackers leaving your huts per attack, choose "
                                         "the next setting. The number of defenders who counter the enemy in "
                                         "the event of an attack is shown by the fourth display. The final "
                                         "three values correspond to the occupation of your huts in the "
                                         "interior, in the center of the country and on its borders.")));
        }
        break;
        case ID_btDefault:
        {
            UpdateSettings(GAMECLIENT.default_settings.military_settings);
            settings_changed = true;
        }
        break;
    }
}
