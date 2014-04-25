// $Id: iwTransport.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "iwTransport.h"

#include "Loader.h"
#include "GameClient.h"
#include "controls.h"

#include "GameCommands.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Tooltips in der der Standardbelegung
const unsigned short STD_TOOLTIP_INDICES[14] =
{
    190,
    192,
    185,
    189,
    188,
    187,
    186,
    181,
    182,
    180,
    184,
    183,
    191,
    193
};

const std::string TOOLTIPS[14] =
{
    gettext_noop("Coins"),
    gettext_noop("Weapons"),
    gettext_noop("Beer"),
    gettext_noop("Iron"),
    gettext_noop("Gold"),
    gettext_noop("Iron ore"),
    gettext_noop("Coal"),
    gettext_noop("Boards"),
    gettext_noop("Stones"),
    gettext_noop("Wood"),
    gettext_noop("Water"),
    gettext_noop("Food"),
    gettext_noop("Tools"),
    gettext_noop("Boats")
};

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p iwTools.
 *
 *  @author OLiver
 */
iwTransport::iwTransport()
    : IngameWindow(CGI_TRANSPORT, 0xFFFF, 0xFFFF, 166, 333, _("Transport"), LOADER.GetImageN("io", 5)),
      settings_changed(false)
{
    AddImageButton(0, 18, 285, 30, 30, TC_GREY, LOADER.GetImageN("io",  21), _("Help"));

    // Standard
    AddImageButton(1, 60, 285, 48, 30, TC_GREY, LOADER.GetImageN("io", 191), _("Default"));
    // ganz hoch
    AddImageButton(2, 118, 235, 30, 20, TC_GREY, LOADER.GetImageN("io", 215), _("Top"));
    // hoch
    AddImageButton(3, 118, 255, 30, 20, TC_GREY, LOADER.GetImageN("io",  33), _("Up"));
    // runter
    AddImageButton(4, 118, 275, 30, 20, TC_GREY, LOADER.GetImageN("io",  34), _("Down"));
    // ganz runter
    AddImageButton(5, 118, 295, 30, 20, TC_GREY, LOADER.GetImageN("io", 216), _("Bottom"));

    // Buttons der einzelnen Waren anlegen
    ctrlOptionGroup* group = AddOptionGroup(6, ctrlOptionGroup::ILLUMINATE);

    // Zeiger auf die Bilder für die einzelnen Waren in der Transportschlange
    TRANSPORT_SPRITES[0] = LOADER.GetMapImageN(2250 + GD_COINS);
    TRANSPORT_SPRITES[1] = LOADER.GetImageN("io", 111);
    TRANSPORT_SPRITES[2] = LOADER.GetMapImageN(2250 + GD_BEER);
    TRANSPORT_SPRITES[3] = LOADER.GetMapImageN(2250 + GD_IRON);
    TRANSPORT_SPRITES[4] = LOADER.GetMapImageN(2250 + GD_GOLD);
    TRANSPORT_SPRITES[5] = LOADER.GetMapImageN(2250 + GD_IRONORE);
    TRANSPORT_SPRITES[6] = LOADER.GetMapImageN(2250 + GD_COAL);
    TRANSPORT_SPRITES[7] = LOADER.GetMapImageN(2250 + GD_BOARDS);
    TRANSPORT_SPRITES[8] = LOADER.GetMapImageN(2250 + GD_STONES);
    TRANSPORT_SPRITES[9] = LOADER.GetMapImageN(2250 + GD_WOOD);
    TRANSPORT_SPRITES[10] = LOADER.GetMapImageN(2250 + GD_WATER);
    TRANSPORT_SPRITES[11] = LOADER.GetImageN("io", 80);
    TRANSPORT_SPRITES[12] = LOADER.GetMapImageN(2250 + GD_HAMMER);
    TRANSPORT_SPRITES[13] = LOADER.GetMapImageN(2250 + GD_BOAT);

    //// Tooltips festlegen
    //for(unsigned i = 0;i<14;++i)
    //  tooltip_indices[i] = STD_TOOLTIP_INDICES[GAMECLIENT.visual_settings.transport_order[i]];


    // Positionen der einzelnen Buttons
    const unsigned short BUTTON_POS[14][2] =
    {
        {20, 25},
        {52, 42},
        {84, 59},
        {116, 76},
        {84, 93},
        {52, 110},
        {20, 127},
        {52, 144},
        {84, 161},
        {116, 178},
        {84, 195},
        {52, 212},
        {20, 229},
        {52, 246}
    };

    // Einstellungen festlegen
    for(unsigned char i = 0; i < 14; ++i)
        group->AddImageButton(i, BUTTON_POS[i][0], BUTTON_POS[i][1], 30, 30, TC_GREY,
                              TRANSPORT_SPRITES[GAMECLIENT.visual_settings.transport_order[i]],
                              _(TOOLTIPS[GAMECLIENT.visual_settings.transport_order[i]]));

    // Netzwerk-Übertragungs-Timer
    AddTimer(7, 2000);
}

iwTransport::~iwTransport()
{
    TransmitSettings();
}


void iwTransport::TransmitSettings()
{
    if(settings_changed)
    {
        // Daten übertragen
        GAMECLIENT.AddGC(new gc::ChangeTransport(GAMECLIENT.visual_settings.transport_order));

        settings_changed = false;
    }
}


