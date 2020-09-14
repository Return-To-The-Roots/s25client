// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#pragma once

#include "AddonList.h"
#include "mygettext/mygettext.h"
#include "s25util/warningSuppression.h"
#include <array>

const std::array<unsigned, 6> SUPPRESS_UNUSED waterwayLengths = {{3, 5, 9, 13, 21, 0}};

/**
 *  Addon for changing the maximum length of waterways.
 */
class AddonMaxWaterwayLength : public AddonList
{
public:
    AddonMaxWaterwayLength()
        : AddonList(AddonId::MAX_WATERWAY_LENGTH, AddonGroup::GamePlay, _("Set maximum waterway length"),
                    _("Limits the distance settlers may travel per boat.\n\n"
                      "Possible values are:\n"
                      "Short: 3 tiles\n"
                      "Default: 5 tiles\n"
                      "Long: 9 tiles\n"
                      "Longer: 13 tiles\n"
                      "Very long: 21 tiles\n"
                      "and Unlimited."),
                    {
                      _("Short"),
                      _("Default"),
                      _("Long"),
                      _("Longer"),
                      _("Very long"),
                      _("Unlimited"),
                    },
                    1)
    {}
};
