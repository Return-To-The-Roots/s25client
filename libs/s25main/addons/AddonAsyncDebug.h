// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonBool.h"

/**
 *  Addon for a debugging asyncs
 */
class AddonAsyncDebug : public AddonBool
{
public:
    AddonAsyncDebug()
        : AddonBool(AddonId::ASYNC_DEBUG, AddonGroup::Other, _("Async debugging (REALLY SLOW!)"),
                    _("Enables extra stuff to debug asyncs. Do not enable unless you know what you are doing!"))
    {}
};
