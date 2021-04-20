// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for disable the AI debugging window
 */
class AddonAIDebugWindow : public AddonBool
{
public:
    AddonAIDebugWindow()
        : AddonBool(AddonId::AI_DEBUG_WINDOW, AddonGroup::Other, _("AI Debugging Window"),
                    _("Enable AI Debugging Window\n"
                      "(possible cheating)"))
    {}
};
