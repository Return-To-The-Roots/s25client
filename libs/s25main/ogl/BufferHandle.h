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
#include <utility>

namespace ogl {
/// RAII Handle of an OpenGL buffer
class BufferHandle
{
    GLuint handle_;

public:
    explicit BufferHandle(GLuint handle = 0u) : handle_(handle) {}
    BufferHandle(BufferHandle&& other) noexcept : handle_(other.handle_) { other.handle_ = 0u; }
    BufferHandle(const BufferHandle&) = delete;
    BufferHandle& operator=(const BufferHandle& other) = delete;
    ~BufferHandle() { reset(); }
    BufferHandle& operator=(BufferHandle&& other) noexcept
    {
        std::swap(handle_, other.handle_);
        return *this;
    }
    void reset(GLuint handle = 0u)
    {
        if(handle_)
            glDeleteBuffers(1, &handle_);
        handle_ = handle;
    }
    GLuint get() const { return handle_; }
    bool isValid() const { return get() != 0u; }
    void create()
    {
        GLuint handle;
        glGenBuffers(1, &handle);
        reset(handle);
    }
};
} // namespace ogl
