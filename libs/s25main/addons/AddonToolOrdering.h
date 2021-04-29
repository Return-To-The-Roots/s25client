// Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"

class AddonToolOrdering : public AddonBool
{
public:
    AddonToolOrdering()
        : AddonBool(AddonId::TOOL_ORDERING, AddonGroup::GamePlay, _("Tool ordering"),
                    _("Allows to order a specific amount of tools for priority production."))
    {}
};
