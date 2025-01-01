// Copyright (C) 2005 - 2025 Settlers Freaks <sf-team at siedler25.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// Use (Windows) dbghelp API for backtrace
#cmakedefine01 RTTR_BACKTRACE_HAS_DBGHELP
// Use `backtrace` function
#cmakedefine01 RTTR_BACKTRACE_HAS_FUNCTION

#if RTTR_BACKTRACE_HAS_FUNCTION
#   include <@Backtrace_HEADER@>
#endif
