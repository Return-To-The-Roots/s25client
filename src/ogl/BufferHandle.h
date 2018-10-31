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

#ifndef BufferHandle_h__
#define BufferHandle_h__

#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <glad/glad.h>

namespace ogl {
/// RAII Handle of an OpenGL buffer
class BufferHandle
{
    struct BufferDeleter
    {
        typedef GLuint pointer;
        void operator()(pointer buffer) const
        {
            if(buffer)
                glDeleteBuffers(1, &buffer);
        }
    };

    boost::interprocess::unique_ptr<GLuint, BufferDeleter> handle_;

public:
    explicit BufferHandle(GLuint handle = 0u) : handle_(handle) {}
    void reset(GLuint handle = 0u) { handle_.reset(handle); }
    GLuint get() const { return handle_.get(); }
    bool isValid() const { return get() != 0u; }
    void create()
    {
        GLuint handle;
        glGenBuffers(1, &handle);
        reset(handle);
    }
};
} // namespace ogl

#endif // BufferHandle_h__
