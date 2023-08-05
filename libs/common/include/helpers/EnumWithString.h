// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <array>
#include <cstring>
#include <string>

namespace rttrEnum {
namespace detail {
    template<class T_Enum>
    struct Tag
    {};
    constexpr void getValues(...);
    constexpr void getRawNames(...);
} // namespace detail

template<class T>
struct EnumData
{
    static constexpr decltype(getValues(detail::Tag<T>{})) values = getValues(detail::Tag<T>{});
    static constexpr decltype(getRawNames(detail::Tag<T>{})) rawNames = getRawNames(detail::Tag<T>{});
    static constexpr size_t size = values.size();
};

template<class T>
constexpr std::size_t size = EnumData<T>::size;
template<class T>
constexpr auto values = EnumData<T>::values;

template<class T>
std::string toString(T value)
{
    for(size_t index = 0; index < size<T>; ++index)
    {
        if(EnumData<T>::values[index] == value)
        {
            const char* const rawName = EnumData<T>::rawNames[index];
            size_t length = std::strcspn(rawName, " =\t\n\r");
            return {rawName, rawName + length};
        }
    }
    return {};
}

namespace detail {
    template<class T>
    struct ignore_assign
    {
        constexpr ignore_assign(T value) : value_(value) {}
        constexpr operator T() const { return value_; }

        // NOLINTNEXTLINE(misc-unconventional-assign-operator)
        constexpr const ignore_assign& operator=(int) const { return *this; }

        T value_;
    };
} // namespace detail
} // namespace rttrEnum

// NOLINTNEXTLINE(bugprone-macro-parentheses)
#define IGNORE_ASSIGN_SINGLE(s, EnumName, expression) (detail::ignore_assign<EnumName>)EnumName::expression,
#define IGNORE_ASSIGN(EnumName, ...) \
    BOOST_PP_SEQ_FOR_EACH(IGNORE_ASSIGN_SINGLE, EnumName, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define STRINGIZE_2(x) #x
#define STRINGIZE_SINGLE(s, data, expression) STRINGIZE_2(expression),
#define STRINGIZE(...) BOOST_PP_SEQ_FOR_EACH(STRINGIZE_SINGLE, 0, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

/// Generates an enum class
/// Provides rttrEnum::size, rttrEnum::values, rttrEnum::toString
///
/// Usage: Put ENUM_WITH_STRING(MyFancyEnum, Value1, Value2 = 42, Value3) in a header
#define ENUM_WITH_STRING(EnumName, ...) \
    ENUM_WITH_STRING_IMPL(EnumName, BOOST_PP_SEQ_SIZE(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)), __VA_ARGS__)

#define ENUM_WITH_STRING_IMPL(EnumName, size, ...)                                                                \
                                                                                                                  \
    enum class EnumName                                                                                           \
    {                                                                                                             \
        __VA_ARGS__                                                                                               \
    };                                                                                                            \
                                                                                                                  \
    namespace rttrEnum::detail {                                                                                  \
        constexpr std::array<EnumName, size> getValues(Tag<EnumName>)                                             \
        {                                                                                                         \
            return {{IGNORE_ASSIGN(EnumName, __VA_ARGS__)}};                                                      \
        }                                                                                                         \
        constexpr std::array<const char*, size> getRawNames(Tag<EnumName>) { return {{STRINGIZE(__VA_ARGS__)}}; } \
    }
