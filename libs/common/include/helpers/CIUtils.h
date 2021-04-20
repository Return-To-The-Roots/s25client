// Copyright (C) 2018 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdlib>
#include <string>

namespace rttr {
inline bool isRunningOnCI()
{
    const auto* ciPtr = std::getenv("CI");
    if(!ciPtr)
        return false;
    const std::string ci = ciPtr;
    return ci == "true" || ci == "True";
}
} // namespace rttr
