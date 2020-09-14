// Copyright (c) 2019 - 2019 Settlers Freaks (sf-team at siedler25.org)
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
