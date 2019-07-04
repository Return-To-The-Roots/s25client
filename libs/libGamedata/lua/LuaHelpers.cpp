// Copyright (c) 2005 - 2019 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LuaHelpers.h"
#include "LuaInterfaceBase.h"

namespace lua {

void assertTrue(bool testValue, const std::string& error)
{
    if(!testValue)
    {
        throw LuaExecutionError(error);
    }
}

void validatePath(const std::string& path)
{
    const std::string errorPrefix( "Invalid path '" + path + "': " );
    assertTrue(path.find("<RTTR_") == 0, errorPrefix + "Must start with <RTTR_");
    assertTrue(path.find("..") == std::string::npos, errorPrefix + "Must not contain '..'");
}

} // namespace lua
