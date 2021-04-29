// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/variant.hpp>
#include <type_traits>

namespace detail {
template<typename...>
struct indexOf;
template<typename T, typename... Rest>
struct indexOf<T, T, Rest...> : std::integral_constant<size_t, 0>
{};
template<typename T, typename U, typename... Rest>
struct indexOf<T, U, Rest...> : std::integral_constant<size_t, 1 + indexOf<T, Rest...>::value>
{};

template<typename... Lambdas>
struct lambda_visitor;

template<typename Lambda1, typename... Lambdas>
struct lambda_visitor<Lambda1, Lambdas...> : public lambda_visitor<Lambdas...>, public Lambda1
{
    using Lambda1::operator();
    using lambda_visitor<Lambdas...>::operator();
    lambda_visitor(Lambda1 l1, Lambdas... lambdas) : lambda_visitor<Lambdas...>(lambdas...), Lambda1(std::move(l1)) {}
};

template<typename Lambda1>
struct lambda_visitor<Lambda1> : public Lambda1
{
    using Lambda1::operator();
    lambda_visitor(Lambda1 l1) : Lambda1(l1) {}
};
} // namespace detail

template<class T, class... Types>
bool holds_alternative(const boost::variant<Types...>& v) noexcept
{
    return v.which() == detail::indexOf<T, Types...>::value;
}

template<class... Fs>
auto composeVisitor(Fs&&... fs)
{
    return detail::lambda_visitor<std::decay_t<Fs>...>(std::forward<Fs>(fs)...);
}
