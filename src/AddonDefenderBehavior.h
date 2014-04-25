// $Id: AddonDefenderBehavior.h 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef ADDONDEFENDERBEHAVIOR_H_INCLUDED
#define ADDONDEFENDERBEHAVIOR_H_INCLUDED

#pragma once

#include "Addons.h"

///////////////////////////////////////////////////////////////////////////////
/**
 *  Addon for changing the behavior of the military setting
 *  defender
 *
 *  @author jh
 */
class AddonDefenderBehavior : public AddonList
{
    public:
        AddonDefenderBehavior() : AddonList(ADDON_DEFENDER_BEHAVIOR,
                                                ADDONGROUP_MILITARY,
                                                gettext_noop("Change defender behavior"),
                                                gettext_noop("Allows to change the military setting 'defender'.\n\n"
                                                        "You can choose to disallow any changes to that setting\n"
                                                        "or you can limit the amount of reoccupying troops\n"
                                                        "(during an attack) according to the defender setting."),
                                                0
                                               )
        {
            addOption(gettext_noop("No change"));
            addOption(gettext_noop("Disallow change"));
            addOption(gettext_noop("Reduce reoccupying troops accordingly"));
        }
};

#endif // !ADDONDEFENDERBEHAVIOR_H_INCLUDED

