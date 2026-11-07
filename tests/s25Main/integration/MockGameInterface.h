// Copyright (C) 2024-2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include "Cheats.h"
#include "GameInterface.h"
#include <turtle/mock.hpp>

MOCK_BASE_CLASS(MockGameInterface, GameInterface)
{
    // LCOV_EXCL_START
    MOCK_METHOD(GI_PlayerDefeated, 1)
    MOCK_METHOD(GI_UpdateMinimap, 1)
    MOCK_METHOD(GI_FlagDestroyed, 1)
    MOCK_METHOD(GI_TreatyOfAllianceChanged, 1)
    MOCK_METHOD(GI_UpdateMapVisibility, 0)
    MOCK_METHOD(GI_Winner, 1)
    MOCK_METHOD(GI_TeamWinner, 1)
    MOCK_METHOD(GI_StartRoadBuilding, 2)
    MOCK_METHOD(GI_CancelRoadBuilding, 0)
    MOCK_METHOD(GI_BuildRoad, 0)
    // clang-format off
    MOCK_METHOD(GI_GetCheats, 0, Cheats&(void))
    // clang-format on
    // LCOV_EXCL_STOP
};