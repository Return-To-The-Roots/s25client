// Copyright (c) 2018 - 2018 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "RTTR_Assert.h"
#include "enum_cast.hpp"
#include "ogl/BufferHandle.h"
#include "ogl/constants.h"
#include <glad/glad.h>
#include <boost/type_traits/make_void.hpp>
#include <stdexcept>

namespace ogl {

template<class T>
using RequireContiguousMemory = boost::void_t<decltype(std::declval<T>().data())>;

template<typename T>
class VBO
{
    BufferHandle handle_;
    Target target_;
    size_t size_;

public:
    VBO() : target_(Target::Array), size_(0) {}
    explicit VBO(Target target)
    {
        target_ = target;
        size_ = 0;
        handle_.create();
    }
    VBO(VBO&&) noexcept = default;
    VBO& operator=(VBO&&) noexcept = default;
    VBO(const VBO&) = delete;
    VBO& operator=(const VBO&) = delete;

    bool isValid() const { return handle_.isValid(); }
    /// Initialize the buffer with the given data. May pass nullptr to only allocate memory
    void fill(const T* data, size_t numElems, Usage usageHint)
    {
        bind();
        size_ = numElems;
        glBufferData(rttr::enum_cast(target_), numElems * sizeof(T), data, rttr::enum_cast(usageHint));
    }
    /// Initialize the buffer with data from a container of contiguous memory.
    template<class T_Container, typename = RequireContiguousMemory<T_Container>>
    void fill(const T_Container& data, Usage usageHint)
    {
        fill(data.data(), data.size(), usageHint);
    }
    /// Update data in the buffer. May pass an offset (in number of elements)
    void update(const T* data, size_t numElems, size_t offset = 0)
    {
        if(offset + numElems > size_)
            throw std::range_error("VBO is smaller than given size and offset");
        bind();
        glBufferSubData(rttr::enum_cast(target_), offset * sizeof(T), numElems * sizeof(T), data);
    }
    /// Update data in the buffer from a container of contiguous memory.
    /// May pass an offset (in number of elements)
    template<class T_Container, typename = RequireContiguousMemory<T_Container>>
    void update(const T_Container& data, size_t offset = 0)
    {
        update(data.data(), data.size(), offset);
    }
    void bind() const
    {
        RTTR_Assert(isValid());
        glBindBuffer(rttr::enum_cast(target_), handle_.get());
    }
    void unbind() const { glBindBuffer(rttr::enum_cast(target_), 0u); }
};
} // namespace ogl
