// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/variant2/variant.hpp>

/// Shortcurs to avoid typing out boost::variant2
template<typename... T>
using boost_variant2 = boost::variant2::variant<T...>;

using boost::variant2::get;
using boost::variant2::get_if;
using boost::variant2::holds_alternative;

namespace detail {
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

template<class... Fs>
auto composeVisitor(Fs&&... fs)
{
    return detail::lambda_visitor<std::decay_t<Fs>...>(std::forward<Fs>(fs)...);
}
