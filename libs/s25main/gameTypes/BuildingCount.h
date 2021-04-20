// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/EnumArray.h"
#include "gameTypes/BuildingType.h"

/// Number of buildings and building sites per type
struct BuildingCount
{
    helpers::EnumArray<unsigned, BuildingType> buildings;
    helpers::EnumArray<unsigned, BuildingType> buildingSites;
};
