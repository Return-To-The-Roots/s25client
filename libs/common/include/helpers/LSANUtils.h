// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#ifdef __has_feature
#    define RTTR_HAS_ASAN __has_feature(address_sanitizer)
#elif defined(__SANITIZE_ADDRESS__)
#    define RTTR_HAS_ASAN __SANITIZE_ADDRESS__
#else
#    define RTTR_HAS_ASAN 0
#endif

#if RTTR_HAS_ASAN
#    include <sanitizer/lsan_interface.h>
namespace rttr {
/// Record all leaks in the current context as expected
using ScopedLeakDisabler = __lsan::ScopedDisabler;
} // namespace rttr
#else
namespace rttr {
struct ScopedLeakDisabler
{
    static void suppressDefaultCtorWarning() {}
    ScopedLeakDisabler()
    { // Pretent to be a RAII class to avoid unused variable warnings
        suppressDefaultCtorWarning();
    };
};
} // namespace rttr
#endif
