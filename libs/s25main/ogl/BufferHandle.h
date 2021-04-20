// Copyright (c) 2018 - 2018 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
