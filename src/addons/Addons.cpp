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
#include "Addons.h"
#include "Loader.h"
#include "Window.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlCheck.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlText.h"
#include "mygettext.h"
#include "libutil/src/colors.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

void Addon::hideGui(Window* window, unsigned int id) const
{
    ctrlText* text = window->GetCtrl<ctrlText>(id);
    if(text)
        text->SetVisible(false);

    ctrlImageButton* button = window->GetCtrl<ctrlImageButton>(id + 1);
    if(button)
        button->SetVisible(false);
}

void Addon::createGui(Window* window, unsigned int id, unsigned short& y, bool  /*readonly*/, unsigned int  /*status*/) const //-V669
{
    ctrlText* text = window->GetCtrl<ctrlText>(id);
    if(!text)
        text = window->AddText(id, 52, y + 4, name_, COLOR_YELLOW, 0, NormalFont);

    text->SetVisible(true);
    text->Move(52, y + 4);

    ctrlImageButton* button = window->GetCtrl<ctrlImageButton>(id + 1);
    if(!button)
        button = window->AddImageButton(id + 1, 20, y, 22, 22, TC_GREY, LOADER.GetImageN("io", 21), description_);

    button->SetVisible(true);
    button->Move(20, y);
}

void AddonList::hideGui(Window* window, unsigned int id) const
{
    Addon::hideGui(window, id);

    ctrlComboBox* combo = window->GetCtrl<ctrlComboBox>(id + 2);
    if(combo)
        combo->SetVisible(false);
}

void AddonList::createGui(Window* window, unsigned int id, unsigned short& y, bool readonly, unsigned int status) const
{
    Addon::createGui(window, id, y, readonly, status);

    ctrlComboBox* combo = window->GetCtrl<ctrlComboBox>(id + 2);
    if(!combo)
    {
        combo = window->AddComboBox(id + 2, 450, y, 220, 20,  TC_GREY, NormalFont, 100, readonly );
        for(std::vector<std::string>::const_iterator it = options.begin(); it != options.end(); ++it)
            combo->AddString(*it);

        setGuiStatus(window, id, status);
    }

    combo->SetVisible(true);
    combo->Move(430, y);

    y += 30;
}

void AddonList::setGuiStatus(Window* window, unsigned int id, unsigned int status) const
{
    ctrlComboBox* combo = window->GetCtrl<ctrlComboBox>(id + 2);

    if(combo)
        combo->SetSelection(status);
}

unsigned int AddonList::getGuiStatus(Window* window, unsigned int id, bool& failed) const
{
    ctrlComboBox* combo = window->GetCtrl<ctrlComboBox>(id + 2);
    if(!combo)
    {
        failed = true;
        return getDefaultStatus();
    }
    failed = false;

    return combo->GetSelection();
}

void AddonBool::hideGui(Window* window, unsigned int id) const
{
    Addon::hideGui(window, id);
    ctrlCheck* check = window->GetCtrl<ctrlCheck>(id + 2);
    if(check)
        check->SetVisible(false);
}

void AddonBool::createGui(Window* window, unsigned int id, unsigned short& y, bool readonly, unsigned int status) const
{
    Addon::createGui(window, id, y, readonly, status);

    ctrlCheck* check = window->GetCtrl<ctrlCheck>(id + 2);
    if(!check)
    {
        check = window->AddCheckBox(id + 2, 430, y, 220, 20,  TC_GREY, _("Use"), NormalFont, readonly );
        setGuiStatus(window, id, status);
    }

    check->SetVisible(true);
    check->Move(430, y);

    y += 30;
}

void AddonBool::setGuiStatus(Window* window, unsigned int id, unsigned int status) const
{
    ctrlCheck* check = window->GetCtrl<ctrlCheck>(id + 2);

    if(check)
        check->SetCheck( (status != 0) );
}

unsigned int AddonBool::getGuiStatus(Window* window, unsigned int id, bool& failed) const
{
    ctrlCheck* check = window->GetCtrl<ctrlCheck>(id + 2);
    if(!check)
    {
        failed = true;
        return getDefaultStatus();
    }
    failed = false;

    return (check->GetCheck() ? 1 : 0);
}
