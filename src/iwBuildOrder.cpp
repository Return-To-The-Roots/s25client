// $Id: iwBuildOrder.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "iwBuildOrder.h"

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
 *  Konstruktor von @p iwBuildOrder.
 *
 *  @author OLiver
 */
iwBuildOrder::iwBuildOrder(void)
    : IngameWindow(CGI_BUILDORDER, (unsigned short) - 1, (unsigned short) - 1, 320, 300, _("Building sequence"), LOADER.GetImageN("io", 5)),
      settings_changed(false)
{
    ctrlList* list = AddList(0, 15, 60, 150, 220, TC_GREY, NormalFont);

    // Liste füllen
    for(unsigned char i = 0; i < 31; ++i)
        list->AddString(_(BUILDING_NAMES[GAMECLIENT.visual_settings.build_order[i]]));

    // Nach ganz oben
    AddImageButton(1, 250, 194, 48, 20, TC_GREY, LOADER.GetImageN("io", 215), _("Top"));
    // Hoch
    AddImageButton(2, 250, 216, 48, 20, TC_GREY, LOADER.GetImageN("io",  33), _("Up"));
    // Runter
    AddImageButton(3, 250, 238, 48, 20, TC_GREY, LOADER.GetImageN("io",  34), _("Down"));
    // Nach ganz unten
    AddImageButton(4, 250, 260, 48, 20, TC_GREY, LOADER.GetImageN("io", 216), _("Bottom"));

    // Bild der Auswahl
    AddImage(5, 240, 150, LOADER.GetNationImageN(GAMECLIENT.GetLocalPlayer()->nation, 250 + GAMECLIENT.visual_settings.build_order[0] * 5));

    ctrlComboBox* combo = AddComboBox(6, 15, 30, 290, 20, TC_GREY, NormalFont, 100);
    combo->AddString(_("Sequence of given order")); // "Reihenfolge der Auftraggebung"
    combo->AddString(_("After the following order")); // "Nach folgender Reihenfolge"

    // Eintrag in Combobox auswählen
    combo->SetSelection(GAMECLIENT.visual_settings.order_type);

    // Standard
    AddImageButton(10, 200, 250, 48, 30, TC_GREY, LOADER.GetImageN("io", 191), _("Default"));

    // Absendetimer, in 2s-Abschnitten wird jeweils das ganze als Netzwerknachricht ggf. abgeschickt
    AddTimer(11, 2000);

    list->SetSelection(0);
}

iwBuildOrder::~iwBuildOrder()
{
    unsigned char selection = (unsigned char)GetCtrl<ctrlComboBox>(6)->GetSelection();
    GAMECLIENT.visual_settings.order_type = selection;

    TransmitSettings();
}

void iwBuildOrder::TransmitSettings()
{
    // Wurden Einstellungen geändert?
    if(settings_changed)
    {
        // Einstellungen speichern
        GAMECLIENT.AddGC(new gc::ChangeBuildOrder((unsigned char)GetCtrl<ctrlComboBox>(6)->GetSelection(), GAMECLIENT.visual_settings.build_order));
        settings_changed = false;
    }
}

void iwBuildOrder::Msg_Timer(const unsigned int ctrl_id)
{
    if(GAMECLIENT.IsReplayModeOn())
        // Im Replay aktualisieren wir die Werte
        UpdateSettings();
    else
        // Im normalen Spielmodus schicken wir den ganzen Spaß ab
        TransmitSettings();
}


void iwBuildOrder::Msg_ListSelectItem(const unsigned int ctrl_id, const unsigned short selection)
{
    switch(ctrl_id)
    {
        default:
            break;

        case 0:
        {
            GetCtrl<ctrlImage>(5)->SetImage(
                LOADER.GetNationImageN(GAMECLIENT.GetLocalPlayer()->nation,
                                       250 + GAMECLIENT.visual_settings.build_order[selection] * 5));
        } break;
    }
}

void iwBuildOrder::Msg_ButtonClick(const unsigned int ctrl_id)
{
    ctrlList* list = GetCtrl<ctrlList>(0);
    unsigned short auswahl = list->GetSelection();
    unsigned short anzahl = list->GetLineCount();

    // Auswahl gültig?
    if(auswahl >= anzahl)
        return;

    switch(ctrl_id)
    {
        default:
            break;

        case 1: // Nach ganz oben
        {
            //Swap(GAMECLIENT.visual_settings.build_order[0], GAMECLIENT.visual_settings.build_order[auswahl]);
            //list->Swap(0, auswahl);
            while(auswahl > 0)
            {
                Swap(GAMECLIENT.visual_settings.build_order[auswahl - 1], GAMECLIENT.visual_settings.build_order[auswahl]);
                list->Swap(auswahl - 1, auswahl);
                --auswahl;
            }
            settings_changed = true;
        } break;
        case 2: // Hoch
        {
            if(auswahl > 0)
            {
                Swap(GAMECLIENT.visual_settings.build_order[auswahl - 1], GAMECLIENT.visual_settings.build_order[auswahl]);
                list->Swap(auswahl - 1, auswahl);
            }
            settings_changed = true;
        } break;
        case 3: // Runter
        {
            if(auswahl < anzahl - 1)
            {
                Swap(GAMECLIENT.visual_settings.build_order[auswahl + 1], GAMECLIENT.visual_settings.build_order[auswahl]);
                list->Swap(auswahl + 1, auswahl);
            }
            settings_changed = true;
        } break;
        case 4: // Nach ganz unten
        {
            while(auswahl < anzahl - 1)
            {
                Swap(GAMECLIENT.visual_settings.build_order[auswahl + 1], GAMECLIENT.visual_settings.build_order[auswahl]);
                list->Swap(auswahl + 1, auswahl);
                ++auswahl;
            }
            settings_changed = true;
        } break;
        case 10: // Standardwerte
        {
            // Baureihenfolge vom Spieler kopieren
            GAMECLIENT.visual_settings.build_order = GAMECLIENT.default_settings.build_order;

            ctrlList* list = GetCtrl<ctrlList>(0);
            list->DeleteAllItems();

            // Liste füllen
            for(unsigned char i = 0; i < 31; ++i)
                list->AddString(_(BUILDING_NAMES[GAMECLIENT.default_settings.build_order[i]]));
            list->SetSelection(0);

            GetCtrl<ctrlImage>(5)->SetImage(LOADER.GetNationImageN(GAMECLIENT.GetLocalPlayer()->nation, 250 + GAMECLIENT.visual_settings.build_order[0] * 5));

            settings_changed = true;
        } break;
    }
}

void iwBuildOrder::UpdateSettings()
{
    // Liste füllen
    for(unsigned char i = 0; i < 31; ++i)
        GetCtrl<ctrlList>(0)->SetString(_(BUILDING_NAMES[GAMECLIENT.visual_settings.build_order[i]]), i);
}
