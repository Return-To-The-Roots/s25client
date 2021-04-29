// Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonList.h"

class AddonMetalworksBehaviorOnZero : public AddonList
{
public:
    AddonMetalworksBehaviorOnZero()
        : AddonList(AddonId::METALWORKSBEHAVIORONZERO, AddonGroup::GamePlay | AddonGroup::Economy,
                    _("Change metalworks behavior on zero"),
                    _("Change the working behavior of metalworks if all sliders in the tools window are set to zero.\n"
                      "Produce random ware: S2-Default\n"
                      "Produce nothing: RttR-Default"),
                    {
                      _("Produce random ware"),
                      _("Produce nothing"),
                    })
    {}
};
