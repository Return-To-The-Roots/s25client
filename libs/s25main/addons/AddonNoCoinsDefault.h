#ifndef NOCOINSDEFAULT_H_INCLUDED
#define NOCOINSDEFAULT_H_INCLUDED

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for disable coins by default
 */
class AddonNoCoinsDefault : public AddonBool
{
public:
    AddonNoCoinsDefault()
        : AddonBool(AddonId::NO_COINS_DEFAULT, AddonGroup::Military, _("Disable coins by default"),
                    _("Receiving coins is disabled for military buildings by default."))
    {}
};

#endif // !NOCOINSDEFAULT_H_INCLUDED
