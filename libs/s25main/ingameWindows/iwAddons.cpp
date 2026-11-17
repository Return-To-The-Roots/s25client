// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwAddons.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "WindowManager.h"
#include "addons/Addon.h"
#include "commonDefines.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlScrollBar.h"
#include "helpers/containerUtils.h"
#include "iwAddonPresets.h"
#include "gameData/const_gui_ids.h"
#include "s25util/colors.h"
#include <utility>

namespace {
enum
{
    ID_txtAddFeatures,
    ID_btApply,
    ID_btAbort,
    ID_btS2Defaults,
    ID_btSavePreset,
    ID_btLoadPreset,
    ID_grpAddonGroup,
    ID_scroll,
    ID_grpAddonsStart
};
/// Breite der Scrollbar
constexpr unsigned SCROLLBAR_WIDTH = 20;
constexpr unsigned AddonGuiLineHeight = 30;

} // namespace

iwAddons::iwAddons(GlobalGameSettings& ggs, Window* parent, AddonChangeAllowed policy,
                   std::vector<AddonId> whitelistedAddons)
    : IngameWindow(CGI_ADDONS, IngameWindow::posLastOrCenter, Extent(700, 530), _("Addon Settings"),
                   LOADER.getImageN("resource", 41), true, CloseBehavior::Custom, parent),
      ggs(ggs), policy_(policy), whitelistedAddons_(std::move(whitelistedAddons))
{
    addText(ID_txtAddFeatures, DrawPoint(20, 30), _("Additional features:"), COLOR_YELLOW, FontStyle{}, NormalFont);

    Extent btSize(200, 22);
    if(policy != AddonChangeAllowed::None)
    {
        addTextButton(ID_btSavePreset, DrawPoint(20, getSize().y - 70), btSize, TextureColor::Green2, _("Save"),
                      NormalFont, _("Save Addon Preset"));
        addTextButton(ID_btLoadPreset, DrawPoint(250, getSize().y - 70), btSize, TextureColor::Green2, _("Load"),
                      NormalFont, _("Load Addon Preset"));
        addTextButton(ID_btS2Defaults, DrawPoint(480, getSize().y - 70), btSize, TextureColor::Grey, _("Default"),
                      NormalFont, _("Use S2 Defaults"));
        addTextButton(ID_btApply, DrawPoint(20, getSize().y - 40), btSize, TextureColor::Green2, _("Apply"), NormalFont,
                      _("Apply Changes"));
    }

    addTextButton(ID_btAbort, DrawPoint(250, getSize().y - 40), btSize, TextureColor::Red1, _("Abort"), NormalFont,
                  _("Close Without Saving"));

    // Kategorien
    ctrlOptionGroup* optiongroup = addOptionGroup(ID_grpAddonGroup, GroupSelectType::Check);
    btSize = Extent(120, 22);
    // "Alle"
    optiongroup->addTextButton(static_cast<unsigned>(AddonGroup::All), DrawPoint(20, 50), btSize, TextureColor::Green2,
                               _("All"), NormalFont);
    // "Militär"
    optiongroup->addTextButton(static_cast<unsigned>(AddonGroup::Military), DrawPoint(150, 50), btSize,
                               TextureColor::Green2, _("Military"), NormalFont);
    // "Wirtschaft"
    optiongroup->addTextButton(static_cast<unsigned>(AddonGroup::Economy), DrawPoint(290, 50), btSize,
                               TextureColor::Green2, _("Economy"), NormalFont);
    // "Spielverhalten"
    optiongroup->addTextButton(static_cast<unsigned>(AddonGroup::GamePlay), DrawPoint(430, 50), btSize,
                               TextureColor::Green2, _("Gameplay"), NormalFont);
    // "Sonstiges"
    optiongroup->addTextButton(static_cast<unsigned>(AddonGroup::Other), DrawPoint(560, 50), btSize,
                               TextureColor::Green2, _("Other"), NormalFont);

    ctrlScrollBar* scrollbar =
      addScrollBar(ID_scroll, DrawPoint(getSize().x - SCROLLBAR_WIDTH - 20, 90),
                   Extent(SCROLLBAR_WIDTH, getSize().y - 170), SCROLLBAR_WIDTH, TextureColor::Green2, 1);
    scrollbar->setPageSize(scrollbar->getSize().y / AddonGuiLineHeight);

    for(unsigned i = 0; i < ggs.getNumAddons(); ++i)
    {
        const unsigned id = ID_grpAddonsStart + i;
        const Addon& addon = assertNonNull(ggs.getAddon(i));
        addonGuis_.emplace_back(addon.createGui(*addGroup(id), isReadOnly(addon.getId())));
        addonGuis_.back()->setStatus(ggs.getSelection(addon.getId()));
    }

    optiongroup->setSelection(static_cast<unsigned>(AddonGroup::All), true);
}

iwAddons::~iwAddons() = default;

