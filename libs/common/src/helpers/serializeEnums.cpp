// Copyright (c) 2021 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "helpers/serializeEnums.h"
#include "helpers/format.hpp"

std::range_error helpers::makeOutOfRange(unsigned value, unsigned maxValue)
{
    return std::range_error(helpers::format("%s is out of range. Maximum allowed value: %s", value, maxValue));
}
