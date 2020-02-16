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

#include "rttrDefines.h" // IWYU pragma: keep
#include "iwAddons.h"

#include <utility>

#include "GlobalGameSettings.h"
#include "Loader.h"
#include "addons/Addon.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlScrollBar.h"
#include "helpers/containerUtils.h"
#include "gameData/const_gui_ids.h"
#include "s25util/colors.h"

iwAddons::iwAddons(GlobalGameSettings& ggs, Window* parent, ChangePolicy policy, std::vector<AddonId> addonIds)
    : IngameWindow(CGI_ADDONS, IngameWindow::posLastOrCenter, Extent(700, 500), _("Addon Settings"), LOADER.GetImageN("resource", 41), true,
                   false, parent),
      ggs(ggs), policy(policy), addonIds(std::move(addonIds))
{
    AddText(0, DrawPoint(20, 30), _("Additional features:"), COLOR_YELLOW, FontStyle{}, NormalFont);

    Extent btSize(200, 22);
    if(policy != READONLY)
        AddTextButton(1, DrawPoint(20, GetSize().y - 40), btSize, TC_GREEN2, _("Apply"), NormalFont, _("Apply Changes"));

    AddTextButton(2, DrawPoint(250, GetSize().y - 40), btSize, TC_RED1, _("Abort"), NormalFont, _("Close Without Saving"));

    if(policy != READONLY)
        AddTextButton(3, DrawPoint(480, GetSize().y - 40), btSize, TC_GREY, _("Default"), NormalFont, _("Use S2 Defaults"));

    // Kategorien
    ctrlOptionGroup* optiongroup = AddOptionGroup(5, ctrlOptionGroup::CHECK);
    btSize = Extent(120, 22);
    // "Alle"
    optiongroup->AddTextButton(static_cast<unsigned>(AddonGroup::All), DrawPoint(20, 50), btSize, TC_GREEN2, _("All"), NormalFont);
    // "Militär"
    optiongroup->AddTextButton(static_cast<unsigned>(AddonGroup::Military), DrawPoint(150, 50), btSize, TC_GREEN2, _("Military"),
                               NormalFont);
    // "Wirtschaft"
    optiongroup->AddTextButton(static_cast<unsigned>(AddonGroup::Economy), DrawPoint(290, 50), btSize, TC_GREEN2, _("Economy"), NormalFont);
    // "Spielverhalten"
    optiongroup->AddTextButton(static_cast<unsigned>(AddonGroup::GamePlay), DrawPoint(430, 50), btSize, TC_GREEN2, _("Gameplay"),
                               NormalFont);
    // "Sonstiges"
    optiongroup->AddTextButton(static_cast<unsigned>(AddonGroup::Other), DrawPoint(560, 50), btSize, TC_GREEN2, _("Other"), NormalFont);

    ctrlScrollBar* scrollbar =
      AddScrollBar(6, DrawPoint(GetSize().x - SCROLLBAR_WIDTH - 20, 90), Extent(SCROLLBAR_WIDTH, GetSize().y - 140), SCROLLBAR_WIDTH,
                   TC_GREEN2, (GetSize().y - 140) / 30 - 1);
    scrollbar->SetRange(ggs.getNumAddons());

    optiongroup->SetSelection(static_cast<unsigned>(AddonGroup::All), true);
}

iwAddons::~iwAddons() = default;

void iwAddons::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        default: break;

        case 1: // Apply changes
        {
            if(policy == READONLY)
                Close();

            // Einstellungen in ADDONMANAGER übertragen
            for(unsigned i = 0; i < ggs.getNumAddons(); ++i)
            {
                unsigned status;
                const Addon* addon = ggs.getAddon(i, status);

                if(!addon)
                    continue;

                bool failed = false;
                status = addon->getGuiStatus(this, 10 + 20 * i, failed);
                if(!failed)
                    ggs.setSelection(addon->getId(), status);
            }

            switch(policy)
            {
                default: break;
                case SETDEFAULTS: { ggs.SaveSettings();
                }
                break;
                case HOSTGAME:
                case HOSTGAME_WHITELIST:
                {
                    // send message via msgboxresult
                    GetParent()->Msg_MsgBoxResult(GetID(), MSR_YES);
                }
                break;
            }
            Close();
        }
        break;

        case 2: // Discard changes
        {
            Close();
        }
        break;

        case 3: // Load S2 Defaults
        {
            // Standardeinstellungen aufs Fenster übertragen
            for(unsigned i = 0; i < ggs.getNumAddons(); ++i)
            {
                unsigned status;
                const Addon* addon = ggs.getAddon(i, status);

                if(!addon)
                    continue;
                if(policy == HOSTGAME_WHITELIST && !helpers::contains(addonIds, addon->getId()))
                    continue;

                addon->setGuiStatus(this, 10 + 20 * i, addon->getDefaultStatus());
            }
        }
        break;
    }
}

/// Aktualisiert die Addons, die angezeigt werden sollen
void iwAddons::UpdateView(const AddonGroup selection)
{
    auto* scrollbar = GetCtrl<ctrlScrollBar>(6);
    unsigned short y = 90;
    unsigned short numAddonsInCurCategory = 0;
    for(unsigned i = 0; i < ggs.getNumAddons(); ++i)
    {
        unsigned id = 10 + 20 * i;
        unsigned status;
        const Addon* addon = ggs.getAddon(i, status);

        if(!addon)
            continue;
        const bool isVisible = (addon->getGroups() & selection) != AddonGroup(0);

        if(isVisible)
            ++numAddonsInCurCategory;
        // hide addon's gui if addon is beyond selected group or is beyond current page scope
        if(!isVisible || numAddonsInCurCategory < scrollbar->GetScrollPos() + 1
           || numAddonsInCurCategory > (unsigned)(scrollbar->GetScrollPos() + scrollbar->GetPageSize()) + 1)
        {
            addon->hideGui(this, id);
            continue;
        }

        bool isReadOnly = policy == READONLY || (policy == HOSTGAME_WHITELIST && !helpers::contains(addonIds, addon->getId()));
        addon->createGui(this, id, y, isReadOnly, status);
    }
    if(numAddonsInCurCategory_ != numAddonsInCurCategory)
    {
        numAddonsInCurCategory_ = numAddonsInCurCategory;
        scrollbar->SetRange(numAddonsInCurCategory);
    }
}

void iwAddons::Msg_OptionGroupChange(const unsigned ctrl_id, const unsigned selection)
{
    switch(ctrl_id)
    {
        case 5: // richtige Kategorie anzeigen
        {
            GetCtrl<ctrlScrollBar>(6)->SetScrollPos(0);
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
    auto* optiongroup = GetCtrl<ctrlOptionGroup>(5);
    UpdateView(static_cast<AddonGroup>(optiongroup->GetSelection()));
}

bool iwAddons::Msg_WheelUp(const MouseCoords& /*mc*/)
{
    auto* scrollbar = GetCtrl<ctrlScrollBar>(6);
    scrollbar->Scroll(-2);
    return true;
}

bool iwAddons::Msg_WheelDown(const MouseCoords& /*mc*/)
{
    auto* scrollbar = GetCtrl<ctrlScrollBar>(6);
    scrollbar->Scroll(+2);
    return true;
}
