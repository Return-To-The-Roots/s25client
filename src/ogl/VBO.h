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

#ifndef VBO_h__
#define VBO_h__

#include "ogl/BufferHandle.h"
#include "ogl/constants.h"
#include <glad/glad.h>
#include <stdexcept>

namespace ogl {
template<typename T>
class VBO
{
    BufferHandle handle_;
    Target target_;
    size_t size_;
    template<class T_Container>
    void ensureContiguous(const T_Container& data)
    {
        // TODO: Use data.data() for C++11
        if(&data.back() - &data.front() + 1 != static_cast<int64_t>(data.size()))
            throw std::runtime_error("Need contiguous container");
    }

public:
    VBO() : target_(Target::Array), size_(0) {}
    explicit VBO(Target target) { reset(target); }
    void reset(Target target)
    {
        // TODO: Remove this function in favor of move in C++11
        target_ = target;
        size_ = 0;
        handle_.create();
    }
    bool isValid() const { return handle_.isValid(); }
    /// Initialize the buffer with the given data. May pass NULL to only allocate memory
    void fill(const T* data, size_t numElems, Usage usageHint)
    {
        bind();
        size_ = numElems;
        glBufferData(enum_cast(target_), numElems * sizeof(T), data, enum_cast(usageHint));
    }
    /// Initialize the buffer with data from a container of contiguous memory.
    template<class T_Container>
    void fill(const T_Container& data, Usage usageHint)
    {
        ensureContiguous(data);
        fill(&data.front(), data.size(), usageHint);
    }
    /// Update data in the buffer. May pass an offset (in number of elements)
    void update(const T* data, size_t numElems, size_t offset = 0)
    {
        if(offset + numElems > size_)
            throw std::range_error("VBO is smaller than given size and offset");
        bind();
        glBufferSubData(enum_cast(target_), offset * sizeof(T), numElems * sizeof(T), data);
    }
    /// Update data in the buffer from a container of contiguous memory.
    /// May pass an offset (in number of elements)
    template<class T_Container>
    void update(const T_Container& data, size_t offset = 0)
    {
        ensureContiguous(data);
        update(&data.front(), data.size(), offset);
    }
    void bind() const
    {
        RTTR_Assert(isValid());
        glBindBuffer(enum_cast(target_), handle_.get());
    }
    void unbind() const { glBindBuffer(enum_cast(target_), 0u); }
};
} // namespace ogl

#endif // VBO_h__