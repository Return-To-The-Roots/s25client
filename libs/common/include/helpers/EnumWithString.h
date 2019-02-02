// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef EnumWithString_h__
#define EnumWithString_h__

#include <boost/preprocessor.hpp>
#include <cstring>

namespace detail {
struct ignore_assign
{
    ignore_assign(int value) : value_(value) {}
    operator int() const { return value_; }

    const ignore_assign& operator=(int) { return *this; }

    int value_;
};
} // namespace detail

#define IGNORE_ASSIGN_SINGLE(s, data, expression) (detail::ignore_assign) expression,
#define IGNORE_ASSIGN(...) BOOST_PP_SEQ_FOR_EACH(IGNORE_ASSIGN_SINGLE, 0, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define STRINGIZE_2(x) #x
#define STRINGIZE_SINGLE(s, data, expression) STRINGIZE_2(expression),
#define STRINGIZE(...) BOOST_PP_SEQ_FOR_EACH(STRINGIZE_SINGLE, 0, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define __HAS_COMMA(...) BOOST_PP_VARIADIC_ELEM(15, __VA_ARGS__, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0)
#define __TRIGGER_PARENTHESIS_(...) ,

#define IS_EMPTY(...)                                                                       \
    /* test if placing it between _TRIGGER_PARENTHESIS_ and the parenthesis adds a comma */ \
    __HAS_COMMA(__TRIGGER_PARENTHESIS_ __VA_ARGS__(/*empty*/))

/// Generates an enum with members:
/// count_ Number of enum entries
/// values_() Function returning an array of all values
/// toString() Member function returning the string representation of the value
///
/// Usage: Put ENUM_WITH_STRING(MyFancyEnum, Value1, Value2 = 42, Value3) in a header
/// In exactly 1 cpp file "#define DEFINE_MyFancyEnum" and include that header
#define ENUM_WITH_STRING(EnumName, ...) \
    BOOST_PP_IF(IS_EMPTY(DEFINE_##EnumName), DEFINE_ENUM_WITH_STRING, DECL_ENUM_WITH_STRING)(EnumName, __VA_ARGS__)

/// Declares the enum. Used when DEFINE_[EnumName] is not defined
/// Do not use directly. Use ENUM_WITH_STRING
#define DECL_ENUM_WITH_STRING(EnumName, ...)                                                   \
                                                                                               \
    struct EnumName                                                                            \
    {                                                                                          \
        enum type_                                                                             \
        {                                                                                      \
            __VA_ARGS__                                                                        \
        };                                                                                     \
                                                                                               \
        type_ value_;                                                                          \
                                                                                               \
        EnumName(type_ value) : value_(value) {}                                               \
        operator type_() const { return value_; }                                              \
                                                                                               \
        const char* toString() const;                                                          \
                                                                                               \
        static const size_t count_ = BOOST_PP_SEQ_SIZE(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)); \
                                                                                               \
        static const int* values_();                                                           \
        static const char* names_[count_];                                                     \
        static void init_();                                                                   \
        static void free_();                                                                   \
    };

/// Declares and defines the enum. Used when DEFINE_[EnumName] is defined
/// Do not use directly. Use ENUM_WITH_STRING
#define DEFINE_ENUM_WITH_STRING(EnumName, ...)                           \
    DECL_ENUM_WITH_STRING(EnumName, __VA_ARGS__)                         \
                                                                         \
    const char* EnumName::toString() const                               \
    {                                                                    \
        for(size_t index = 0; index < count_; ++index)                   \
        {                                                                \
            if(values_()[index] == value_)                               \
                return names_[index];                                    \
        }                                                                \
                                                                         \
        return nullptr;                                                  \
    }                                                                    \
                                                                         \
    const int* EnumName::values_()                                       \
    {                                                                    \
        static const int values[] = {IGNORE_ASSIGN(__VA_ARGS__)};        \
        return values;                                                   \
    }                                                                    \
                                                                         \
    const char* EnumName::names_[EnumName::count_];                      \
    void EnumName::init_()                                               \
    {                                                                    \
        static const char* const rawnames_[] = {STRINGIZE(__VA_ARGS__)}; \
                                                                         \
        for(size_t index = 0; index < count_; ++index)                   \
        {                                                                \
            size_t length = std::strcspn(rawnames_[index], " =\t\n\r");  \
                                                                         \
            char* name = new char[length + 1];                           \
            names_[index] = name;                                        \
                                                                         \
            std::strncpy(name, rawnames_[index], length);                \
            name[length] = '\0';                                         \
        }                                                                \
    }                                                                    \
    void EnumName::free_()                                               \
    {                                                                    \
        for(size_t index = 0; index < count_; ++index)                   \
        {                                                                \
            delete[] names_[index];                                      \
            names_[index] = nullptr;                                     \
        }                                                                \
    }                                                                    \
    namespace {                                                          \
        struct Init##EnumName                                            \
        {                                                                \
            Init##EnumName() { EnumName::init_(); }                      \
            ~Init##EnumName() { EnumName::free_(); }                     \
        } Initializer##EnumName##__;                                     \
    }

#endif // EnumWithString_h__
