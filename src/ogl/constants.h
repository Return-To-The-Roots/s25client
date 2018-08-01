// Copyright (c) 2018 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef constants_h__
#define constants_h__

#include <boost/core/scoped_enum.hpp>
#include <boost/core/underlying_type.hpp>
#include <glad/glad.h>

namespace ogl {
// TODO: remove format-disable after C++11
// clang-format off
    BOOST_SCOPED_ENUM_UT_DECLARE_BEGIN(Target, GLenum){
        Array = GL_ARRAY_BUFFER,
        Index = GL_ELEMENT_ARRAY_BUFFER
    } BOOST_SCOPED_ENUM_DECLARE_END(Target)

    BOOST_SCOPED_ENUM_UT_DECLARE_BEGIN(Usage, GLenum){
        Static = GL_STATIC_DRAW,   // Very rare changes
        Dynamic = GL_DYNAMIC_DRAW, // Often changes
        Stream = GL_STREAM_DRAW    // Changes every frame
    } BOOST_SCOPED_ENUM_DECLARE_END(Usage)

    template<typename T_Enum>
    typename boost::underlying_type<T_Enum>::type enum_cast(T_Enum val)
    {
        return boost::underlying_cast<typename boost::underlying_type<T_Enum>::type>(val);
    }
// clang-format on
} // namespace ogl

#endif // constants_h__
