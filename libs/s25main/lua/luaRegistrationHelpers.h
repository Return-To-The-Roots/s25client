// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
