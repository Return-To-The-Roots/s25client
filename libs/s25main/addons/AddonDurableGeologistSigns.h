// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
