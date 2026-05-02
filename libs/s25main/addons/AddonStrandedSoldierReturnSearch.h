// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonList.h"
#include "mygettext/mygettext.h"

class AddonStrandedSoldierReturnSearch : public AddonList
{
public:
    AddonStrandedSoldierReturnSearch()
        : AddonList(
          AddonId::STRANDED_SOLDIER_RETURN_SEARCH, AddonGroup::Military, _("Stranded soldier return search"),
          _("Controls the search radius used by stranded soldiers when looking for a way back to a warehouse."),
          {
            _("Default search range (1x)"),
            _("Reduced search range (0.5x)"),
            _("Extended search range (2x)"),
            _("Very large search range (4x)"),
          })
    {}
};
