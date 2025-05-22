// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
namespace helpers {

template<class D = void, class... Types>
constexpr auto make_array(Types&&... t)
{
    using ResultType = std::conditional_t<std::is_same_v<D, void>, std::common_type_t<Types...>, D>;
    return std::array<ResultType, sizeof...(Types)>{{ResultType(std::forward<Types>(t))...}};
}

} // namespace helpers
