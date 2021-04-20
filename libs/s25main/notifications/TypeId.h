// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>

/** Class for getting a unique Id per type: TypeId::value<int>()
    Note: NOT constant over different program version and NOT thread safe */
class TypeId
{
    static uint32_t counter;

public:
    template<typename T>
    static uint32_t value()
    {
        static uint32_t id = counter++;
        return id;
    }
};
