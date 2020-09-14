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
        : AddonBool(AddonId::INEXHAUSTIBLE_FISH, AddonGroup::Economy, _("Inexhaustible Fish"),
                    _("Deactivates reduction of fish population."))
    {}
};
