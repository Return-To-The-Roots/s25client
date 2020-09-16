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

#include "iwDistribution.h"

#include <utility>

#include "GamePlayer.h"
#include "Loader.h"
#include "WindowManager.h"
#include "controls/ctrlGroup.h"
#include "controls/ctrlProgress.h"
#include "controls/ctrlTab.h"
#include "iwHelp.h"
#include "network/GameClient.h"
#include "ogl/FontStyle.h"
#include "world/GameWorldViewer.h"
#include "gameData/BuildingConsts.h"
#include "gameData/const_gui_ids.h"

struct iwDistribution::DistributionGroup
{
    DistributionGroup(std::string name, glArchivItem_Bitmap* img) : name(std::move(name)), img(img) {}
    std::string name;
    glArchivItem_Bitmap* img;
    std::vector<std::string> entries;
};
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
        for(const std::string& entry : group.entries)
        {
            unsigned txtId = group.entries.size() + curId;
            tabGrp->AddText(txtId, txtPos, entry, COLOR_YELLOW, FontStyle::CENTER | FontStyle::BOTTOM, SmallFont);
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
    AddImageButton(2, DrawPoint(15, GetSize().y - 15 - btSize.y), btSize, TC_GREY, LOADER.GetImageN("io", 225),
                   _("Help"));
    // Standardbelegung
    AddImageButton(10, GetSize() - DrawPoint::all(15) - btSize, btSize, TC_GREY, LOADER.GetImageN("io", 191),
                   _("Default"));

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
        // Read values from the progress ctrls to the struct
        Distributions newDistribution;

        unsigned distIdx = 0;
        for(unsigned i = 0; i < groups.size(); ++i)
        {
            ctrlGroup* tab = GetCtrl<ctrlTab>(0)->GetGroup(i);
            const DistributionGroup& group = groups[i];
            // Werte der Gruppen auslesen
            for(unsigned j = 0; j < group.entries.size(); ++j, ++distIdx)
            {
                auto value = static_cast<uint8_t>(tab->GetCtrl<ctrlProgress>(j)->GetPosition());
                newDistribution[distIdx] = value;
            }
        }
        RTTR_Assert(distIdx == newDistribution.size());

        // und übermitteln
        if(gcFactory.ChangeDistribution(newDistribution))
        {
            GAMECLIENT.visual_settings.distribution = newDistribution;
            settings_changed = false;
        }
    }
}

void iwDistribution::Msg_Group_ProgressChange(const unsigned /*group_id*/, const unsigned /*ctrl_id*/,
                                              const unsigned short /*position*/)
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
    unsigned distIdx = 0;
    for(unsigned g = 0; g < groups.size(); ++g)
    {
        // Look for correct group
        const DistributionGroup& group = groups[g];
        ctrlGroup* tab = GetCtrl<ctrlTab>(0)->GetGroup(g);
        // And correct entry
        for(unsigned i = 0; i < group.entries.size(); ++i, ++distIdx)
            tab->GetCtrl<ctrlProgress>(i)->SetPosition(GAMECLIENT.visual_settings.distribution[distIdx]);
    }
    RTTR_Assert(distIdx == std::tuple_size<Distributions>::value);
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
            WINDOWMANAGER.ReplaceWindow(
              std::make_unique<iwHelp>(_("The priority of goods for the individual buildings can be set here. "
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
    if(!groups.empty())
        return;

    GoodType lastGood = GD_NOTHING;
    for(const DistributionMapping& mapping : distributionMap)
    {
        // New group?
        if(lastGood != std::get<0>(mapping))
        {
            lastGood = std::get<0>(mapping);
            // Fish = all foodstuff
            std::string name = lastGood == GD_FISH ? gettext_noop("Foodstuff") : WARE_NAMES[lastGood];
            glArchivItem_Bitmap* img = nullptr;
            switch(lastGood)
            {
                case GD_FISH: img = LOADER.GetImageN("io", 80); break;
                case GD_GRAIN: img = LOADER.GetImageN("io", 90); break;
                case GD_IRON: img = LOADER.GetImageN("io", 81); break;
                case GD_COAL: img = LOADER.GetImageN("io", 91); break;
                case GD_WOOD: img = LOADER.GetImageN("io", 89); break;
                case GD_BOARDS: img = LOADER.GetImageN("io", 82); break;
                case GD_WATER: img = LOADER.GetImageN("io", 92); break;
                default: break;
            }
            if(!img)
                throw std::runtime_error("Unexpected good in distribution");
            groups.push_back(DistributionGroup(_(name), img));
        }
        // HQ = Construction
        std::string name = std::get<1>(mapping) == BLD_HEADQUARTERS ? gettext_noop("Construction") :
                                                                      BUILDING_NAMES[std::get<1>(mapping)];
        groups.back().entries.push_back(_(name));
    }
}
