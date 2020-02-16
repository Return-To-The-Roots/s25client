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
#ifndef ADDONREFUNDMATERIALS_H_INCLUDED
#define ADDONREFUNDMATERIALS_H_INCLUDED

#pragma once

#include "AddonList.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for refunding materials soon as a building gets destroyed.
 */
class AddonRefundMaterials : public AddonList
{
public:
    AddonRefundMaterials()
        : AddonList(AddonId::REFUND_MATERIALS, AddonGroup::Economy, _("Refund materials for destroyed buildings"),
                    _("Get building materials back when a building is destroyed."),
                    {
                      _("No refund"),
                      _("Refund 25%"),
                      _("Refund 50%"),
                      _("Refund 75%"),
                      _("Get all back"),
                    })
    {}
};

#endif // !ADDONREFUNDMATERIALS_H_INCLUDED
