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

#pragma once

#include <glad/glad.h>

namespace ogl {
enum class Target : GLenum
{
    Array = GL_ARRAY_BUFFER,
    Index = GL_ELEMENT_ARRAY_BUFFER
};

enum class Usage : GLenum
{
    Static = GL_STATIC_DRAW,   // Very rare changes
    Dynamic = GL_DYNAMIC_DRAW, // Often changes
    Stream = GL_STREAM_DRAW    // Changes every frame
};
} // namespace ogl
