// $Id: AddonCharburner.h 6582 2010-07-16 11:23:35Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.
#ifndef ADDONTRADE_H_INCLUDED
#define ADDONTRADE_H_INCLUDED


#include "Addons.h"

///////////////////////////S////////////////////////////////////////////////////
/**
 *  Addon for a Charburner
 *
 *  @author OLiver
 */
class AddonTrade : public AddonBool
{
    public:
        AddonTrade() : AddonBool(ADDON_TRADE,
                                     ADDONGROUP_ECONOMY,
                                     gettext_noop("Trade"),
                                     gettext_noop("Allows to send wares/figures to allied warehouses"),
                                     0
                                    )
        {
        }
};

#endif // !ADDONEXHAUSTIBLEWELLS_H_INCLUDED
