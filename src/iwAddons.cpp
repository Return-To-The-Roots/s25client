// $Id: iwAddons.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
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
#include "main.h"
#include "iwAddons.h"
#include "controls.h"

#include "GlobalGameSettings.h"
#include "GameClient.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author FloSoft
 */
iwAddons::iwAddons(GlobalGameSettings* ggs, ChangePolicy policy)
    : IngameWindow(CGI_ADDONS, 0xFFFF, 0xFFFF, 700, 500, _("Addon Settings"), LOADER.GetImageN("resource", 41), true), ggs(ggs), policy(policy)
{
    AddText(0, 20, 30, _("Additional features:"), COLOR_YELLOW, 0, NormalFont);

    if(policy != READONLY)
        AddTextButton(1,  20, height - 40, 200, 22, TC_GREY, _("Apply Changes"), NormalFont);

    AddTextButton(2, 250, height - 40, 200, 22, TC_RED1, _("Close Without Saving"), NormalFont);

    if(policy != READONLY)
        AddTextButton(3, 480, height - 40, 200, 22, TC_GREY, _("Use S2 Defaults"), NormalFont);

    // Kategorien
    ctrlOptionGroup* optiongroup = AddOptionGroup(5, ctrlOptionGroup::CHECK, scale);
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

    ctrlScrollBar* scrollbar = AddScrollBar(6, width - SCROLLBAR_WIDTH - 20, 90, SCROLLBAR_WIDTH, height - 140, SCROLLBAR_WIDTH, TC_GREEN2, (height - 140) / 30 - 1);
    scrollbar->SetRange(ggs->getCount());

    optiongroup->SetSelection(ADDONGROUP_ALL, true);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author FloSoft
 */
iwAddons::~iwAddons()
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author FloSoft
 */
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
            for(unsigned int i = 0; i < ggs->getCount(); ++i)
            {
                unsigned int status;
                const Addon* addon = ggs->getAddon(i, status);

                if(!addon)
                    continue;

                bool failed = false;
                status = addon->getGuiStatus(this, 10 + 20 * (ggs->getCount() - i - 1), failed);
                if(!failed)
                    ggs->setSelection(addon->getId(), status);
            }

            switch(policy)
            {
                default:
                    break;
                case SETDEFAULTS:
                {
                    ggs->SaveSettings();
                } break;
                case HOSTGAME:
                {
                    // send message via msgboxresult
                    MsgboxResult mbr = MSR_YES;
                    parent->Msg_MsgBoxResult(GetID(), mbr);
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
            for(unsigned int i = 0; i < ggs->getCount(); ++i)
            {
                unsigned int status;
                const Addon* addon = ggs->getAddon(i, status);

                if(!addon)
                    continue;

                addon->setGuiStatus(this, 10 + 20 * (ggs->getCount() - i - 1), addon->getDefaultStatus());
            }
        } break;
    }
}

/// Aktualisiert die Addons, die angezeigt werden sollen
void iwAddons::UpdateView(const unsigned short selection)
{
    //LOG.lprintf("\nUpdateView start\n");
    ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(6);
    unsigned short y = 90;
    unsigned short inthiscategory = 0;
    //LOG.lprintf("Page range: %u - %u\n", scrollbar->GetPos(), (unsigned int)(scrollbar->GetPos()+scrollbar->GetPageSize()));
    for(unsigned int i = 0; i < ggs->getCount(); ++i)
    {
        unsigned int id = 10 + 20 * (ggs->getCount() - i - 1);
        unsigned int status;
        const Addon* addon = ggs->getAddon(i, status);

        if(!addon)
            continue;
        unsigned int groups = addon->getGroups();

        if( (groups & selection) == selection)
        {
            ++inthiscategory;
            //LOG.lprintf("ADD  addon: %s - falls in this category \n",addon->getName().c_str());
        }
        //hide addon's gui if addon is beyond selected group or is beyond current page scope
        if( ((groups & selection) != selection) || inthiscategory < scrollbar->GetPos() + 1
                || inthiscategory > (unsigned int)(scrollbar->GetPos() + scrollbar->GetPageSize()) + 1 )
        {
            //if((groups & selection) != selection)
            //  LOG.lprintf("HIDE addon: %s - category mismatch\n",addon->getName().c_str());
            //else
            //  LOG.lprintf("HIDE addon: %s - no available space on page\n",addon->getName().c_str());
            addon->hideGui(this, id);
            continue;
        }

        addon->createGui(this, id, y, (policy == READONLY), status);
    }
    if(_inthiscategory != inthiscategory)
    {
        _inthiscategory = inthiscategory;
        scrollbar->SetRange(inthiscategory);
    }
    //LOG.lprintf("UpdateView end\n");
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author FloSoft
 */
void iwAddons::Msg_OptionGroupChange(const unsigned int ctrl_id, const unsigned short selection)
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
void iwAddons::Msg_ScrollChange(const unsigned int ctrl_id, const unsigned short position)
{
    ctrlOptionGroup* optiongroup = GetCtrl<ctrlOptionGroup>(5);
    UpdateView(optiongroup->GetSelection());
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author Divan
 */
bool iwAddons::Msg_WheelUp(const MouseCoords& mc)
{
    // Forward to ScrollBar
    ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(6);

    // Simulate two Button Clicks
    scrollbar->Msg_ButtonClick(0);
    scrollbar->Msg_ButtonClick(0);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author Divan
 */
bool iwAddons::Msg_WheelDown(const MouseCoords& mc)
{
    // Forward to ScrollBar
    ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(6);

    // Simulate two Button Clicks
    scrollbar->Msg_ButtonClick(1);
    scrollbar->Msg_ButtonClick(1);
    return true;
}

