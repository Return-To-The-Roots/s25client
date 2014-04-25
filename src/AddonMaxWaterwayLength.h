// $Id: AddonMaxWaterwayLength.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef ADDONMAXWATERWAYLENGTH_H_INCLUDED
#define ADDONMAXWATERWAYLENGTH_H_INCLUDED

#pragma once

#include "Addons.h"

///////////////////////////////////////////////////////////////////////////////
/**
 *  Addon for changing the maximum length of waterways.
 *
 *  @author Divan
 */
class AddonMaxWaterwayLength : public AddonList
{
    public:
        AddonMaxWaterwayLength() : AddonList(ADDON_MAX_WATERWAY_LENGTH,
                                                 ADDONGROUP_GAMEPLAY,
                                                 gettext_noop("Set maximum waterway length"),
                                                 gettext_noop("Limits the distance settlers may travel per boat.\n\n"
                                                         "Possible values are:\n"
                                                         "Short: 3 tiles\n"
                                                         "Default: 5 tiles\n"
                                                         "Long: 9 tiles\n"
                                                         "Longer: 13 tiles\n"
                                                         "Very long: 21 tiles\n"
                                                         "and Unlimited."),
                                                 1
                                                )
        {
            addOption(gettext_noop("Short"));
            addOption(gettext_noop("Default"));
            addOption(gettext_noop("Long"));
            addOption(gettext_noop("Longer"));
            addOption(gettext_noop("Very long"));
            addOption(gettext_noop("Unlimited"));
        }
};

#endif // !ADDONMAXWATERWAYLENGTH_H_INCLUDED
