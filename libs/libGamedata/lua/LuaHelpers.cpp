// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LuaHelpers.h"
#include "LuaInterfaceBase.h"

namespace lua {

void assertTrue(bool testValue, const std::string& error)
{
    if(!testValue)
        throw LuaExecutionError(error);
}

void validatePath(const std::string& path)
{
    std::string errorStart = std::string("Invalid path '") + path + "': ";
    assertTrue(path.find("<RTTR_") == 0, errorStart + "Must start with <RTTR_");
    assertTrue(path.find("..") == std::string::npos, errorStart + "Must not contain '..'");
}

} // namespace lua
