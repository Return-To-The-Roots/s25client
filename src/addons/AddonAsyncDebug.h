// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef ADDONASYNCDEBUG_H_INCLUDED
#define ADDONASYNCDEBUG_H_INCLUDED


#include "Addons.h"

///////////////////////////////////////////////////////////////////////////////
/**
 *  Addon for a debugging asyncs
 *
 *  @author Maqs
 */
class AddonAsyncDebug : public AddonBool
{
    public:
        AddonAsyncDebug() : AddonBool(AddonId::ASYNC_DEBUG,
                                          ADDONGROUP_OTHER,
                                          _("Async debugging (REALLY SLOW!)"),
                                          _("Enables extra stuff to debug asyncs. Do not enable unless you know what you are doing!"),
                                          0
                                         )
        {
        }
};

#endif // !ADDONASYNCDEBUG_H_INCLUDED
