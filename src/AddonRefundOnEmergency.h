// $Id: AddonRefundOnEmergency.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef ADDONREFUNDONEMERGENCY_H_INCLUDED
#define ADDONREFUNDONEMERGENCY_H_INCLUDED

#pragma once

#include "Addons.h"

///////////////////////////////////////////////////////////////////////////////
/**
 *  Addon for refunding materials soon as a building gets destroyed and
 *  the emergency program is active
 *
 *  @author FloSoft
 */
class AddonRefundOnEmergency : public AddonBool
{
    public:
        AddonRefundOnEmergency() : AddonBool(ADDON_REFUND_ON_EMERGENCY,
                                                 ADDONGROUP_ECONOMY,
                                                 gettext_noop("Refund materials when emergency program is active"),
                                                 gettext_noop("Allows you to get building materials back if the building is destroyed\n"
                                                         "and your emergency program is active."),
                                                 0
                                                )
        {
        }
};

#endif // !ADDONREFUNDONEMERGENCY_H_INCLUDED