void iwAddons::close()
{
    // Close an open save/load preset window: the load window holds a callback into this window,
    // so it must not outlive it
    WINDOWMANAGER.close(CGI_ADDON_PRESETS);
    IngameWindow::close();
}

void iwAddons::msgButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        default: break;

        case ID_btApply:
        {
            if(policy_ != AddonChangeAllowed::None)
            {
                for(const auto& gui : addonGuis_)
                    ggs.setSelection(gui->getAddon().getId(), gui->getStatus());

                switch(policy_)
                {
                    default: break;
                    case AddonChangeAllowed::AllAndSaveToConfig: ggs.saveSettings(); break;
                    case AddonChangeAllowed::All:
                    case AddonChangeAllowed::WhitelistOnly:
                        // send message via msgboxresult
                        getParent()->msgMsgBoxResult(getID(), MsgboxResult::Yes);
                        break;
                }
            }
            close();
        }
        break;

        case ID_btAbort: // Discard changes
            close();
            break;

        case ID_btSavePreset:
        {
            std::map<unsigned, unsigned> states;
            for(const auto& gui : addonGuis_)
                states[static_cast<unsigned>(gui->getAddon().getId())] = gui->getStatus();
            WINDOWMANAGER.show(std::make_unique<iwSaveAddonPreset>(std::move(states)));
        }
        break;

        case ID_btLoadPreset:
            WINDOWMANAGER.show(std::make_unique<iwLoadAddonPreset>(
              [this](const std::map<unsigned, unsigned>& states) { applyAddonStates(states); }));
            break;

        case ID_btS2Defaults: // Load S2 Defaults
            for(const auto& gui : addonGuis_)
            {
                const Addon& addon = gui->getAddon();
                if(!isReadOnly(addon.getId()))
                    gui->setStatus(addon.getDefaultStatus());
            }
            break;
    }
}

void iwAddons::updateView(const AddonGroup selection)
{
    auto* scrollbar = getCtrl<ctrlScrollBar>(ID_scroll);
    const unsigned scrollPos = scrollbar->getScrollPos();
    const unsigned scrollPosEnd = scrollPos + scrollbar->getPageSize();
    unsigned short y = 90;
    unsigned short numAddonsInCurCategory = 0;
    for(const auto& gui : addonGuis_)
    {
        const bool isVisible = bitset::any(gui->getAddon().getGroups(), selection);
        Window& group = gui->getWindow();

        // Don't show addon's gui if addon is beyond selected group or is beyond current page scope
        if(isVisible && numAddonsInCurCategory >= scrollPos && numAddonsInCurCategory < scrollPosEnd)
        {
            group.setVisible(true);
            group.setPos({group.getPos().x, y});
            y += AddonGuiLineHeight;
        } else
            group.setVisible(false);
        if(isVisible)
            ++numAddonsInCurCategory;
    }
    scrollbar->setRange(numAddonsInCurCategory);
}

void iwAddons::applyAddonStates(const std::map<unsigned, unsigned>& states)
{
    for(const auto& gui : addonGuis_)
    {
        const Addon& addon = gui->getAddon();
        if(!isReadOnly(addon.getId()))
        {
            const auto it = states.find(static_cast<unsigned>(addon.getId()));
            const unsigned rawStatus = (it != states.end()) ? it->second : addon.getDefaultStatus();
            const unsigned status = (rawStatus < addon.getNumOptions()) ? rawStatus : addon.getDefaultStatus();
            gui->setStatus(status);
        }
    }
}

bool iwAddons::isReadOnly(AddonId id) const
{
    return policy_ == AddonChangeAllowed::None
           || (policy_ == AddonChangeAllowed::WhitelistOnly && !helpers::contains(whitelistedAddons_, id));
}

void iwAddons::msgOptionGroupChange(const unsigned ctrl_id, const unsigned selection)
{
    switch(ctrl_id)
    {
        case ID_grpAddonGroup: // richtige Kategorie anzeigen
        {
            getCtrl<ctrlScrollBar>(ID_scroll)->setScrollPos(0);
            updateView(static_cast<AddonGroup>(selection));
        }
        break;
    }
}

/**
 *  get scrollbar notification
 */
void iwAddons::msgScrollChange(const unsigned /*ctrl_id*/, const unsigned short /*position*/)
{
    auto* optiongroup = getCtrl<ctrlOptionGroup>(ID_grpAddonGroup);
    updateView(static_cast<AddonGroup>(optiongroup->getSelection()));
}

bool iwAddons::msgWheelUp(const MouseCoords& /*mc*/)
{
    getCtrl<ctrlScrollBar>(ID_scroll)->scroll(-2);
    return true;
}

bool iwAddons::msgWheelDown(const MouseCoords& /*mc*/)
{
    getCtrl<ctrlScrollBar>(ID_scroll)->scroll(+2);
    return true;
}
