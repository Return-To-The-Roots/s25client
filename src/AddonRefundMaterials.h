// $Id: AddonRefundMaterials.h 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef ADDONREFUNDMATERIALS_H_INCLUDED
#define ADDONREFUNDMATERIALS_H_INCLUDED

#pragma once

#include "Addons.h"

///////////////////////////////////////////////////////////////////////////////
/**
 *  Addon for refunding materials soon as a building gets destroyed.
 *
 *  @author FloSoft
 */
class AddonRefundMaterials : public AddonList
{
    public:
        AddonRefundMaterials() : AddonList(ADDON_REFUND_MATERIALS,
                                               ADDONGROUP_ECONOMY,
                                               gettext_noop("Refund materials when building is destroyed"),
                                               gettext_noop("Allows you to get building materials back if the building is destroyed."),
                                               0
                                              )
        {
            addOption(gettext_noop("No refund"));
            addOption(gettext_noop("Refund 25%"));
            addOption(gettext_noop("Refund 50%"));
            addOption(gettext_noop("Refund 75%"));
            addOption(gettext_noop("Get all back"));
        }
};

#endif // !ADDONREFUNDMATERIALS_H_INCLUDED

