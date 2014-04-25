#ifndef NOCOINSDEFAULT_H_INCLUDED
#define NOCOINSDEFAULT_H_INCLUDED

#pragma once

#include "Addons.h"

///////////////////////////////////////////////////////////////////////////////
/**
 *  Addon for disable coins by default
 *
 *  @author MrHase
 */
class AddonNoCoinsDefault : public AddonBool
{
    public:
        AddonNoCoinsDefault() : AddonBool(ADDON_NO_COINS_DEFAULT,
                                              ADDONGROUP_MILITARY,
                                              gettext_noop("Disable coins by default"),
                                              gettext_noop("Receiving coins is disabled\n"
                                                      "for military buildings by default."),
                                              0
                                             )
        {
        }
};

#endif // !NOCOINSDEFAULT_H_INCLUDED
