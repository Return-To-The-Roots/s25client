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

#pragma once
#ifndef libs_libGamedata_lua_LuaHelpers_h
#define libs_libGamedata_lua_LuaHelpers_h

#include <string>

namespace lua {

/// If the given value is not true, a runtime error with the given description is thrown
void assertTrue(bool testValue, const std::string& error);
/// Validate the path. A valid path starts with '<RTTR_' and contains no '..'
void validatePath(const std::string& path);

} // namespace lua

#endif // !libs_libGamedata_lua_LuaHelpers_h
