// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/OptionalEnum.h"
#include <optional>
#include <ostream>

namespace helpers {
template<typename T>
std::ostream& operator<<(std::ostream& os, OptionalEnum<T> const& v)
{
    return (v) ? os << *v : os << "[empty]";
}
} // namespace helpers

namespace std {
// LCOV_EXCL_START
template<typename T>
static std::ostream& boost_test_print_type(std::ostream& os, std::optional<T> const& v)
{
    return (v) ? os << *v : os << "[empty]";
}
// LCOV_EXCL_STOP
} // namespace std
