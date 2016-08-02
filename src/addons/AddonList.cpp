// Copyright (c) 2005 - 2016 Settlers Freaks (sf-team at siedler25.org)
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
#include "AddonList.h"
#include "Loader.h"
#include "Window.h"
#include "controls/ctrlComboBox.h"
#include "helpers/containerUtils.h"
#include "mygettext.h"

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

unsigned AddonList::getNumOptions() const
{
    return options.size();
}

void AddonList::removeOptions()
{
    options.clear();
}

void AddonList::addOption(const std::string& name)
{
    if(!helpers::contains(options, name))
        options.push_back(name);
}
