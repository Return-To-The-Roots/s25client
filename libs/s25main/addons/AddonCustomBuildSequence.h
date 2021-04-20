// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for allowing a custom build sequence
 */
class AddonCustomBuildSequence : public AddonBool
{
public:
    AddonCustomBuildSequence()
        : AddonBool(AddonId::CUSTOM_BUILD_SEQUENCE, AddonGroup::Economy | AddonGroup::GamePlay,
                    _("Custom build sequence"),
                    _("Allows every player to control whether building sites should be supplied "
                      "in sequence of given order or in a definable sequence based on the building type."))
    {}
};
