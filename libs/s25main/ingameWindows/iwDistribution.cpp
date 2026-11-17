// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwDistribution.h"
#include "AddonHelperFunctions.h"
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
                                 _("Distribution of goods"), LOADER.getImageN("resource", 41)),
      gwv(gwv), gcFactory(gcFactory)
{
    createGroups();

    resize(Extent(groups.size() * TAB_ICON_SIZE + TAB_ICON_SIZE / 2 + 20, 312));

    // Tab Control
    ctrlTab* tab = addTabCtrl(0, DrawPoint(10, 20), groups.size() * TAB_ICON_SIZE + TAB_ICON_SIZE / 2);
    DrawPoint txtPos(getSize().x / 2, 60);
    DrawPoint progPos(PROGRESS_BORDER_DISTANCE - tab->getPos().x, txtPos.y);
    const Extent progSize(getSize().x - 2 * PROGRESS_BORDER_DISTANCE, 20);

    for(unsigned groupId = 0; groupId < groups.size(); groupId++)
    {
        const DistributionGroup& group = groups[groupId];
        ctrlGroup* tabGrp = tab->addTab(group.img, group.name, groupId);
        txtPos.y = progPos.y = 60;
        unsigned curId = 0;
        for(const auto& entry : group.entries)
        {
            unsigned txtId = group.entries.size() + curId;
            tabGrp->addText(txtId, txtPos, std::get<0>(entry), COLOR_YELLOW, FontStyle::CENTER | FontStyle::BOTTOM,
                            SmallFont);
            tabGrp->addProgress(curId++, progPos, progSize, TextureColor::Grey, 139, 138, 10);
            txtPos.y = progPos.y += progSize.y * 2;
        }
    }

    // Gruppe auswählen
    tab->setSelection(0);

    const Extent btSize(32, 32);
    // "Help" button
    addImageButton(2, DrawPoint(15, getFullSize().y - 15 - btSize.y), btSize, TextureColor::Grey,
                   LOADER.getImageN("io", 225), _("Help"));
    // "Default" button
    addImageButton(10, getFullSize() - DrawPoint::all(15) - btSize, btSize, TextureColor::Grey,
                   LOADER.getImageN("io", 191), _("Default"));

    iwDistribution::updateSettings();
}

void iwDistribution::transmitSettings()
{
    if(GAMECLIENT.isReplayModeOn())
        return;
    if(settings_changed)
    {
        // Read values from the progress ctrls to the struct
        Distributions newDistribution;
        // Set default values for all distributions because groups with only one entry are not shown and will not be set
        std::transform(distributionMap.begin(), distributionMap.end(), newDistribution.begin(),
                       [](DistributionMapping const& mapping) { return std::get<2>(mapping); });

        for(unsigned i = 0; i < groups.size(); ++i)
        {
            ctrlGroup* tab = getCtrl<ctrlTab>(0)->getGroup(i);
            const DistributionGroup& group = groups[i];
            // Werte der Gruppen auslesen
            for(unsigned j = 0; j < group.entries.size(); ++j)
            {
                auto value = static_cast<uint8_t>(tab->getCtrl<ctrlProgress>(j)->getPosition());
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

void iwDistribution::msgGroupProgressChange(const unsigned /*group_id*/, const unsigned /*ctrl_id*/,
                                            const unsigned short /*position*/)
{
    settings_changed = true;
}

void iwDistribution::updateSettings(const Distributions& distribution)
{
    if(GAMECLIENT.isReplayModeOn())
        gwv.getPlayer().fillVisualSettings(GAMECLIENT.visual_settings);

    for(unsigned g = 0; g < groups.size(); ++g)
    {
        // Look for correct group
        const DistributionGroup& group = groups[g];
        ctrlGroup* tab = getCtrl<ctrlTab>(0)->getGroup(g);
        // And correct entry
        for(unsigned i = 0; i < group.entries.size(); ++i)
            tab->getCtrl<ctrlProgress>(i)->setPosition(distribution[std::get<1>(group.entries[i])]);
    }
}

void iwDistribution::updateSettings()
{
    updateSettings(GAMECLIENT.visual_settings.distribution);
}

void iwDistribution::msgButtonClick(const unsigned ctrl_id)
{
    if(GAMECLIENT.isReplayModeOn())
        return;
    switch(ctrl_id)
    {
        default: return;

        case 2:
        {
            WINDOWMANAGER.replaceWindow(
              std::make_unique<iwHelp>(_("The priority of goods for the individual buildings can be set here. "
                                         "The higher the value, the quicker the required goods are delivered "
                                         "to the building concerned.")));
        }
        break;
        // Default button
        case 10:
        {
            updateSettings(GAMECLIENT.default_settings.distribution);
            settings_changed = true;
        }
        break;
    }
}

void iwDistribution::createGroups()
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
                case GoodType::Fish: img = LOADER.getImageN("io", 80); break;
                case GoodType::Grain: img = LOADER.getImageN("io", 90); break;
                case GoodType::Iron: img = LOADER.getImageN("io", 81); break;
                case GoodType::Coal: img = LOADER.getImageN("io", 91); break;
                case GoodType::Wood: img = LOADER.getImageN("io", 89); break;
                case GoodType::Boards: img = LOADER.getImageN("io", 82); break;
                case GoodType::Water: img = LOADER.getImageN("io", 92); break;
                case GoodType::Ham:
                    img = LOADER.getImageN("leather_bobs",
                                           leatheraddon::bobIndex[leatheraddon::BobType::DistributionOfPigsIcon]);
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
        return makeIsUnusedBuilding(gwv.getWorld().getGGS())(buildingType);
    };
    for(auto& group : groups)
        helpers::eraseIf(group.entries, isUnused);

    // Remove groups with only one entry (pigs when leather addon is disabled)
    helpers::eraseIf(groups, [](DistributionGroup& group) { return group.entries.size() == 1; });
}
