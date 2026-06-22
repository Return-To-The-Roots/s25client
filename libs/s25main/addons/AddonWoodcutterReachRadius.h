// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonList.h"
#include "mygettext/mygettext.h"

const std::array<unsigned, 6> woodcutterRadiusValues = {
  6,  // Default
  8,  // +33%
  10, // +66%
  12, // +100%
  15, // +150%
  18  // +200%
};

class AddonWoodcutterReachRadius : public AddonList
{
public:
    AddonWoodcutterReachRadius()
        : AddonList(AddonId::WOODCUTTER_REACH_RADIUS, AddonGroup::GamePlay, _("Adjust woodcutter's range"),
                    _("Increase the radius in which the woodcutter searches for trees."),
                    {_("Default"), _("+33%"), _("+66%"), _("+100%"), _("+150%"), _("+200%")})
    {}
};
