#ifndef ADDONINEXHAUSTIBLEFISH_H_INCLUDED
#define ADDONINEXHAUSTIBLEFISH_H_INCLUDED

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

/**
 *  addon decativates reduction of fish population by fishing.
 */
class AddonInexhaustibleFish : public AddonBool
{
public:
    AddonInexhaustibleFish()
        : AddonBool(AddonId::INEXHAUSTIBLE_FISH, ADDONGROUP_ECONOMY, _("Inexhaustible Fish"),
                    _("Deactivates reduction of fish population."), 0)
    {
    }
};

#endif
