// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef MACROS_H_INCLUDED
#define MACROS_H_INCLUDED

// Macro that can be used to suppress unused warnings. Required e.g. for const boost::arrays defined in headers
// Don't use this if not absolutely necessary!
#ifdef __GNUC__
#define SUPPRESS_UNUSED __attribute__((unused))
#else
#define SUPPRESS_UNUSED
#endif

// RTTR_FUNCTION_NAME evaluates to the name of the current function if supported
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 supports __func__ as a standard. */
#define RTTR_FUNCTION_NAME __func__
#elif((__GNUC__ >= 2) || defined(_MSC_VER))
#define RTTR_FUNCTION_NAME __FUNCTION__
#else
#define RTTR_FUNCTION_NAME "<Unknown Func>"
#endif

#if defined _WIN32 && defined _DEBUG && defined _MSC_VER && !defined NOCRTDBG
// Check for heap corruption
#define CHECK_HEAP_CORRUPTION _ASSERTE(_CrtCheckMemory());
#else
#define CHECK_HEAP_CORRUPTION
#endif // _WIN32 && _DEBUG && !NOCRTDBG

/// Call a member function trough an object and a member function pointer
#define CALL_MEMBER_FN(object, ptrToMember) ((object).*(ptrToMember))

/// Iterate over all points of an area using a point of TYPE named "pt"
/// WIDTH and HEIGHT is evaluated at most once
/// Use like:
///     RTTR_FOREACH_PT(Point<int>, world.GetSize()) {
///         std::cout << pt.x << "/" << pt.y;
///     }
#define RTTR_FOREACH_PT(TYPE, SIZE)                                           \
    /* Create scoped temporaries holding width and height by */               \
    /* using assignment in if to save potential accesses */                   \
    if(TYPE::ElementType rttrForeachPtWidth = TYPE::ElementType(SIZE.x))      \
        if(TYPE::ElementType rttrForeachPtHeight = TYPE::ElementType(SIZE.y)) \
            for(TYPE pt(0, 0); pt.y < rttrForeachPtHeight; ++pt.y)            \
                for(pt.x = 0; pt.x < rttrForeachPtWidth; ++pt.x)

#endif // !MACROS_H_INCLUDED
