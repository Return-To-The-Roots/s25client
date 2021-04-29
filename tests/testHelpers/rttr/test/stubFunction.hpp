// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/preprocessor/cat.hpp>

namespace rttr {

template<typename T>
struct StubFunctionReset
{
    T** funcRef_;
    T* origFunc_;
    StubFunctionReset(T*& function, T* newFunction) : funcRef_(&function), origFunc_(function)
    {
        function = newFunction;
    }
    ~StubFunctionReset()
    {
        if(funcRef_)
            *funcRef_ = origFunc_;
    }
    StubFunctionReset(const StubFunctionReset&) = delete;
    StubFunctionReset(StubFunctionReset&& rhs) noexcept : funcRef_(rhs.funcRef_), origFunc_(rhs.origFunc_)
    {
        rhs.funcRef_ = nullptr;
    }
};

#define RTTR_STUB_FUNCTION(function, newFunction) \
    rttr::StubFunctionReset<decltype(newFunction)> BOOST_PP_CAT(_stub_, __COUNTER__)(function, newFunction)

} // namespace rttr
