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
#ifndef ADDONMAXRANK_H_INCLUDED
#define ADDONMAXRANK_H_INCLUDED

#pragma once

#include "Addons.h"

///////////////////////////////////////////////////////////////////////////////
/**
 *  limit max rank of soldiers
 *
 *  rank 4 General Default
 *
 *  rank 3 Officer
 *
 *  rank 2 Sergeant
 *
 *  rank 1 Privatefirstclass
 *
 *  rank 0 Private
 *
 *  @author poc
 */
class AddonMaxRank : public AddonList
{
    public:
        AddonMaxRank() : AddonList(ADDON_MAX_RANK,
                                       ADDONGROUP_MILITARY,
                                       gettext_noop("Set max rank"),
                                       gettext_noop("Allows you to select the highest rank for soldiers\n\n"),
                                       0
                                      )
        {
            addOption(gettext_noop("General (4)"));
            addOption(gettext_noop("Officer (3)"));
            addOption(gettext_noop("Sergeant (2)"));
            addOption(gettext_noop("Privatefc (1)"));
            addOption(gettext_noop("Private (0)"));
        }
};

#endif

