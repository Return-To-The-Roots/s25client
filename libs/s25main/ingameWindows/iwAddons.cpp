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

#include "iwAddons.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "addons/Addon.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlScrollBar.h"
#include "helpers/containerUtils.h"
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
    : IngameWindow(CGI_ADDONS, IngameWindow::posLastOrCenter, Extent(700, 500), _("Addon Settings"),
                   LOADER.GetImageN("resource", 41), true, false, parent),
      ggs(ggs), policy_(policy), whitelistedAddons_(std::move(whitelistedAddons))
{
    AddText(ID_txtAddFeatures, DrawPoint(20, 30), _("Additional features:"), COLOR_YELLOW, FontStyle{}, NormalFont);

    Extent btSize(200, 22);
    if(policy != AddonChangeAllowed::None)
        AddTextButton(ID_btApply, DrawPoint(20, GetSize().y - 40), btSize, TC_GREEN2, _("Apply"), NormalFont,
                      _("Apply Changes"));

    AddTextButton(ID_btAbort, DrawPoint(250, GetSize().y - 40), btSize, TC_RED1, _("Abort"), NormalFont,
                  _("Close Without Saving"));

    if(policy != AddonChangeAllowed::None)
        AddTextButton(ID_btS2Defaults, DrawPoint(480, GetSize().y - 40), btSize, TC_GREY, _("Default"), NormalFont,
                      _("Use S2 Defaults"));

    // Kategorien
    ctrlOptionGroup* optiongroup = AddOptionGroup(ID_grpAddonGroup, GroupSelectType::Check);
    btSize = Extent(120, 22);
    // "Alle"
    optiongroup->AddTextButton(static_cast<unsigned>(AddonGroup::All), DrawPoint(20, 50), btSize, TC_GREEN2, _("All"),
                               NormalFont);
    // "Militär"
    optiongroup->AddTextButton(static_cast<unsigned>(AddonGroup::Military), DrawPoint(150, 50), btSize, TC_GREEN2,
                               _("Military"), NormalFont);
    // "Wirtschaft"
    optiongroup->AddTextButton(static_cast<unsigned>(AddonGroup::Economy), DrawPoint(290, 50), btSize, TC_GREEN2,
                               _("Economy"), NormalFont);
    // "Spielverhalten"
    optiongroup->AddTextButton(static_cast<unsigned>(AddonGroup::GamePlay), DrawPoint(430, 50), btSize, TC_GREEN2,
                               _("Gameplay"), NormalFont);
    // "Sonstiges"
    optiongroup->AddTextButton(static_cast<unsigned>(AddonGroup::Other), DrawPoint(560, 50), btSize, TC_GREEN2,
                               _("Other"), NormalFont);

    ctrlScrollBar* scrollbar = AddScrollBar(ID_scroll, DrawPoint(GetSize().x - SCROLLBAR_WIDTH - 20, 90),
                                            Extent(SCROLLBAR_WIDTH, GetSize().y - 140), SCROLLBAR_WIDTH, TC_GREEN2, 1);
    scrollbar->SetPageSize(scrollbar->GetSize().y / AddonGuiLineHeight);

    for(unsigned i = 0; i < ggs.getNumAddons(); ++i)
    {
        const unsigned id = ID_grpAddonsStart + i;
        const Addon* addon = ggs.getAddon(i);
        RTTR_Assert(addon);
        auto& group = *AddGroup(id);
        addonGuis_.emplace_back(addon->createGui(group, isReadOnly(addon->getId())));
        addonGuis_.back()->setStatus(group, ggs.getSelection(addon->getId()));
    }

    optiongroup->SetSelection(static_cast<unsigned>(AddonGroup::All), true);
}

iwAddons::~iwAddons() = default;

