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
#ifndef ADDONMAXWATERWAYLENGTH_H_INCLUDED
#define ADDONMAXWATERWAYLENGTH_H_INCLUDED

#pragma once

#include "Addons.h"
#include "mygettext/src/mygettext.h"
#include <boost/array.hpp>

const boost::array<unsigned, 6> SUPPRESS_UNUSED waterwayLengths = {{ 3, 5, 9, 13, 21, 0 }};

///////////////////////////////////////////////////////////////////////////////
/**
 *  Addon for changing the maximum length of waterways.
 *
 *  @author Divan
 */
class AddonMaxWaterwayLength : public AddonList
{
    public:
        AddonMaxWaterwayLength() : AddonList(AddonId::MAX_WATERWAY_LENGTH,
                                                 ADDONGROUP_GAMEPLAY,
                                                 _("Set maximum waterway length"),
                                                 _("Limits the distance settlers may travel per boat.\n\n"
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
            addOption(_("Short"));
            addOption(_("Default"));
            addOption(_("Long"));
            addOption(_("Longer"));
            addOption(_("Very long"));
            addOption(_("Unlimited"));
        }
};

#endif // !ADDONMAXWATERWAYLENGTH_H_INCLUDED
