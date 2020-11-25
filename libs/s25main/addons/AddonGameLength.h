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

/**
 *  Addon allows users to adjust the game length (for Economy Mode)
 */
const unsigned int AddonGameLengthList[] = {0,           15 * 60000,  30 * 60000,  60 * 60000, 90 * 60000, 120 * 60000, 150 * 60000, 180 * 60000,
  240 * 60000, 480 * 60000}; // length in ms

class AddonGameLength: public AddonList
{
public:
    AddonGameLength()
        : AddonList(
          AddonId::GAME_LENGTH, AddonGroup::Economy | AddonGroup::GamePlay, _("Game Length (Economy Mode)"),
                    _("Adjust the time after which the economy mode victory condition is checked. Reference times are on fast speed."),
                    {
                      _("unlimited"),
                      _("~15min"),
                      _("~30min"),
                      _("~60min"),
                      _("~90min"),
                      _("~120min"),
                      _("~150min"),
                      _("~180min"),
                      _("~240min"),
                      _("~480min")
                    })
    {}
};
