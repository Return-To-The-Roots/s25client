// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#include "defines.h" // IWYU pragma: keep
#include "iwTools.h"

#include "Loader.h"
#include "GameClient.h"
#include "controls/ctrlDeepening.h"
#include "controls/ctrlProgress.h"
#include "gameData/const_gui_ids.h"
#include "libutil/src/colors.h"
#include <iostream>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

iwTools::iwTools()
    : IngameWindow(CGI_TOOLS, 0xFFFE, 0xFFFE, 166 + (GAMECLIENT.GetGGS().isEnabled(AddonId::TOOL_ORDERING) ? 46 : 0), 432, _("Tools"), LOADER.GetImageN("io", 5)),
      settings_changed(false)
{
    // Einzelne Balken
    AddProgress( 0, 17,  25, 132, 26, TC_GREY, 141, 140, 10, _("Tongs"), 4, 4, 0, _("Less often"), _("More often"));
    AddProgress( 1, 17,  53, 132, 26, TC_GREY, 145, 144, 10, _("Axe"), 4, 4, 0, _("Less often"), _("More often"));
    AddProgress( 2, 17,  81, 132, 26, TC_GREY, 147, 146, 10, _("Saw"), 4, 4, 0, _("Less often"), _("More often"));
    AddProgress( 3, 17, 109, 132, 26, TC_GREY, 149, 148, 10, _("Pick-axe"), 4, 4, 0, _("Less often"), _("More often"));
    AddProgress( 4, 17, 137, 132, 26, TC_GREY, 143, 142, 10, _("Hammer"), 4, 4, 0, _("Less often"), _("More often"));
    AddProgress( 5, 17, 165, 132, 26, TC_GREY, 151, 150, 10, _("Shovel"), 4, 4, 0, _("Less often"), _("More often"));
    AddProgress( 6, 17, 193, 132, 26, TC_GREY, 153, 152, 10, _("Crucible"), 4, 4, 0, _("Less often"), _("More often"));
    AddProgress( 7, 17, 221, 132, 26, TC_GREY, 155, 154, 10, _("Rod and line"), 4, 4, 0, _("Less often"), _("More often"));
    AddProgress( 8, 17, 249, 132, 26, TC_GREY, 157, 156, 10, _("Scythe"), 4, 4, 0, _("Less often"), _("More often"));
    AddProgress( 9, 17, 277, 132, 26, TC_GREY, 159, 158, 10, _("Cleaver"), 4, 4, 0, _("Less often"), _("More often"));
    AddProgress(10, 17, 305, 132, 26, TC_GREY, 161, 160, 10, _("Rolling pin"), 4, 4, 0, _("Less often"), _("More often"));
    AddProgress(11, 17, 333, 132, 26, TC_GREY, 163, 162, 10, _("Bow"), 4, 4, 0, _("Less often"), _("More often"));

    if (GAMECLIENT.GetGGS().isEnabled(AddonId::TOOL_ORDERING))
    {
        // qx:tools
        for (unsigned i = 0; i < TOOL_COUNT; ++i)
        {
            AddImageButton(100 + i * 2, 174, 25   + i * 28, 20, 13, TC_GREY, LOADER.GetImageN("io",  33), "+1");
            AddImageButton(101 + i * 2, 174, 25 + 13 + i * 28, 20, 13, TC_GREY, LOADER.GetImageN("io",  34), "-1");
            std::stringstream str;
            str << std::max(0, int( GAMECLIENT.GetLocalPlayer().tools_ordered[i] + GAMECLIENT.GetLocalPlayer().tools_ordered_delta[i] ));
            AddDeepening  (200 + i, 151, 25 + 4 + i * 28, 20, 18, TC_GREY, str.str(), NormalFont, COLOR_YELLOW);
        }
    }
    m_Updated = GAMECLIENT.GetGFNumber();

    // Info
    AddImageButton(12,  18, 384, 30, 32, TC_GREY, LOADER.GetImageN("io",  21), _("Help"));
    // Standard
    AddImageButton(13, 118 + (GAMECLIENT.GetGGS().isEnabled(AddonId::TOOL_ORDERING) ? 46 : 0), 384, 30, 32, TC_GREY, LOADER.GetImageN("io", 191), _("Default"));

    // Einstellungen festlegen
    for(unsigned char i = 0; i < TOOL_COUNT; ++i)
        GetCtrl<ctrlProgress>(i)->SetPosition(GAMECLIENT.visual_settings.tools_settings[i]);

    // Netzwerk-Übertragungs-Timer
    AddTimer(14, 2000);
}

