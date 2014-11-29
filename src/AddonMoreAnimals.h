// $Id: AddonRefundMaterials.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef ADDONMOREANIMALS_H_INCLUDED
#define ADDONMOREANIMALS_H_INCLUDED

#pragma once

#include "Addons.h"

///////////////////////////////////////////////////////////////////////////////
/**
 *  Addon allows users to adjust the percentage of trees that have the recurring spawn animal event
 *
 *  @author PoC
 */
class AddonMoreAnimals : public AddonList
{
    public:
        AddonMoreAnimals() : AddonList(ADDON_MORE_ANIMALS,
                                               ADDONGROUP_ECONOMY,
                                               gettext_noop("More trees spawn animals"),
                                               gettext_noop("Allows you to adjust the percentage of trees that spawn animals."),
                                               0
                                              )
        {
            addOption(gettext_noop("default"));
            addOption(gettext_noop("Increase 50%"));
            addOption(gettext_noop("Increase 100%"));
            addOption(gettext_noop("Increase 200%"));
			addOption(gettext_noop("Increase 500%"));
            addOption(gettext_noop("Increase 1000%"));
        }
};

#endif // !ADDONREFUNDMATERIALS_H_INCLUDED

