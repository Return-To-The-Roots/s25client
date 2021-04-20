// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "exportImport.h"

RTTR_DECL unsigned GetDriverAPIVersion();
RTTR_DECL const char* GetDriverName();

using GetDriverAPIVersion_t = decltype(GetDriverAPIVersion);
using GetDriverName_t = decltype(GetDriverName);
