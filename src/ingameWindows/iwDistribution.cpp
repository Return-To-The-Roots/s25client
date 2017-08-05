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

#include "defines.h" // IWYU pragma: keep
#include "iwDistribution.h"

#include "GameClient.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "WindowManager.h"
#include "controls/ctrlGroup.h"
#include "controls/ctrlProgress.h"
#include "controls/ctrlTab.h"
#include "iwHelp.h"
#include "ogl/glArchivItem_Font.h"
#include "world/GameWorldViewer.h"
#include "gameData/const_gui_ids.h"
#include <boost/assign/std/vector.hpp>
#include <boost/foreach.hpp>

std::vector<iwDistribution::DistributionGroup> iwDistribution::groups;

/// Dertermines width of the progress bars: distance to the window borders
const unsigned PROGRESS_BORDER_DISTANCE = 20;

iwDistribution::iwDistribution(const GameWorldViewer& gwv, GameCommandFactory& gcFactory)
    : IngameWindow(CGI_DISTRIBUTION, IngameWindow::posLastOrCenter, Extent(290, 312), _("Distribution of goods"),
                   LOADER.GetImageN("resource", 41)),
      gwv(gwv), gcFactory(gcFactory), settings_changed(false)
{
    CreateGroups();

    // Tab Control
    ctrlTab* tab = AddTabCtrl(0, DrawPoint(10, 20), 270);
    DrawPoint txtPos(GetSize().x / 2, 60);
    DrawPoint progPos(PROGRESS_BORDER_DISTANCE - tab->GetPos().x, txtPos.y);
    const Extent progSize(GetSize().x - 2 * PROGRESS_BORDER_DISTANCE, 20);

    for(unsigned groupId = 0; groupId < groups.size(); groupId++)
    {
        const DistributionGroup& group = groups[groupId];
        ctrlGroup* tabGrp = tab->AddTab(group.img, group.name, groupId);
        txtPos.y = progPos.y = 60;
        unsigned curId = 0;
        BOOST_FOREACH(const std::string& entry, group.entries)
        {
            tabGrp->AddText(curId++, txtPos, entry, COLOR_YELLOW, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, SmallFont);
            tabGrp->AddProgress(curId++, progPos, progSize, TC_GREY, 139, 138, 10);
            txtPos.y = progPos.y += progSize.y * 2;
        }
    }

    // Gruppe auswählen
    tab->SetSelection(0);

    // Timer für die Übertragung der Daten via Netzwerk
    AddTimer(1, 2000);

    const Extent btSize(32, 32);
    // Hilfe
    AddImageButton(2, DrawPoint(15, GetSize().y - 15 - btSize.y), btSize, TC_GREY, LOADER.GetImageN("io", 225), _("Help"));
    // Standardbelegung
    AddImageButton(10, GetSize() - DrawPoint::all(15) - btSize, btSize, TC_GREY, LOADER.GetImageN("io", 191), _("Default"));

    UpdateSettings();
}

iwDistribution::~iwDistribution()
{
    TransmitSettings();
}

void iwDistribution::TransmitSettings()
{
    if(GAMECLIENT.IsReplayModeOn())
        return;
    if(settings_changed)
    {
        // Werte aus den Progress-Controls auslesen
        Distributions newDistribution;

        for(unsigned char i = 0, j = 0; i < groups.size(); ++i)
        {
            ctrlGroup* tab = GetCtrl<ctrlTab>(0)->GetGroup(i);
            // Werte der Gruppen auslesen
            for(unsigned char k = 0; k < groups[i].entries.size(); ++k, ++j)
            {
                newDistribution[j] = (unsigned char)tab->GetCtrl<ctrlProgress>(k * 2 + 1)->GetPosition();
            }
        }

        // und übermitteln
        if(gcFactory.ChangeDistribution(newDistribution))
        {
            GAMECLIENT.visual_settings.distribution = newDistribution;
            settings_changed = false;
        }
    }
}

void iwDistribution::Msg_Group_ProgressChange(const unsigned /*group_id*/, const unsigned /*ctrl_id*/, const unsigned short /*position*/)
{
    settings_changed = true;
}

void iwDistribution::Msg_Timer(const unsigned /*ctrl_id*/)
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
    if(GAMECLIENT.IsReplayModeOn())
        gwv.GetPlayer().FillVisualSettings(GAMECLIENT.visual_settings);
    // Globale Id für alle Gruppen für die visual_settings
    unsigned vsi = 0;
    // Alle Gruppen durchgehen und Einstellungen festlegen
    for(unsigned g = 0; g < groups.size(); ++g)
    {
        ctrlGroup* group = GetCtrl<ctrlTab>(0)->GetGroup(g);
        for(unsigned i = 0; i < groups[g].entries.size(); ++i, ++vsi)
            group->GetCtrl<ctrlProgress>(i * 2 + 1)->SetPosition(GAMECLIENT.visual_settings.distribution[vsi]);
    }
}

void iwDistribution::Msg_ButtonClick(const unsigned ctrl_id)
{
    if(GAMECLIENT.IsReplayModeOn())
        return;
    switch(ctrl_id)
    {
        default: return;

        case 2:
        {
            WINDOWMANAGER.Show(new iwHelp(GUI_ID(CGI_HELP), _("The priority of goods for the individual buildings can be set here. "
                                                              "The higher the value, the quicker the required goods are delivered "
                                                              "to the building concerned.")));
        }
        break;
        // Default button
        case 10:
        {
            GAMECLIENT.visual_settings.distribution = GAMECLIENT.default_settings.distribution;
            UpdateSettings();
            settings_changed = true;
        }
        break;
    }
}

void iwDistribution::CreateGroups()
{
    using namespace boost::assign;
    if(!groups.empty())
        return;

    groups.push_back(DistributionGroup(_("Foodstuff"), LOADER.GetImageN("io", 80)));
    groups.back().entries += _("Granite mine"), _("Coal mine"), _("Iron mine"), _("Gold mine");

    groups.push_back(DistributionGroup(_("Grain"), LOADER.GetImageN("io", 90)));
    groups.back().entries += _("Mill"), _("Pig farm"), _("Donkey breeding"), _("Brewery"), _("Charburner");

    groups.push_back(DistributionGroup(_("Iron"), LOADER.GetImageN("io", 81)));
    groups.back().entries += _("Armory"), _("Metalworks");

    groups.push_back(DistributionGroup(_("Coal"), LOADER.GetImageN("io", 91)));
    groups.back().entries += _("Armory"), _("Iron smelter"), _("Mint");

    groups.push_back(DistributionGroup(_("Wood"), LOADER.GetImageN("io", 89)));
    groups.back().entries += _("Sawmill"), _("Charburner");

    groups.push_back(DistributionGroup(_("Boards"), LOADER.GetImageN("io", 82)));
    groups.back().entries += _("Construction"), _("Metalworks"), _("Shipyard");

    groups.push_back(DistributionGroup(_("Water"), LOADER.GetImageN("io", 92)));
    groups.back().entries += _("Bakery"), _("Brewery"), _("Pig farm");
}
