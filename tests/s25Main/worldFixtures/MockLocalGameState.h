// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ILocalGameState.h"
#include <turtle/mock.hpp>

MOCK_BASE_CLASS(MockLocalGameState, ILocalGameState)
{
    MOCK_CONST_METHOD(GetPlayerId, 0);
    MOCK_CONST_METHOD(IsHost, 0);
    MOCK_CONST_METHOD(FormatGFTime, 1);
    MOCK_NON_CONST_METHOD(SystemChat, 1);
};
