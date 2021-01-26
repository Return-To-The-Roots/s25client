// Copyright (c) 2021 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

#pragma once

#include "s25util/strAlgos.h"
#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

#define RTTR_LUA_REG_SINGLE_2(EnumName, EPrefix, Enumerator) \
    state[(EPrefix) + s25util::toUpper(#Enumerator)] = EnumName::Enumerator;
#define RTTR_LUA_REG_SINGLE(s, EnumNamePrefix, Enumerator) \
    RTTR_LUA_REG_SINGLE_2(BOOST_PP_SEQ_ELEM(0, EnumNamePrefix), BOOST_PP_SEQ_ELEM(1, EnumNamePrefix), Enumerator)

/// Register an enum for use in lua.
/// For each enumerator there will be a lua constant `<Prefix>uppercase(enumerator)`
/// Example: Enum::Value with Prefix "E_" --> state["E_VALUE"] = Enum::Value
///
/// Usage: RTTR_LUA_REGISTER_ENUM(MyEnum, Prefix, Value1, Value2, Value3, ...)
#define RTTR_LUA_REGISTER_ENUM(EnumName, EPrefix, ...)                                                         \
    do                                                                                                         \
    {                                                                                                          \
        BOOST_PP_SEQ_FOR_EACH(RTTR_LUA_REG_SINGLE, (EnumName)(EPrefix), BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) \
    } while(false)