iwTools::~iwTools()
{
    TransmitSettings();
}

void iwTools::TransmitSettings()
{
    // Wurden Einstellungen verändert?
    if(settings_changed)
    {
        // Einstellungen speichern
        for(unsigned char i = 0; i < TOOL_COUNT; ++i)
            GAMECLIENT.visual_settings.tools_settings[i] = (unsigned char)GetCtrl<ctrlProgress>(i)->GetPosition();

        GAMECLIENT.ChangeTools(GAMECLIENT.visual_settings.tools_settings, &GAMECLIENT.GetLocalPlayer().tools_ordered_delta[0]);

        settings_changed = false;
    }
}

// qx:tools
void iwTools::UpdateTexts()
{
    if (GAMECLIENT.GetGGS().isEnabled(AddonId::TOOL_ORDERING))
    {
        for (unsigned i = 0; i < TOOL_COUNT; ++i)
        {
            ctrlDeepening* field = GetCtrl<ctrlDeepening>(200 + i);
            std::stringstream str;
            str << std::max(0, int( GAMECLIENT.GetLocalPlayer().tools_ordered[i] + GAMECLIENT.GetLocalPlayer().tools_ordered_delta[i] ));
            field->SetText(str.str());
        }
    }
}
unsigned int iwTools::m_UpdateReq = 0;
void iwTools::UpdateOrders()
{
    m_UpdateReq = GAMECLIENT.GetGFNumber();
}

void iwTools::Msg_PaintBefore()
{
    if (m_Updated < m_UpdateReq)
    {
        UpdateTexts();
        m_Updated = m_UpdateReq;
    }
}

void iwTools::Msg_ButtonClick(const unsigned int ctrl_id)
{
    // qx:tools
    if ( ctrl_id >= 100 && ctrl_id < (100 + 2 * TOOL_COUNT) )
    {
        unsigned int tool = (ctrl_id - 100) / 2;
        GameClientPlayer& me = GAMECLIENT.GetLocalPlayer();

        if (ctrl_id & 0x1)
        {
            // dec
            if ( (me.tools_ordered[tool] + me.tools_ordered_delta[tool]) > 0 )
            {
                --GAMECLIENT.GetLocalPlayer().tools_ordered_delta[tool];
                settings_changed = true;
            }
        }
        else
        {
            // inc
            if ( (GAMECLIENT.GetLocalPlayer().tools_ordered[tool] + GAMECLIENT.GetLocalPlayer().tools_ordered_delta[tool]) < 100 ) //-V807
            {
                ++GAMECLIENT.GetLocalPlayer().tools_ordered_delta[tool];
                settings_changed = true;
            }
        }

        ctrlDeepening* field = GetCtrl<ctrlDeepening>(200 + tool);
        std::stringstream txt;
        txt << int(GAMECLIENT.GetLocalPlayer().tools_ordered[tool] + GAMECLIENT.GetLocalPlayer().tools_ordered_delta[tool]);
        field->SetText(txt.str());
    }
    else
        switch(ctrl_id)
        {
            default: return;
            case 13: // Standard
            {
                GAMECLIENT.visual_settings.tools_settings = GAMECLIENT.default_settings.tools_settings;
                UpdateSettings();
                settings_changed = true;
            } break;
        }
}

void iwTools::Msg_ProgressChange(const unsigned int  /*ctrl_id*/, const unsigned short  /*position*/)
{
    // Einstellungen wurden geändert
    settings_changed = true;
}

void iwTools::Msg_Timer(const unsigned int  /*ctrl_id*/)
{
    if(GAMECLIENT.IsReplayModeOn())
        // Im Replay aktualisieren wir die Werte
        UpdateSettings();
    else
        // Im normalen Spielmodus schicken wir den ganzen Spaß ab
        TransmitSettings();
}

void iwTools::UpdateSettings()
{
    // Einstellungen festlegen
    for(unsigned i = 0; i < 12; ++i)
        GetCtrl<ctrlProgress>(i)->SetPosition(GAMECLIENT.visual_settings.tools_settings[i]);
}
