#ifndef ADDONINEXHAUSTIBLEGRANITEMINES_H_INCLUDED
#define ADDONINEXHAUSTIBLEGRANITEMINES_H_INCLUDED

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for allowing to have unlimited resources.
 */
class AddonInexhaustibleGraniteMines : public AddonBool
{
public:
    AddonInexhaustibleGraniteMines()
        : AddonBool(AddonId::INEXHAUSTIBLE_GRANITEMINES, AddonGroup::Economy, _("Inexhaustible Granite Mines"),
                    _("Granite mines will never be depleted."))
    {}
};

#endif // !ADDONINEXHAUSTIBLEGRANITEMINES_H_INCLUDED
