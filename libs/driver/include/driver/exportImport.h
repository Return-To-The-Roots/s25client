// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/config.hpp>

#ifdef BUILD_DLL
#    define RTTR_DECL extern "C" BOOST_SYMBOL_EXPORT
#else
#    define RTTR_DECL extern "C" BOOST_SYMBOL_IMPORT
#endif
