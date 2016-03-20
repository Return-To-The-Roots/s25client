// Copyright (c) 2005 - 2013 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef ADDONMILITARYHITPOINTS_H_INCLUDED
#define ADDONMILITARYHITPOINTS_H_INCLUDED

#pragma once

#include "Addons.h"

///////////////////////////////////////////////////////////////////////////////
/**
*  Addon adds shows the hitpoints of a soldier above his picture in military buildings. 
*
*  @author Ribosom
*/
class AddonMilitaryHitpoints : public AddonBool
{
public:
	AddonMilitaryHitpoints() : AddonBool(AddonId::MILITARY_HITPOINTS,
		ADDONGROUP_GAMEPLAY | ADDONGROUP_MILITARY,
		_("Military Hitpoints"),
		_("Display the hitpoints of units in military buildings."),
		0
		)
	{
	}
};

#endif 
