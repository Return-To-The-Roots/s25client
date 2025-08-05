// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/EnumArray.h"
#include <cstdint>

/// Types of pacts
enum class PactType : uint8_t
{
    TreatyOfAlliance,
    NonAgressionPact,
    OneSidedAlliance,
};
constexpr auto maxEnumValue(PactType)
{
    return PactType::OneSidedAlliance;
}

constexpr unsigned DURATION_INFINITE = 0xFFFFFFFF;

/// Names of the possible pacts
extern const helpers::EnumArray<const char*, PactType> PACT_NAMES;
