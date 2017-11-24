// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef ADDONEXHAUSTIBLEWELLS_H_INCLUDED
#define ADDONEXHAUSTIBLEWELLS_H_INCLUDED

#pragma once

#include "AddonList.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for allowing to have unlimited resources.
 */
class AddonExhaustibleWells : public AddonList
{
public:
    AddonExhaustibleWells()
        : AddonList(AddonId::EXHAUSTIBLE_WELLS, ADDONGROUP_ECONOMY, _("Exhaustible Wells"),
                    _("Wells will now dry out, limiting the available water"), 0)
    {
        addOption(_("Disabled"));
        addOption(_("Disabled and no notification"));
        addOption(_("Use"));
    }
};

#endif // !ADDONEXHAUSTIBLEWELLS_H_INCLUDED