void iwAddons::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        default: break;

        case ID_btApply:
        {
            if(policy_ != AddonChangeAllowed::None)
            {
                // Einstellungen in ADDONMANAGER übertragen
                for(unsigned i = 0; i < ggs.getNumAddons(); ++i)
                {
                    const auto& group = *GetCtrl<ctrlGroup>(ID_grpAddonsStart + i);
                    ggs.setSelection(ggs.getAddon(i)->getId(), addonGuis_[i]->getStatus(group));
                }

                switch(policy_)
                {
                    default: break;
                    case AddonChangeAllowed::AllAndSaveToConfig: ggs.SaveSettings(); break;
                    case AddonChangeAllowed::All:
                    case AddonChangeAllowed::WhitelistOnly:
                        // send message via msgboxresult
                        GetParent()->Msg_MsgBoxResult(GetID(), MsgboxResult::Yes);
                        break;
                }
            }
            Close();
        }
        break;

        case ID_btAbort: // Discard changes
            Close();
            break;

        case ID_btS2Defaults: // Load S2 Defaults
            // Standardeinstellungen aufs Fenster übertragen
            for(unsigned i = 0; i < ggs.getNumAddons(); ++i)
            {
                const Addon* addon = ggs.getAddon(i);
                if(!isReadOnly(addon->getId()))
                    addonGuis_[i]->setStatus(*GetCtrl<ctrlGroup>(ID_grpAddonsStart + i), addon->getDefaultStatus());
            }
            break;
    }
}

/// Aktualisiert die Addons, die angezeigt werden sollen
void iwAddons::UpdateView(const AddonGroup selection)
{
    auto* scrollbar = GetCtrl<ctrlScrollBar>(ID_scroll);
    const unsigned scrollPos = scrollbar->GetScrollPos();
    const unsigned scrollPosEnd = scrollPos + scrollbar->GetPageSize();
    unsigned short y = 90;
    unsigned short numAddonsInCurCategory = 0;
    for(unsigned i = 0; i < ggs.getNumAddons(); ++i)
    {
        const Addon* addon = ggs.getAddon(i);
        const bool isVisible = (addon->getGroups() & selection) != AddonGroup(0);
        auto* group = GetCtrl<ctrlGroup>(ID_grpAddonsStart + i);

        // Don't show addon's gui if addon is beyond selected group or is beyond current page scope
        if(isVisible && numAddonsInCurCategory >= scrollPos && numAddonsInCurCategory < scrollPosEnd)
        {
            group->SetVisible(true);
            group->SetPos({group->GetPos().x, y});
            y += AddonGuiLineHeight;
        } else
            group->SetVisible(false);
        if(isVisible)
            ++numAddonsInCurCategory;
    }
    scrollbar->SetRange(numAddonsInCurCategory);
}

bool iwAddons::isReadOnly(AddonId id) const
{
    return policy_ == AddonChangeAllowed::None
           || (policy_ == AddonChangeAllowed::WhitelistOnly && !helpers::contains(whitelistedAddons_, id));
}

void iwAddons::Msg_OptionGroupChange(const unsigned ctrl_id, const unsigned selection)
{
    switch(ctrl_id)
    {
        case ID_grpAddonGroup: // richtige Kategorie anzeigen
        {
            GetCtrl<ctrlScrollBar>(ID_scroll)->SetScrollPos(0);
            UpdateView(static_cast<AddonGroup>(selection));
        }
        break;
    }
}

/**
 *  get scrollbar notification
 */
void iwAddons::Msg_ScrollChange(const unsigned /*ctrl_id*/, const unsigned short /*position*/)
{
    auto* optiongroup = GetCtrl<ctrlOptionGroup>(ID_grpAddonGroup);
    UpdateView(static_cast<AddonGroup>(optiongroup->GetSelection()));
}

bool iwAddons::Msg_WheelUp(const MouseCoords& /*mc*/)
{
    GetCtrl<ctrlScrollBar>(ID_scroll)->Scroll(-2);
    return true;
}

bool iwAddons::Msg_WheelDown(const MouseCoords& /*mc*/)
{
    GetCtrl<ctrlScrollBar>(ID_scroll)->Scroll(+2);
    return true;
}
