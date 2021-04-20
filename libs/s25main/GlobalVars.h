// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "s25util/Singleton.h"

/// Klasse für alle "globalen" Variablen/Objekte
class GlobalVars : public Singleton<GlobalVars>
{
public:
    bool notdone = true;
};

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#define GLOBALVARS GlobalVars::inst()
