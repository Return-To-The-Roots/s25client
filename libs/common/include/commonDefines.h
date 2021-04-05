// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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
