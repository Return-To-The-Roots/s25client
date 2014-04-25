// $Id: AddonAIDebugWindow.h 9357 2014-04-25 15:35:25Z FloSoft $
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

#ifndef ADDONAIDEBUGWINDOW_H_INCLUDED
#define ADDONAIDEBUGWINDOW_H_INCLUDED

#pragma once

#include "Addons.h"

///////////////////////////////////////////////////////////////////////////////
/**
 *  Addon for disable the AI debugging window
 *
 *  @author jh
 */
class AddonAIDebugWindow : public AddonBool
{
    public:
        AddonAIDebugWindow() : AddonBool(ADDON_AI_DEBUG_WINDOW,
                                             ADDONGROUP_OTHER,
                                             gettext_noop("AI Debugging Window"),
                                             gettext_noop("Enable AI Debugging Window\n"
                                                     "(possible cheating)"),
                                             0
                                            )
        {
        }
};

#endif // !ADDONAIDEBUGWINDOW_H_INCLUDED
