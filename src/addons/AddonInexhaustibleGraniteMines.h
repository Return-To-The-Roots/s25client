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
        : AddonBool(AddonId::INEXHAUSTIBLE_GRANITEMINES, ADDONGROUP_ECONOMY, _("Inexhaustible Granite Mines"),
                    _("Granite mines will never be depleted."), 0)
    {}
};

#endif // !ADDONINEXHAUSTIBLEGRANITEMINES_H_INCLUDED
