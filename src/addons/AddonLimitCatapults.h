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
#ifndef ADDONLIMITCATAPULTS_H_INCLUDED
#define ADDONLIMITCATAPULTS_H_INCLUDED

#pragma once

#include "Addons.h"
#include "mygettext/src/mygettext.h"

///////////////////////////////////////////////////////////////////////////////
/**
 *  Addon for limiting the count of catapults
 *
 *  @author FloSoft
 */
class AddonLimitCatapults : public AddonList
{
    public:
        AddonLimitCatapults() : AddonList(ADDON_LIMIT_CATAPULTS,
                                              ADDONGROUP_MILITARY,
                                              _("Limit count of catapults"),
                                              _("Limits the count of catapults per player.\n\n"
                                                      "Proportional uses 0.125 catapult per barrack (you need 8 baracks for one catapult)\n"
                                                      "                  0.25           per guardhouse (4 guardhouses per catapult)\n"
                                                      "                  0.5            per watchtower (2 watchtower per catapult)\n"
                                                      "                  1              per fortress (1 fortress per catapult)"),
                                              0
                                             )
        {
            addOption(_("Unlimited"));
            addOption(_("Proportional"));
            addOption(_("No catapults"));
            addOption(_("3 catapults"));
            addOption(_("5 catapults"));
            addOption(_("10 catapults"));
            addOption(_("20 catapults"));
            addOption(_("30 catapults"));
        }
};

#endif // !ADDONLIMITCATAPULTS_H_INCLUDED
