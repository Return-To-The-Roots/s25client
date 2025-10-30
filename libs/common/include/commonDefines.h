// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "RTTR_Assert.h"

/// Call a member function trough an object and a member function pointer
#define CALL_MEMBER_FN(object, ptrToMember) ((object).*(ptrToMember))

/// Deletes the ptr and sets it to nullptr
template<typename T>
inline void deletePtr(T*& ptr)
{
    delete ptr;
    ptr = nullptr;
}

/// Calculate |a-b| of 2 unsigned values
template<typename T>
inline T absDiff(T a, T b)
{
    return (a > b) ? a - b : b - a;
}

/// Same as static_cast<T> but assert that it actually can be casted via dynamic_cast
template<typename T, typename T_Src>
inline T checkedCast(T_Src* src)
{
    RTTR_Assert(!src || dynamic_cast<T>(src));
    return static_cast<T>(src);
}

template<typename T, typename T_Src>
inline T& checkedCast(T_Src& src)
{
    RTTR_Assert(dynamic_cast<T*>(&src));
    return static_cast<T&>(src);
}

// Fwd decl
namespace boost {
namespace filesystem {
}
namespace nowide {
}
} // namespace boost

/// Shortcut for boost::filesystem
namespace bfs = boost::filesystem;
/// Shortcut for boost::nowide
namespace bnw = boost::nowide;
