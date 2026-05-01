// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"
#include "const_addons.h"
#include "mygettext/mygettext.h"

/**
 *  Addon for creating finite granite resources below granite mines without explicit stone resources.
 */
class AddonGraniteMinesWorkEverywhere : public AddonBool
{
public:
    AddonGraniteMinesWorkEverywhere()
        : AddonBool(AddonId::GRANITEMINES_WORK_EVERYWHERE, AddonGroup::Economy, _("Granite Mines Work Everywhere"),
                    _("Granite mines can create a finite stone resource on otherwise empty mountain spots."))
    {}
};
