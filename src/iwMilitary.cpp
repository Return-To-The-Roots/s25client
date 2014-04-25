// $Id: iwMilitary.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "iwMilitary.h"

#include "Loader.h"
#include "controls.h"
#include "GameClient.h"
#include "GameCommands.h"


///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p iwMilitary.
 *
 *  @author OLiver
 */
iwMilitary::iwMilitary(void)
    : IngameWindow(CGI_MILITARY, 0xFFFE, 0xFFFE, 168, 330, _("Military"), LOADER.GetImageN("io", 5)),
      settings_changed(false)
{
    // Einzelne Balken
    AddProgress(0, 17, 25, 132, 26, TC_GREY, 119, 120, MILITARY_SETTINGS_SCALE[0], "", 4, 4, 0, _("Fewer recruits"), _("More recruits")); /* pitch: 4, 4 */
    AddProgress(1, 17, 57, 132, 26, TC_GREY, 121, 122, MILITARY_SETTINGS_SCALE[1], "", 4, 4, 0, _("Weak defense"), _("Strong defense"));
    AddProgress(2, 17, 89, 132, 26, TC_GREY, 123, 124, MILITARY_SETTINGS_SCALE[2], "", 4, 4, 0, _("Fewer defenders"), _("More defenders"));
    AddProgress(3, 17, 121, 132, 26, TC_GREY, 209, 210, MILITARY_SETTINGS_SCALE[3], "", 4, 4, 0, _("Less attackers"), _("More attackers"));
    AddProgress(4, 17, 153, 132, 26, TC_GREY, 129, 130, MILITARY_SETTINGS_SCALE[4], "", 4, 4, 0, _("Interior"), _("Interior"));
    AddProgress(5, 17, 185, 132, 26, TC_GREY, 127, 128, MILITARY_SETTINGS_SCALE[5], "", 4, 4, 0, _("Center of country"), _("Center of country"));
    AddProgress(6, 17, 217, 132, 26, TC_GREY, 1000, 1001, MILITARY_SETTINGS_SCALE[6], "", 4, 4, 0, _("Near harbor points"), _("Near harbor points"));

    AddProgress(7, 17, 249, 132, 26, TC_GREY, 125, 126, MILITARY_SETTINGS_SCALE[7], "", 4, 4, 0, _("Border areas"), _("Border areas"));

    // unteren 2 Buttons
    AddImageButton(20, 18, 282, 30, 32, TC_GREY, LOADER.GetImageN("io", 21), _("Help"));
    AddImageButton(21, 120, 282, 30, 32, TC_GREY, LOADER.GetImageN("io", 191), _("Default"));

    // Falls Verteidiger ändern verboten ist, einfach die Bar ausblenden
    if (GameClient::inst().GetGGS().getSelection(ADDON_DEFENDER_BEHAVIOR) == 1)
    {
        GetCtrl<ctrlProgress>(2)->SetVisible(false);
    }

    // Absendetimer, in 2s-Abschnitten wird jeweils das ganze als Netzwerknachricht ggf. abgeschickt
    AddTimer(22, 2000);
    UpdateSettings();
}

iwMilitary::~iwMilitary()
{
    TransmitSettings();
}

/// Sendet veränderte Einstellungen (an den Client), falls sie verändert wurden
void iwMilitary::TransmitSettings()
{
    // Wurden Einstellungen geändert?
    if(settings_changed)
    {
        // Einstellungen speichern
        for(unsigned char i = 0; i < MILITARY_SETTINGS_COUNT; ++i)
            GAMECLIENT.visual_settings.military_settings[i] = (unsigned char)GetCtrl<ctrlProgress>(i)->GetPosition();

        GAMECLIENT.AddGC(new gc::ChangeMilitary(GAMECLIENT.visual_settings.military_settings));
        settings_changed = false;
    }
}


void iwMilitary::Msg_Timer(const unsigned int ctrl_id)
{
    if(GAMECLIENT.IsReplayModeOn())
        // Im Replay aktualisieren wir die Werte
        UpdateSettings();
    else
        // Im normalen Spielmodus schicken wir den ganzen Spaß ab
        TransmitSettings();
}

void iwMilitary::Msg_ProgressChange(const unsigned int ctrl_id, const unsigned short position)
{

    // Einstellungen wurden geändert
    settings_changed = true;
}

void iwMilitary::UpdateSettings()
{
    // Einstellungen festlegen
    for(unsigned i = 0; i < MILITARY_SETTINGS_COUNT; ++i)
        GetCtrl<ctrlProgress>(i)->SetPosition(GAMECLIENT.visual_settings.military_settings[i]);
}

void iwMilitary::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        default: return;
            // Default button
        case 21:
        {
            GAMECLIENT.visual_settings.military_settings = GAMECLIENT.default_settings.military_settings;
            UpdateSettings();
            settings_changed = true;
        } break;
    }
}

///////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////

