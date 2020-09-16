// Copyright (c) 2018 - 2019 Settlers Freaks (sf-team at siedler25.org)
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
