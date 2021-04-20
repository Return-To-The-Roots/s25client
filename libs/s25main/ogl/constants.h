// Copyright (C) 2018 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
