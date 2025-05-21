// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwDistribution.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "LeatherLoader.h"
#include "Loader.h"
#include "WindowManager.h"
#include "WineLoader.h"
#include "addons/const_addons.h"
#include "controls/ctrlGroup.h"
#include "controls/ctrlProgress.h"
#include "controls/ctrlTab.h"
#include "iwHelp.h"
#include "network/GameClient.h"
#include "ogl/FontStyle.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldViewer.h"
#include "gameData/BuildingConsts.h"
#include "gameData/GoodConsts.h"
#include "gameData/const_gui_ids.h"
#include <utility>

/// Dertermines width of the progress bars: distance to the window borders
const unsigned PROGRESS_BORDER_DISTANCE = 20;
const unsigned TAB_ICON_SIZE = 36;

iwDistribution::iwDistribution(const GameWorldViewer& gwv, GameCommandFactory& gcFactory)
    : TransmitSettingsIgwAdapter(CGI_DISTRIBUTION, IngameWindow::posLastOrCenter, Extent(290, 312),
                                 _("Distribution of goods"), LOADER.GetImageN("resource", 41)),
      gwv(gwv), gcFactory(gcFactory)
{
    CreateGroups();

    Resize(Extent(groups.size() * TAB_ICON_SIZE + TAB_ICON_SIZE / 2 + 20, 312));

    // Tab Control
    ctrlTab* tab = AddTabCtrl(0, DrawPoint(10, 20), groups.size() * TAB_ICON_SIZE + TAB_ICON_SIZE / 2);
    DrawPoint txtPos(GetSize().x / 2, 60);
    DrawPoint progPos(PROGRESS_BORDER_DISTANCE - tab->GetPos().x, txtPos.y);
    const Extent progSize(GetSize().x - 2 * PROGRESS_BORDER_DISTANCE, 20);

    for(unsigned groupId = 0; groupId < groups.size(); groupId++)
    {
        const DistributionGroup& group = groups[groupId];
        ctrlGroup* tabGrp = tab->AddTab(group.img, group.name, groupId);
        txtPos.y = progPos.y = 60;
        unsigned curId = 0;
        for(const auto& entry : group.entries)
        {
            unsigned txtId = group.entries.size() + curId;
            tabGrp->AddText(txtId, txtPos, std::get<0>(entry), COLOR_YELLOW, FontStyle::CENTER | FontStyle::BOTTOM,
                            SmallFont);
            tabGrp->AddProgress(curId++, progPos, progSize, TextureColor::Grey, 139, 138, 10);
            txtPos.y = progPos.y += progSize.y * 2;
        }
    }

    // Gruppe auswählen
    tab->SetSelection(0);

    const Extent btSize(32, 32);
    // "Help" button
    AddImageButton(2, DrawPoint(15, GetFullSize().y - 15 - btSize.y), btSize, TextureColor::Grey,
                   LOADER.GetImageN("io", 225), _("Help"));
    // "Default" button
    AddImageButton(10, GetFullSize() - DrawPoint::all(15) - btSize, btSize, TextureColor::Grey,
                   LOADER.GetImageN("io", 191), _("Default"));

    iwDistribution::UpdateSettings();
}

