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

iwMilitary::iwMilitary(const GameWorldViewer& gwv, GameCommandFactory& gcFactory)
    : IngameWindow(CGI_MILITARY, IngameWindow::posLastOrCenter, Extent(168, 330), _("Military"), LOADER.GetImageN("io", 5)), gwv(gwv),
      gcFactory(gcFactory), settings_changed(false)
{
    // Einzelne Balken
    const Extent progSize(132, 26);
    const Extent progPadding(4, 4);
    AddProgress(0, DrawPoint(17, 25), progSize, TC_GREY, 119, 120, MILITARY_SETTINGS_SCALE[0], "", progPadding, 0, _("Fewer recruits"),
                _("More recruits")); /* pitch: progPadding */
    AddProgress(1, DrawPoint(17, 57), progSize, TC_GREY, 121, 122, MILITARY_SETTINGS_SCALE[1], "", progPadding, 0, _("Weak defense"),
                _("Strong defense"));
    AddProgress(2, DrawPoint(17, 89), progSize, TC_GREY, 123, 124, MILITARY_SETTINGS_SCALE[2], "", progPadding, 0, _("Fewer defenders"),
                _("More defenders"));
    AddProgress(3, DrawPoint(17, 121), progSize, TC_GREY, 209, 210, MILITARY_SETTINGS_SCALE[3], "", progPadding, 0, _("Less attackers"),
                _("More attackers"));
    AddProgress(4, DrawPoint(17, 153), progSize, TC_GREY, 129, 130, MILITARY_SETTINGS_SCALE[4], "", progPadding, 0, _("Interior"),
                _("Interior"));
    AddProgress(5, DrawPoint(17, 185), progSize, TC_GREY, 127, 128, MILITARY_SETTINGS_SCALE[5], "", progPadding, 0, _("Center of country"),
                _("Center of country"));
    AddProgress(6, DrawPoint(17, 217), progSize, TC_GREY, 1000, 1001, MILITARY_SETTINGS_SCALE[6], "", progPadding, 0,
                _("Near harbor points"), _("Near harbor points"));
    AddProgress(7, DrawPoint(17, 249), progSize, TC_GREY, 125, 126, MILITARY_SETTINGS_SCALE[7], "", progPadding, 0, _("Border areas"),
                _("Border areas"));

    // unteren 2 Buttons
    AddImageButton(20, DrawPoint(18, 282), Extent(30, 32), TC_GREY, LOADER.GetImageN("io", 225), _("Help"));
    AddImageButton(21, DrawPoint(120, 282), Extent(30, 32), TC_GREY, LOADER.GetImageN("io", 191), _("Default"));

    // Falls Verteidiger ändern verboten ist, einfach die Bar ausblenden
    if(gwv.GetWorld().GetGGS().getSelection(AddonId::DEFENDER_BEHAVIOR) == 1)
    {
        GetCtrl<ctrlProgress>(2)->SetVisible(false);
    }

    // Absendetimer, in 2s-Abschnitten wird jeweils das ganze als Netzwerknachricht ggf. abgeschickt
    AddTimer(22, 2000);
    UpdateSettings();
}

iwMilitary::~iwMilitary()
{
    try
    {
        TransmitSettings();
    } catch(...)
    {
        // Ignore
    }
}

/// Sendet veränderte Einstellungen (an den Client), falls sie verändert wurden
void iwMilitary::TransmitSettings()
{
    if(GAMECLIENT.IsReplayModeOn())
        return;

    // Wurden Einstellungen geändert?
    if(settings_changed)
    {
        // Einstellungen speichern
        MilitarySettings& milSettings = GAMECLIENT.visual_settings.military_settings;
        for(unsigned char i = 0; i < milSettings.size(); ++i)
            milSettings[i] = (unsigned char)GetCtrl<ctrlProgress>(i)->GetPosition();

        gcFactory.ChangeMilitary(milSettings);
        settings_changed = false;
    }
}

void iwMilitary::Msg_Timer(const unsigned /*ctrl_id*/)
{
    if(GAMECLIENT.IsReplayModeOn())
        // Im Replay aktualisieren wir die Werte
        UpdateSettings();
    else
        // Im normalen Spielmodus schicken wir den ganzen Spaß ab
        TransmitSettings();
}

void iwMilitary::Msg_ProgressChange(const unsigned /*ctrl_id*/, const unsigned short /*position*/)
{
    // Einstellungen wurden geändert
    settings_changed = true;
}

void iwMilitary::UpdateSettings()
{
    if(GAMECLIENT.IsReplayModeOn())
        gwv.GetPlayer().FillVisualSettings(GAMECLIENT.visual_settings);
    for(unsigned i = 0; i < GAMECLIENT.visual_settings.military_settings.size(); ++i)
        GetCtrl<ctrlProgress>(i)->SetPosition(GAMECLIENT.visual_settings.military_settings[i]);
}

void iwMilitary::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        default:
            return;
        // Default button
        case 20:
        {
            WINDOWMANAGER.ReplaceWindow(std::make_unique<iwHelp>(_("This is where you can make adjustments to all military matters. "
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
        case 21:
        {
            GAMECLIENT.visual_settings.military_settings = GAMECLIENT.default_settings.military_settings;
            UpdateSettings();
            settings_changed = true;
        }
        break;
    }
}
