#ifndef ADDONINEXHAUSTIBLEGRANITEMINES_H_INCLUDED
#define ADDONINEXHAUSTIBLEGRANITEMINES_H_INCLUDED

#pragma once

#include "Addons.h"

///////////////////////////////////////////////////////////////////////////////
/**
*  Addon for allowing to have unlimited resources.
*
*  @author FloSoft
*/
class AddonInexhaustibleGraniteMines : public AddonBool
{
    public:
        AddonInexhaustibleGraniteMines() : AddonBool(ADDON_INEXHAUSTIBLE_GRANITEMINES,
                    ADDONGROUP_ECONOMY,
                    gettext_noop("Inexhaustible Granite Mines"),
                    gettext_noop("Allows to have unlimited Granite resources.\n\n"
                                 "Granite mines will never be depleted."),
                    0
                                                        )
        {
        }
};

#endif // !ADDONINEXHAUSTIBLEGRANITEMINES_H_INCLUDED