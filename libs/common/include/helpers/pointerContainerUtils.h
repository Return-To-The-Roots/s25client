// Copyright (C) 2021 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "helpers/containerUtils.h"
#include <memory>
#include <type_traits>

namespace helpers {

/// Find the given pointer in a container of unique_ptrs
template<typename T, typename U>
auto findPtr(T& container, const U* value)
{
    using SmartPointer = typename std::remove_const_t<T>::value_type;
    return find_if(container, [value](const SmartPointer& ptr) { return ptr.get() == value; });
}

/// Extract the given pointer from a container of unique_ptrs
template<typename T, typename U>
auto extractPtr(T& container, const U* value)
{
    const auto itPtr = findPtr(container, value);
    RTTR_Assert(itPtr != container.end());
    std::unique_ptr<U> result = std::move(*itPtr);
    container.erase(itPtr);
    return result;
}

/// Return true iff the container contains a unique_ptr with the given value
template<typename T, typename U>
bool containsPtr(const T& container, const U* value)
{
    return findPtr(container, value) != container.end();
}

} // namespace helpers
