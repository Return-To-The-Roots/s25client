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
#include "iwDistribution.h"

#include "Loader.h"
#include "controls/ctrlGroup.h"
#include "controls/ctrlProgress.h"
#include "controls/ctrlTab.h"
#include "GameClient.h"
#include "ogl/glArchivItem_Font.h"
#include "gameData/const_gui_ids.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep


/// Anzahl der einzelnen Einstellungen in den Gruppen
// Nahrungsgruppe
const unsigned GROUP_SIZES[7] =
{
    4, 5, 2, 3, 2, 3, 4
};

/// Dertermines width of the progress bars: distance to the window borders
const unsigned PROGRESS_BORDER_DISTANCE = 20;

iwDistribution::iwDistribution()
    : IngameWindow(CGI_DISTRIBUTION, 0xFFFF, 0xFFFF, 290, 312, _("Distribution of goods"), LOADER.GetImageN("resource", 41)),
      settings_changed(false)
{
    ctrlGroup* group;

    // Werte für die Progressbars
    //const GameClientPlayer *player = GAMECLIENT.GetLocalPlayer();

    // Tab Control
    ctrlTab* tab = AddTabCtrl(0, 10, 20, 270);

    // Nahrungsgruppe
    group = tab->AddTab(LOADER.GetImageN("io", 80), _("Foodstuff"), TAB_FOOD);
    // Granitbergwerk
    group->AddText(0, width_ / 2,  60, _("Granite mine"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, SmallFont);
    (void) group->AddProgress(1, PROGRESS_BORDER_DISTANCE - tab->GetX(false),  60, width_ - 2 * PROGRESS_BORDER_DISTANCE, 20, TC_GREY, 139, 138, 10);
    // Kohlebergwerk
    group->AddText(2, width_ / 2, 100, _("Coal mine"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, SmallFont);
    (void) group->AddProgress(3, PROGRESS_BORDER_DISTANCE - tab->GetX(false), 100, width_ - 2 * PROGRESS_BORDER_DISTANCE, 20, TC_GREY, 139, 138, 10);
    // Eisenbergwerk
    group->AddText(4, width_ / 2, 140, _("Iron mine"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, SmallFont);
    (void) group->AddProgress(5, PROGRESS_BORDER_DISTANCE - tab->GetX(false), 140, width_ - 2 * PROGRESS_BORDER_DISTANCE, 20, TC_GREY, 139, 138, 10);
    // Goldbergwerk
    group->AddText(6, width_ / 2, 180, _("Gold mine"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, SmallFont);
    (void) group->AddProgress(7, PROGRESS_BORDER_DISTANCE - tab->GetX(false), 180, width_ - 2 * PROGRESS_BORDER_DISTANCE, 20, TC_GREY, 139, 138, 10);

    // Getreidegruppe
    group = tab->AddTab(LOADER.GetImageN("io", 90), _("Grain"), TAB_CORN);

    // Mühle
    group->AddText(0, width_ / 2,  60, _("Mill"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, SmallFont);
    (void) group->AddProgress(1, PROGRESS_BORDER_DISTANCE - tab->GetX(false),  60, width_ - 2 * PROGRESS_BORDER_DISTANCE, 20, TC_GREY, 139, 138, 10);
    // Schweinezucht
    group->AddText(2, width_ / 2, 100, _("Pig farm"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, SmallFont);
    (void) group->AddProgress(3, PROGRESS_BORDER_DISTANCE - tab->GetX(false), 100, width_ - 2 * PROGRESS_BORDER_DISTANCE, 20, TC_GREY, 139, 138, 10);
    // Eselzucht
    group->AddText(4, width_ / 2, 140, _("Donkey breeding"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, SmallFont);
    (void) group->AddProgress(5, PROGRESS_BORDER_DISTANCE - tab->GetX(false), 140, width_ - 2 * PROGRESS_BORDER_DISTANCE, 20, TC_GREY, 139, 138, 10);
    // Brauerei
    group->AddText(6, width_ / 2, 180, _("Brewery"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, SmallFont);
    (void) group->AddProgress(7, PROGRESS_BORDER_DISTANCE - tab->GetX(false), 180, width_ - 2 * PROGRESS_BORDER_DISTANCE, 20, TC_GREY, 139, 138, 10);
    // Charburner
    group->AddText(8, width_ / 2, 220, _("Charburner"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, SmallFont);
    (void) group->AddProgress(9, PROGRESS_BORDER_DISTANCE - tab->GetX(false), 220, width_ - 2 * PROGRESS_BORDER_DISTANCE, 20, TC_GREY, 139, 138, 10);

    // Eisengruppe
    group = tab->AddTab(LOADER.GetImageN("io", 81), _("Iron"), TAB_IRON);

    // Schmiede
    group->AddText(0, width_ / 2,  60, _("Armory"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, SmallFont);
    (void) group->AddProgress(1, PROGRESS_BORDER_DISTANCE - tab->GetX(false),  60, width_ - 2 * PROGRESS_BORDER_DISTANCE, 20, TC_GREY, 139, 138, 10);
    // Schlosserei
    group->AddText(2, width_ / 2, 100, _("Metalworks"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, SmallFont);
    (void) group->AddProgress(3, PROGRESS_BORDER_DISTANCE - tab->GetX(false), 100, width_ - 2 * PROGRESS_BORDER_DISTANCE, 20, TC_GREY, 139, 138, 10);

    // Kohlegruppe
    group = tab->AddTab(LOADER.GetImageN("io", 91), _("Coal"), TAB_COAL);

    // Schmiede
    group->AddText(0, width_ / 2,  60, _("Armory"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, SmallFont);
    (void) group->AddProgress(1, PROGRESS_BORDER_DISTANCE - tab->GetX(false),  60, width_ - 2 * PROGRESS_BORDER_DISTANCE, 20, TC_GREY, 139, 138, 10);
    // Eisenschmelze
    group->AddText(2, width_ / 2, 100, _("Iron smelter"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, SmallFont);
    (void) group->AddProgress(3, PROGRESS_BORDER_DISTANCE - tab->GetX(false), 100, width_ - 2 * PROGRESS_BORDER_DISTANCE, 20, TC_GREY, 139, 138, 10);
    // Münzprägerei
    group->AddText(4, width_ / 2, 140, _("Mint"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, SmallFont);
    (void) group->AddProgress(5, PROGRESS_BORDER_DISTANCE - tab->GetX(false), 140, width_ - 2 * PROGRESS_BORDER_DISTANCE, 20, TC_GREY, 139, 138, 10);

    // Wood group
    group = tab->AddTab(LOADER.GetImageN("io", 89), _("Wood"), TAB_WOOD);

    // Sawmill
    group->AddText(0, width_ / 2,  60, _("Sawmill"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, SmallFont);
    (void) group->AddProgress(1, PROGRESS_BORDER_DISTANCE - tab->GetX(false),  60, width_ - 2 * PROGRESS_BORDER_DISTANCE, 20, TC_GREY, 139, 138, 10);
    // Charburner
    group->AddText(2, width_ / 2, 100, _("Charburner"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, SmallFont);
    (void) group->AddProgress(3, PROGRESS_BORDER_DISTANCE - tab->GetX(false), 100, width_ - 2 * PROGRESS_BORDER_DISTANCE, 20, TC_GREY, 139, 138, 10);

    // Brettergruppe
    group = tab->AddTab(LOADER.GetImageN("io", 82), _("Boards"), TAB_BOARD);

    // Baustellen
    group->AddText(0, width_ / 2,  60, _("Construction"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, SmallFont);
    (void) group->AddProgress(1, PROGRESS_BORDER_DISTANCE - tab->GetX(false),  60, width_ - 2 * PROGRESS_BORDER_DISTANCE, 20, TC_GREY, 139, 138, 10);
    // Schlosserei
    group->AddText(2, width_ / 2, 100, _("Metalworks"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, SmallFont);
    (void) group->AddProgress(3, PROGRESS_BORDER_DISTANCE - tab->GetX(false), 100, width_ - 2 * PROGRESS_BORDER_DISTANCE, 20, TC_GREY, 139, 138, 10);
    // Werft
    group->AddText(4, 120, 140, _("Shipyard"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, SmallFont);
    (void) group->AddProgress(5, PROGRESS_BORDER_DISTANCE - tab->GetX(false), 140, width_ - 2 * PROGRESS_BORDER_DISTANCE, 20, TC_GREY, 139, 138, 10);

    // Wasserbüffel äh -gruppe ;-)
    group = tab->AddTab(LOADER.GetImageN("io", 92), _("Water"), TAB_WATER);

    // Bäckerei
    group->AddText(0, width_ / 2,  60, _("Bakery"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, SmallFont);
    (void) group->AddProgress(1, PROGRESS_BORDER_DISTANCE - tab->GetX(false),  60, width_ - 2 * PROGRESS_BORDER_DISTANCE, 20, TC_GREY, 139, 138, 10);
    // Brauerei
    group->AddText(2, width_ / 2, 100, _("Brewery"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, SmallFont);
    (void) group->AddProgress(3, PROGRESS_BORDER_DISTANCE - tab->GetX(false), 100, width_ - 2 * PROGRESS_BORDER_DISTANCE, 20, TC_GREY, 139, 138, 10);
    // Schweinezucht
    group->AddText(4, width_ / 2, 140, _("Pig farm"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, SmallFont);
    (void) group->AddProgress(5, PROGRESS_BORDER_DISTANCE - tab->GetX(false), 140, width_ - 2 * PROGRESS_BORDER_DISTANCE, 20, TC_GREY, 139, 138, 10);
    // Eselzucht
    group->AddText(6, width_ / 2, 180, _("Donkey breeding"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, SmallFont);
    (void) group->AddProgress(7, PROGRESS_BORDER_DISTANCE - tab->GetX(false), 180, width_ - 2 * PROGRESS_BORDER_DISTANCE, 20, TC_GREY, 139, 138, 10);





    UpdateSettings();

    // Gruppe auswählen
    tab->SetSelection(0);

    // Timer für die Übertragung der Daten via Netzwerk
    AddTimer(1, 2000);


    // Hilfe
    AddImageButton(2, 15, height_ - 15 - 32, 32, 32, TC_GREY, LOADER.GetImageN("io", 21), _("Help"));
    // Standardbelegung
    AddImageButton(10, width_ - 15 - 32, height_ - 15 - 32, 32, 32, TC_GREY, LOADER.GetImageN("io", 191), _("Default"));

}

iwDistribution::~iwDistribution()
{
    TransmitSettings();
}

void iwDistribution::TransmitSettings()
{
    if(settings_changed)
    {
        // Werte aus den Progress-Controls auslesen

        for(unsigned char i = 1, j = 0; i <= 7; ++i)
        {
            // Werte der Gruppen auslesen
            for(unsigned char k = 0; k < GROUP_SIZES[i - 1]; ++k)
                GAMECLIENT.visual_settings.distribution[j + k]
                = (unsigned char)GetCtrl<ctrlTab>(0)->GetGroup(i)->GetCtrl<ctrlProgress>(k * 2 + 1)->
                  GetPosition();
            j += GROUP_SIZES[i - 1];
        }

        // und übermitteln
        GAMECLIENT.ChangeDistribution(GAMECLIENT.visual_settings.distribution);

        settings_changed = false;
    }
}

void iwDistribution::Msg_Group_ProgressChange(const unsigned int  /*group_id*/, const unsigned int  /*ctrl_id*/, const unsigned short  /*position*/)
{
    settings_changed = true;
}

void iwDistribution::Msg_Timer(const unsigned int  /*ctrl_id*/)
{
    if(GAMECLIENT.IsReplayModeOn())
        // Im Replay aktualisieren wir die Werte
        UpdateSettings();
    else
        // Im normalen Spielmodus schicken wir den ganzen Spaß ab
        TransmitSettings();
}

void iwDistribution::UpdateSettings()
{
    // Globale Id für alle Gruppen für die visual_settings
    unsigned vsi = 0;
    // Alle Gruppen durchgehen und Einstellungen festlegen
    for(unsigned g = 0; g < 7; ++g)
    {
        ctrlGroup* group = GetCtrl<ctrlTab>(0)->GetGroup(TAB_FOOD + g);
        for(unsigned i = 0; i < GROUP_SIZES[g]; ++i, ++vsi)
            group->GetCtrl<ctrlProgress>(i * 2 + 1)->SetPosition(GAMECLIENT.visual_settings.distribution[vsi]);
    }
}


void iwDistribution::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        default: return;
            // Default button
        case 10:
        {
            GAMECLIENT.visual_settings.distribution = GAMECLIENT.default_settings.distribution;
            UpdateSettings();
            settings_changed = true;
        } break;
    }
}

