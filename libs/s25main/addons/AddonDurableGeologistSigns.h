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
#include <boost/format.hpp>

const std::array<unsigned, 4> SUPPRESS_UNUSED signDurabilityFactor = {{1, 2, 4, 10}};

/**
 *  Addon makes resource signs stay visible longer than normal
 */
class AddonDurableGeologistSigns : public AddonList
{
public:
    AddonDurableGeologistSigns()
        : AddonList(AddonId::DURABLE_GEOLOGIST_SIGNS, AddonGroup::Economy, _("Geologist sign durability"),
                    _("Increase the durability of geologist signs by a factor."),
                    {
                      _("Default"),
                      (boost::format(_("%1%x")) % signDurabilityFactor[1]).str(),
                      (boost::format(_("%1%x")) % signDurabilityFactor[2]).str(),
                      (boost::format(_("%1%x")) % signDurabilityFactor[3]).str(),
                    },
                    0)
    {}
};