void iwDistribution::TransmitSettings()
{
    if(GAMECLIENT.IsReplayModeOn())
        return;
    if(settings_changed)
    {
        // Read values from the progress ctrls to the struct
        Distributions newDistribution{0};

        for(unsigned i = 0; i < groups.size(); ++i)
        {
            ctrlGroup* tab = GetCtrl<ctrlTab>(0)->GetGroup(i);
            const DistributionGroup& group = groups[i];
            // Werte der Gruppen auslesen
            for(unsigned j = 0; j < group.entries.size(); ++j)
            {
                auto value = static_cast<uint8_t>(tab->GetCtrl<ctrlProgress>(j)->GetPosition());
                newDistribution[std::get<1>(group.entries[j])] = value;
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

void iwDistribution::Msg_Group_ProgressChange(const unsigned /*group_id*/, const unsigned /*ctrl_id*/,
                                              const unsigned short /*position*/)
{
    settings_changed = true;
}

void iwDistribution::UpdateSettings(const Distributions& distribution)
{
    if(GAMECLIENT.IsReplayModeOn())
        gwv.GetPlayer().FillVisualSettings(GAMECLIENT.visual_settings);

    for(unsigned g = 0; g < groups.size(); ++g)
    {
        // Look for correct group
        const DistributionGroup& group = groups[g];
        ctrlGroup* tab = GetCtrl<ctrlTab>(0)->GetGroup(g);
        // And correct entry
        for(unsigned i = 0; i < group.entries.size(); ++i)
            tab->GetCtrl<ctrlProgress>(i)->SetPosition(distribution[std::get<1>(group.entries[i])]);
    }
}

void iwDistribution::UpdateSettings()
{
    UpdateSettings(GAMECLIENT.visual_settings.distribution);
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
            UpdateSettings(GAMECLIENT.default_settings.distribution);
            settings_changed = true;
        }
        break;
    }
}

void iwDistribution::CreateGroups()
{
    if(!groups.empty())
        return;

    GoodType lastGood = GoodType::Nothing;
    unsigned pos = 0;
    for(const DistributionMapping& mapping : distributionMap)
    {
        // New group?
        if(lastGood != std::get<0>(mapping))
        {
            lastGood = std::get<0>(mapping);
            // Fish = all foodstuff
            std::string name = lastGood == GoodType::Fish ? gettext_noop("Foodstuff") : WARE_NAMES[lastGood];
            glArchivItem_Bitmap* img = nullptr;
            switch(lastGood)
            {
                case GoodType::Fish: img = LOADER.GetImageN("io", 80); break;
                case GoodType::Grain: img = LOADER.GetImageN("io", 90); break;
                case GoodType::Iron: img = LOADER.GetImageN("io", 81); break;
                case GoodType::Coal: img = LOADER.GetImageN("io", 91); break;
                case GoodType::Wood: img = LOADER.GetImageN("io", 89); break;
                case GoodType::Boards: img = LOADER.GetImageN("io", 82); break;
                case GoodType::Water: img = LOADER.GetImageN("io", 92); break;
                case GoodType::Ham:
                    img = LOADER.GetImageN("leather_bobs",
                                           leatheraddon::bobIndex[leatheraddon::BobTypes::DISTRIBUTION_OF_PIGS_ICON]);
                    break;
                default: break;
            }
            if(!img)
                throw std::runtime_error("Unexpected good in distribution");

            groups.push_back(DistributionGroup(_(name), img));
        }
        // HQ = Construction
        std::string name = std::get<1>(mapping) == BuildingType::Headquarters ? gettext_noop("Construction") :
                                                                                BUILDING_NAMES[std::get<1>(mapping)];
        groups.back().entries.push_back(std::tuple(_(name), pos));
        pos++;
    }

    auto isUnused = [&](std::tuple<std::string, unsigned> const& bts) {
        const BuildingType buildingType = std::get<1>(distributionMap[std::get<1>(bts)]);
        if(!wineaddon::isAddonActive(gwv.GetWorld()) && wineaddon::isWineAddonBuildingType(buildingType))
            return true;
        if(!gwv.GetWorld().GetGGS().isEnabled(AddonId::CHARBURNER) && buildingType == BuildingType::Charburner)
            return true;
        if(!leatheraddon::isAddonActive(gwv.GetWorld()) && leatheraddon::isLeatherAddonBuildingType(buildingType))
            return true;
        return false;
    };
    for(auto& group : groups)
        helpers::erase_if(group.entries, isUnused);

    helpers::erase_if(groups, [](DistributionGroup& group) { return group.entries.size() == 1; });
}
