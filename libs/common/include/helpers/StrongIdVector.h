// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "RTTR_Assert.h"
#include "StrongId.h"
#include <vector>

namespace helpers {

/// Vector of T-elements (std::vector<T>) indexed by StrongId TIndex
template<typename T, typename TIndex>
struct StrongIdVector;

template<typename T, typename TIndex_U, typename TIndex_T>
struct StrongIdVector<T, StrongId<TIndex_U, TIndex_T>> : std::vector<T>
{
    using Parent = std::vector<T>;
    using index_t = StrongId<TIndex_U, TIndex_T>;

    using Parent::Parent;

    typename Parent::reference operator[](index_t id)
    {
        RTTR_Assert(id && id.value() <= this->size());
        return Parent::operator[](id.value() - 1);
    }
    typename Parent::const_reference operator[](index_t id) const
    {
        RTTR_Assert(id && id.value() <= this->size());
        return Parent::operator[](id.value() - 1);
    }
};

} // namespace helpers