void iwTransport::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 1: // Standardbelegung
        {
            ctrlOptionGroup* group = GetCtrl<ctrlOptionGroup>(6);

            GAMECLIENT.visual_settings.transport_order = GAMECLIENT.default_settings.transport_order;

            //// Tooltips in der der Standardbelegung
            //memcpy(tooltip_indices,STD_TOOLTIP_INDICES,14*sizeof(unsigned short));

            for(unsigned char i = 0; i < 14; ++i)
            {
                group->GetCtrl<ctrlImageButton>(i)->SetImage(TRANSPORT_SPRITES[i]);
                group->GetCtrl<ctrlImageButton>(i)->SetTooltip(_(TOOLTIPS[i]));
            }

            settings_changed = true;
        } break;
        case 2: // ganz hoch
        {
            ctrlOptionGroup* group = GetCtrl<ctrlOptionGroup>(6);

            // Wenn wir schon ganz oben sind, gehts nicht weiter höher
            while(group->GetSelection() > 0 && group->GetSelection() != 0xFFFF)
            {
                Swap(GAMECLIENT.visual_settings.transport_order[group->GetSelection()], GAMECLIENT.visual_settings.transport_order[group->GetSelection() - 1]);
                group->GetCtrl<ctrlImageButton>(group->GetSelection())->SwapImage(group->GetCtrl<ctrlImageButton>(group->GetSelection() - 1));
                group->GetCtrl<ctrlImageButton>(group->GetSelection())->SwapTooltip(group->GetCtrl<ctrlImageButton>(group->GetSelection() - 1));
                group->SetSelection(group->GetSelection() - 1);
            }

            settings_changed = true;
        } break;
        case 3: // hoch
        {
            ctrlOptionGroup* group = GetCtrl<ctrlOptionGroup>(6);

            // Wenn wir schon ganz oben sind, gehts nicht weiter höher
            if(group->GetSelection() > 0 && group->GetSelection() != 0xFFFF)
            {
                Swap(GAMECLIENT.visual_settings.transport_order[group->GetSelection()], GAMECLIENT.visual_settings.transport_order[group->GetSelection() - 1]);
                group->GetCtrl<ctrlImageButton>(group->GetSelection())->SwapImage(group->GetCtrl<ctrlImageButton>(group->GetSelection() - 1));
                group->GetCtrl<ctrlImageButton>(group->GetSelection())->SwapTooltip(group->GetCtrl<ctrlImageButton>(group->GetSelection() - 1));
                group->SetSelection(group->GetSelection() - 1);
            }

            settings_changed = true;
        } break;
        case 4: // runter
        {
            ctrlOptionGroup* group = GetCtrl<ctrlOptionGroup>(6);

            // Wenn wir schon ganz unten sind, gehts nicht weiter runter
            if(group->GetSelection() < 13 && group->GetSelection() != 0xFFFF)
            {
                Swap(GAMECLIENT.visual_settings.transport_order[group->GetSelection()], GAMECLIENT.visual_settings.transport_order[group->GetSelection() + 1]);
                group->GetCtrl<ctrlImageButton>(group->GetSelection())->SwapImage(group->GetCtrl<ctrlImageButton>(group->GetSelection() + 1));
                group->GetCtrl<ctrlImageButton>(group->GetSelection())->SwapTooltip(group->GetCtrl<ctrlImageButton>(group->GetSelection() + 1));
                group->SetSelection(group->GetSelection() + 1);
            }

            settings_changed = true;
        } break;
        case 5: // ganz runter
        {
            ctrlOptionGroup* group = GetCtrl<ctrlOptionGroup>(6);

            // Wenn wir schon ganz unten sind, gehts nicht weiter runter
            while(group->GetSelection() < 13 && group->GetSelection() != 0xFFFF)
            {
                Swap(GAMECLIENT.visual_settings.transport_order[group->GetSelection()], GAMECLIENT.visual_settings.transport_order[group->GetSelection() + 1]);
                group->GetCtrl<ctrlImageButton>(group->GetSelection())->SwapImage(group->GetCtrl<ctrlImageButton>(group->GetSelection() + 1));
                group->GetCtrl<ctrlImageButton>(group->GetSelection())->SwapTooltip(group->GetCtrl<ctrlImageButton>(group->GetSelection() + 1));
                group->SetSelection(group->GetSelection() + 1);
            }

            settings_changed = true;
        } break;
    }
}

void iwTransport::Msg_Timer(const unsigned int ctrl_id)
{
    if(GAMECLIENT.IsReplayModeOn())
        // Im Replay aktualisieren wir die Werte
        UpdateSettings();
    else
        // Im normalen Spielmodus schicken wir den ganzen Spaß ab
        TransmitSettings();
}

void iwTransport::UpdateSettings()
{
    ctrlOptionGroup* group = GetCtrl<ctrlOptionGroup>(6);

    // Einstellungen festlegen
    for(unsigned char i = 0; i < 14; ++i)
    {
        group->GetCtrl<ctrlImageButton>(i)->SetImage(TRANSPORT_SPRITES[GAMECLIENT.visual_settings.transport_order[i]]);
        group->GetCtrl<ctrlImageButton>(i)->SetTooltip(_(TOOLTIPS[GAMECLIENT.visual_settings.transport_order[i]]));
    }
}
