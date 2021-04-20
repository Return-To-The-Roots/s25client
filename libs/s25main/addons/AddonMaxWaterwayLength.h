// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
