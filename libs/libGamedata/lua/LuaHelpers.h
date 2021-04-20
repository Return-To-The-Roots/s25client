// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

namespace lua {

/// If the given value is not true, a runtime error with the given description is thrown
void assertTrue(bool testValue, const std::string& error);
/// Validate the path. A valid path starts with '<RTTR_' and contains no '..'
void validatePath(const std::string& path);

} // namespace lua
