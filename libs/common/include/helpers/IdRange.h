// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "RTTR_Assert.h"
#include "StrongId.h"
#include <iterator>
#include <limits>

namespace helpers {
/// Iterate over all valid strong Ids of a given container size
template<typename T>
class idRange;

template<typename T, typename U>
class idRange<StrongId<T, U>>
{
    using id_t = StrongId<T, U>;
    using underlying_t = typename id_t::underlying_t;
    const underlying_t startValue_;
    const underlying_t endValue_;

    explicit idRange(underlying_t startValue, underlying_t count) : startValue_(startValue), endValue_(count + 1)
    {
        RTTR_Assert(count < std::numeric_limits<underlying_t>::max());
        RTTR_Assert(startValue_ <= endValue_);
    }

public:
    /// Create range for container with given number of elements
    explicit idRange(underlying_t count) : idRange(1, count) {}

    class iterator
    {
    protected:
        underlying_t value;

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = id_t;
        using difference_type = std::make_signed_t<std::common_type_t<underlying_t, int>>;
        using pointer = id_t*;
        using reference = id_t&;

        explicit constexpr iterator(underlying_t value) : value(value) {}
        constexpr id_t operator*() const noexcept { return id_t(value); }
        void operator++() noexcept { ++value; }
        constexpr bool operator!=(iterator rhs) const noexcept { return value != rhs.value; }
    };

    iterator begin() const noexcept { return iterator(startValue_); }
    iterator end() const noexcept { return iterator(endValue_); }

    template<typename T_Id, typename U2>
    friend constexpr idRange<T_Id> idRangeAfter(T_Id prev, U2 totalCount);
};

/// Return range starting after the given value
/// totalCount is the count of the full range not including the `prev` element
template<typename T, typename U>
constexpr idRange<T> idRangeAfter(T prev, U totalCount)
{
    return idRange<T>(prev.next().value(), totalCount);
}

} // namespace helpers
