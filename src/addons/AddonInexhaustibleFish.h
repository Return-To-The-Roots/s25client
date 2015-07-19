#ifndef ADDONINEXHAUSTIBLEFISH_H_INCLUDED
#define ADDONINEXHAUSTIBLEFISH_H_INCLUDED

#pragma once

#include "Addons.h"

///////////////////////////////////////////////////////////////////////////////
/**
*  addon decativates reduction of fish population by fishing.
*
*  @author PoC
*/
class AddonInexhaustibleFish : public AddonBool
{
    public:
        AddonInexhaustibleFish() : AddonBool(ADDON_INEXHAUSTIBLE_FISH,
                                                 ADDONGROUP_ECONOMY,
                                                 gettext_noop("Inexhaustible Fish"),
                                                 gettext_noop("Deactivates reduction of fish population.\n"),
                                                 0
                                                )
        {
        }
};

#endif