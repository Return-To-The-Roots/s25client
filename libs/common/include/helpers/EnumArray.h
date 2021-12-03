// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "MaxEnumValue.h"
#include "enum_cast.hpp"
#include <array>

namespace helpers {

/// Array that is meant to be indexed with enum values instead of integrals
/// <typeparam name="T_Elements">Type of the elements</typeparam>
/// <typeparam name="T_Index">Type of the indices</typeparam>
template<typename T_Elements, typename T_Index>
struct EnumArray
{
    using value_type = T_Elements;
    using iterator = T_Elements*;
    using const_iterator = const T_Elements*;

    constexpr T_Elements* data() { return elems; }
    constexpr const T_Elements* data() const { return elems; }
    static constexpr unsigned size() { return helpers::MaxEnumValue_v<T_Index> + 1u; }
    constexpr bool empty() const { return false; }

    constexpr T_Elements& operator[](T_Index idx) noexcept { return elems[rttr::enum_cast(idx)]; }
    constexpr const T_Elements& operator[](T_Index idx) const noexcept { return elems[rttr::enum_cast(idx)]; }

    iterator begin() noexcept { return elems; }
    constexpr const_iterator begin() const noexcept { return elems; }
    iterator end() noexcept { return elems + size(); }
    constexpr const_iterator end() const noexcept { return elems + size(); }

    constexpr bool operator==(const EnumArray& rhs)
    {
        for(unsigned i = 0; i < size(); ++i)
        {
            if(elems[i] != rhs.elems[i])
                return false;
        }
        return true;
    }
    constexpr bool operator!=(const EnumArray& rhs) { return !(*this == rhs); }

    T_Elements elems[size()];
};

namespace detail {
    template<typename T_Index, typename T, std::size_t... I>
    constexpr auto toEnumArrayImpl(const std::array<T, sizeof...(I)>& src, std::index_sequence<I...>)
    {
        return helpers::EnumArray<T, T_Index>{src[I]...};
    }
    template<typename T, typename... T_Indices>
    struct GetMultiEnumArray
    {
        using type = T;
    };
    template<typename T, typename T_Index, typename... T_Indices>
    struct GetMultiEnumArray<T, T_Index, T_Indices...>
    {
        using type = EnumArray<typename GetMultiEnumArray<T, T_Indices...>::type, T_Index>;
    };
} // namespace detail

/// Shortcut for creating ND EnumArrays similar to int[5][4]
/// MultiEnumArray<Value, MyEnum1, MyEnum2> --> EnumArray<EnumArray<Value, MyEnum2>, MyEnum1>
template<typename T, typename... T_Indices>
using MultiEnumArray = typename detail::GetMultiEnumArray<T, T_Indices...>::type;

/// Convert a std::array to an EnumArray
template<typename T_Index, typename T>
constexpr auto toEnumArray(const std::array<T, helpers::NumEnumValues_v<T_Index>>& src)
{
    return detail::toEnumArrayImpl<T_Index>(src, std::make_index_sequence<helpers::NumEnumValues_v<T_Index>>());
}
} // namespace helpers
