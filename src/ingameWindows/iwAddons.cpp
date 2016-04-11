// Copyright (c) 2005-2010 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Siedler II.5 RTTR.
//
// Siedler II.5 RTTR is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Siedler II.5 RTTR is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Siedler II.5 RTTR. If not, see <http://www.gnu.org/licenses/>.

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h" // IWYU pragma: keep
#include "iwAddons.h"

#include "GlobalGameSettings.h"
#include "Loader.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlScrollBar.h"
#include "gameData/const_gui_ids.h"
#include "libutil/src/colors.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep
class MouseCoords;

iwAddons::iwAddons(GlobalGameSettings& ggs, ChangePolicy policy, const std::vector<AddonId>& addonIds)
    : IngameWindow(CGI_ADDONS, 0xFFFF, 0xFFFF, 700, 500, _("Addon Settings"), LOADER.GetImageN("resource", 41), true), ggs(ggs), policy(policy), addonIds(addonIds)
{
    AddText(0, 20, 30, _("Additional features:"), COLOR_YELLOW, 0, NormalFont);

    if(policy != READONLY)
        AddTextButton(1,  20, height_ - 40, 200, 22, TC_GREY, _("Apply Changes"), NormalFont);

    AddTextButton(2, 250, height_ - 40, 200, 22, TC_RED1, _("Close Without Saving"), NormalFont);

    if(policy != READONLY)
        AddTextButton(3, 480, height_ - 40, 200, 22, TC_GREY, _("Use S2 Defaults"), NormalFont);

    // Kategorien
    ctrlOptionGroup* optiongroup = AddOptionGroup(5, ctrlOptionGroup::CHECK, scale_);
    // "Alle"
    optiongroup->AddTextButton(ADDONGROUP_ALL,  20, 50, 120, 22, TC_GREEN2, _("All"), NormalFont);
    // "Militär"
    optiongroup->AddTextButton(ADDONGROUP_MILITARY, 150, 50, 120, 22, TC_GREEN2, _("Military"), NormalFont);
    // "Wirtschaft"
    optiongroup->AddTextButton(ADDONGROUP_ECONOMY, 290, 50, 120, 22, TC_GREEN2, _("Economy"), NormalFont);
    // "Spielverhalten"
    optiongroup->AddTextButton(ADDONGROUP_GAMEPLAY, 430, 50, 120, 22, TC_GREEN2, _("Gameplay"), NormalFont);
    // "Sonstiges"
    optiongroup->AddTextButton(ADDONGROUP_OTHER, 560, 50, 120, 22, TC_GREEN2, _("Other"), NormalFont);

    ctrlScrollBar* scrollbar = AddScrollBar(6, width_ - SCROLLBAR_WIDTH - 20, 90, SCROLLBAR_WIDTH, height_ - 140, SCROLLBAR_WIDTH, TC_GREEN2, (height_ - 140) / 30 - 1);
    scrollbar->SetRange(ggs.getCount());

    optiongroup->SetSelection(ADDONGROUP_ALL, true);
}

iwAddons::~iwAddons()
{
}

void iwAddons::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        default:
            break;

        case 1: // Apply changes
        {
            if(policy == READONLY)
                Close();

            // Einstellungen in ADDONMANAGER übertragen
            for(unsigned int i = 0; i < ggs.getCount(); ++i)
            {
                unsigned int status;
                const Addon* addon = ggs.getAddon(i, status);

                if(!addon)
                    continue;

                bool failed = false;
                status = addon->getGuiStatus(this, 10 + 20 * (ggs.getCount() - i - 1), failed);
                if(!failed)
                    ggs.setSelection(addon->getId(), status);
            }

            switch(policy)
            {
                default:
                    break;
                case SETDEFAULTS:
                {
                    ggs.SaveSettings();
                } break;
                case HOSTGAME:
                case HOSTGAME_WHITELIST:
                {
                    // send message via msgboxresult
                    parent_->Msg_MsgBoxResult(GetID(), MSR_YES);
                } break;
            }
            Close();
        } break;

        case 2: // Discard changes
        {
            Close();
        } break;

        case 3: // Load S2 Defaults
        {
            // Standardeinstellungen aufs Fenster übertragen
            for(unsigned int i = 0; i < ggs.getCount(); ++i)
            {
                unsigned int status;
                const Addon* addon = ggs.getAddon(i, status);

                if(!addon)
                    continue;
                if(policy == HOSTGAME_WHITELIST && !helpers::contains(addonIds, addon->getId()))
                    continue;

                addon->setGuiStatus(this, 10 + 20 * (ggs.getCount() - i - 1), addon->getDefaultStatus());
            }
        } break;
    }
}

/// Aktualisiert die Addons, die angezeigt werden sollen
void iwAddons::UpdateView(const unsigned short selection)
{
    ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(6);
    unsigned short y = 90;
    unsigned short numAddonsInCurCategory = 0;
    for(unsigned int i = 0; i < ggs.getCount(); ++i)
    {
        unsigned int id = 10 + 20 * (ggs.getCount() - i - 1);
        unsigned int status;
        const Addon* addon = ggs.getAddon(i, status);

        if(!addon)
            continue;
        unsigned int groups = addon->getGroups();

        if( (groups & selection) == selection)
            ++numAddonsInCurCategory;
        //hide addon's gui if addon is beyond selected group or is beyond current page scope
        if( ((groups & selection) != selection) || numAddonsInCurCategory < scrollbar->GetPos() + 1
                || numAddonsInCurCategory > (unsigned int)(scrollbar->GetPos() + scrollbar->GetPageSize()) + 1 )
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

void iwAddons::Msg_OptionGroupChange(const unsigned int ctrl_id, const int selection)
{
    switch(ctrl_id)
    {
        case 5: // richtige Kategorie anzeigen
        {
            ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(6);
            scrollbar->SetPos(0);
            UpdateView(selection);

        } break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  get scrollbar notification
 *
 *  @author FloSoft
 */
void iwAddons::Msg_ScrollChange(const unsigned int  /*ctrl_id*/, const unsigned short  /*position*/)
{
    ctrlOptionGroup* optiongroup = GetCtrl<ctrlOptionGroup>(5);
    UpdateView(optiongroup->GetSelection());
}

bool iwAddons::Msg_WheelUp(const MouseCoords&  /*mc*/)
{
    // Forward to ScrollBar
    ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(6);

    // Simulate two Button Clicks
    scrollbar->Msg_ButtonClick(0);
    scrollbar->Msg_ButtonClick(0);
    return true;
}

bool iwAddons::Msg_WheelDown(const MouseCoords&  /*mc*/)
{
    // Forward to ScrollBar
    ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(6);

    // Simulate two Button Clicks
    scrollbar->Msg_ButtonClick(1);
    scrollbar->Msg_ButtonClick(1);
    return true;
}

