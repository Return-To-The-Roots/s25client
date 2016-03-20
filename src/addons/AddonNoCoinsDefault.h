#ifndef NOCOINSDEFAULT_H_INCLUDED
#define NOCOINSDEFAULT_H_INCLUDED

#pragma once

#include "Addons.h"
#include "mygettext/src/mygettext.h"

///////////////////////////////////////////////////////////////////////////////
/**
 *  Addon for disable coins by default
 *
 *  @author MrHase
 */
class AddonNoCoinsDefault : public AddonBool
{
    public:
        AddonNoCoinsDefault() : AddonBool(AddonId::NO_COINS_DEFAULT,
                                              ADDONGROUP_MILITARY,
                                              _("Disable coins by default"),
                                              _("Receiving coins is disabled for military buildings by default."),
                                              0
                                             )
        {
        }
};

#endif // !NOCOINSDEFAULT_H_INCLUDED
