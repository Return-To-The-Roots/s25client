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
*  Addon which controls the behavior of the coin order after capturing the military building
*/
class AddonCoinsCapturedBld: public AddonList
{
public:
    AddonCoinsCapturedBld()
        : AddonList(AddonId::COINS_CAPTURED_BLD, ADDONGROUP_MILITARY, _("Sets the coin order for captured buildings"),
            _("Sets the coin order captured military buildings."), 0)
    {
        this->addOption(_("Keep setting"));
        this->addOption(_("Enable"));
        this->addOption(_("Disable"));
    }
};

#endif // !COINSCAPTUREDBLD_H_INCLUDED
