#ifndef ADDONINEXHAUSTIBLEFISH_H_INCLUDED
#define ADDONINEXHAUSTIBLEFISH_H_INCLUDED

#pragma once

#include "Addons.h"
#include "mygettext/src/mygettext.h"

///////////////////////////////////////////////////////////////////////////////
/**
*  addon decativates reduction of fish population by fishing.
*
*  @author PoC
*/
class AddonInexhaustibleFish : public AddonBool
{
    public:
        AddonInexhaustibleFish() : AddonBool(AddonId::INEXHAUSTIBLE_FISH,
                                                 ADDONGROUP_ECONOMY,
                                                 _("Inexhaustible Fish"),
                                                 _("Deactivates reduction of fish population."),
                                                 0
                                                )
        {
        }
};

#endif
