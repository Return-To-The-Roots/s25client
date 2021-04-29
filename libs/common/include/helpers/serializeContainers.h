// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "serializeEnums.h"
#include "s25util/Serializer.h"
#include <type_traits>

namespace helpers {

namespace detail {
    template<typename T, typename = void>
    struct isResizableContainer : std::false_type
    {};
    template<typename T>
    struct isResizableContainer<T, decltype(std::declval<T>().resize(size_t{}))> : std::true_type
    {};

    template<typename T>
    std::enable_if_t<std::is_enum<T>::value, T> popEnumOrIntegral(Serializer& ser)
    {
        return popEnum<T>(ser);
    }
    template<typename T>
    std::enable_if_t<!std::is_enum<T>::value, T> popEnumOrIntegral(Serializer& ser)
    {
        return ser.Pop<T>();
    }

    template<typename T>
    void pushContainer(Serializer& ser, const T& container, long)
    {
        using Type = typename T::value_type;
        using Integral = typename std::conditional_t<std::is_enum<Type>::value, std::underlying_type<Type>,
                                                     std::common_type<Type>>::type;
        for(const auto el : container)
        {
            // Cast also required for bool vector -.-
            ser.Push(static_cast<Integral>(el));
        }
    }
    template<typename T>
    auto pushContainer(Serializer& ser, const T& container, int) -> std::enable_if_t<sizeof(*container.data()) == 1u>
    {
        ser.PushRawData(container.data(), container.size());
    }

    // If the container is resizable, pop the size and resize it
    template<typename T>
    std::enable_if_t<isResizableContainer<T>::value> maybePopSizeAndResize(Serializer& ser, T& container)
    {
        container.resize(ser.PopVarSize());
    }
    template<typename T>
    std::enable_if_t<!isResizableContainer<T>::value> maybePopSizeAndResize(Serializer&, T&)
    {}

    template<typename T>
    void popContainer(Serializer& ser, T&& container, long)
    {
        using Type = typename std::remove_reference_t<T>::value_type;
        for(auto&& el : container) // auto&& required for vector<bool> proxy object
            el = popEnumOrIntegral<Type>(ser);
    }
    template<typename T>
    auto popContainer(Serializer& ser, T& container, int)
      -> std::enable_if_t<sizeof(*container.data()) == 1u && !std::is_enum<typename T::value_type>::value>
    {
        ser.PopRawData(container.data(), container.size());
    }

} // namespace detail

template<typename T>
void pushContainer(Serializer& ser, const T& container, bool ignoreSize = false)
{
    using Type = typename T::value_type;
    static_assert(std::is_integral<Type>::value || std::is_enum<Type>::value,
                  "Only integral types and enums are possible");

    if(detail::isResizableContainer<T>::value && !ignoreSize)
        ser.PushVarSize(container.size());
    detail::pushContainer(ser, container, int());
}

template<typename T>
void popContainer(Serializer& ser, T&& result, bool ignoreSize = false)
{
    using Type = typename std::remove_reference_t<T>::value_type;
    static_assert(std::is_integral<Type>::value || std::is_enum<Type>::value,
                  "Only integral types and enums are possible");

    if(!ignoreSize)
        detail::maybePopSizeAndResize(ser, result);
    detail::popContainer(ser, result, int());
}
template<typename T>
T popContainer(Serializer& ser)
{
    T result;
    popContainer(ser, result);
    return result;
}

} // namespace helpers
