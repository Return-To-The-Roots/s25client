// Copyright (c) 2005 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef COINSCAPTUREDBLD_H_INCLUDED
#define COINSCAPTUREDBLD_H_INCLUDED

#pragma once

#include "AddonList.h"
#include "mygettext/mygettext.h"

/**
 *  Addon which controls if coins should be enabled/disabled on capture
 */
class AddonCoinsCapturedBld : public AddonList
{
public:
    AddonCoinsCapturedBld()
        : AddonList(AddonId::COINS_CAPTURED_BLD, AddonGroup::Military, _("Coins on captured buildings"),
                    _("Change the coin setting for captured military buildings."),
                    {
                      _("Keep setting"),
                      _("Enable"),
                      _("Disable"),
                    })
    {}
};

#endif // !COINSCAPTUREDBLD_H_INCLUDED
