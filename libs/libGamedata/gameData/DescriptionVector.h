// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DescIdx.h"
#include <boost/config.hpp>
#include <utility>
#include <vector>

template<typename T>
class IndexRange;

/// Adapter that provides type-safe access to elements by their description index
template<typename T_Element, typename T_Index>
class DescriptionVector
{
    std::vector<T_Element> storage;

public:
    using index_type = DescIdx<T_Index>;
    using index_range_type = IndexRange<T_Index>;

    DescriptionVector() = default;
    DescriptionVector(size_t count) : storage(count) {}

    auto size() const { return storage.size(); }
    auto begin() { return storage.begin(); }
    auto begin() const { return storage.begin(); }
    auto end() { return storage.end(); }
    auto end() const { return storage.end(); }
    T_Element& operator[](const index_type idx) { return storage[idx.value]; }
    const T_Element& operator[](const index_type idx) const { return storage[idx.value]; }

    void resize(const size_t new_size) { storage.resize(new_size); }
    void clear() { storage.clear(); }
    void push_back(const T_Element& value) { storage.push_back(value); }
    void push_back(T_Element&& value) { storage.push_back(std::move(value)); }

    /// Return an iterable yielding all indices in order
    index_range_type indices() const
    {
        RTTR_Assert(size() < index_type::INVALID);
        return index_range_type(index_type(size()));
    }
};

/// (Iterable) Range yielding DescIdx<T> instances
template<typename T>
class IndexRange
{
public:
    using index_type = DescIdx<T>;

    explicit constexpr IndexRange(index_type endValue) : startValue_(0), endValue_(endValue) {}

    class iterator
    {
        index_type value;

    public:
        explicit BOOST_FORCEINLINE constexpr iterator(index_type value) : value(value) {}
        BOOST_FORCEINLINE constexpr index_type operator*() const noexcept { return value; }
        BOOST_FORCEINLINE void operator++() noexcept { ++value.value; }
        BOOST_FORCEINLINE constexpr bool operator!=(iterator rhs) const noexcept { return value != rhs.value; }
    };

    BOOST_FORCEINLINE constexpr iterator begin() const noexcept { return iterator(startValue_); }
    BOOST_FORCEINLINE constexpr iterator end() const noexcept { return iterator(endValue_); }

private:
    const index_type startValue_;
    const index_type endValue_;
};