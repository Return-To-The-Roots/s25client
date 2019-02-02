// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef rttrDefines_h__
#define rttrDefines_h__

// IWYU pragma: begin_exports

#include "commonDefines.h"
#include "macros.h"

// IWYU pragma: end_exports

/**
 *  konvertiert einen void*-Pointer zu einem function-Pointer mithilfe einer
 *  Union. GCC meckert da sonst wegen "type punned pointer" bzw
 *  "iso c++ forbids conversion".
 */
template<typename F>
inline F pto2ptf(void* o)
{
    union
    {
        F f;
        void* o;
    } U;
    U.o = o;

    return U.f;
}

template<typename T>
inline T min(T a, T b)
{
    return (a < b) ? a : b;
}

template<typename T>
inline T max(T a, T b)
{
    return (a < b) ? b : a;
}

#endif // rttrDefines_h__
