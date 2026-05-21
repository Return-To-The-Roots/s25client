// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonList.h"
#include "mygettext/mygettext.h"

const std::array<unsigned, 7> stonemasonRadiusValues = {
  8,  // Default
  10, // +25%
  12, // +50%
  14, // +75%
  16, // +100%
  20, // +150%
  24  // +200%
};

class AddonStonemasonReachRadius : public AddonList
{
public:
    AddonStonemasonReachRadius()
        : AddonList(AddonId::STONEMASON_REACH_RADIUS, AddonGroup::GamePlay, _("Adjust stonemason's range"),
                    _("Increase the radius in which the stonemason searches for stone."),
                    {
                      _("Default"),
                      _("+25%"),
                      _("+50%"),
                      _("+75%"),
                      _("+100%"),
                      _("+150%"),
                      _("+200%"),
                    })
    {}
};
