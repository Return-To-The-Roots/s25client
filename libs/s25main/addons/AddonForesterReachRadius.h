// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonList.h"
#include "mygettext/mygettext.h"

const std::array<unsigned, 6> foresterRadiusValues = {
  6,  // Default
  8,  // +33%
  10, // +66%
  12, // +100%
  15, // +150%
  18  // +200%
};

class AddonForesterReachRadius : public AddonList
{
public:
    AddonForesterReachRadius()
        : AddonList(AddonId::FORESTER_REACH_RADIUS, AddonGroup::GamePlay, _("Adjust forester's range"),
                    _("Increase the radius in which the forester plants trees."),
                    {_("Default"), _("+33%"), _("+66%"), _("+100%"), _("+150%"), _("+200%")})
    {}
};
