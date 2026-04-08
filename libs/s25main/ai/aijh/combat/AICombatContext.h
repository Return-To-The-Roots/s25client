// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/AIInfo.h"
#include "gameTypes/MapCoordinates.h"

class AIInterface;
class GameWorldBase;
class nobBaseMilitary;
struct AIConfig;

namespace AIJH {

enum class AICombatTargetSelectionMode
{
    Random,
    Prudent,
    Biting,
    Attrition
};

class AICombatContext
{
public:
    virtual ~AICombatContext() = default;

    virtual AIInterface& GetInterface() = 0;
    virtual const AIInterface& GetInterface() const = 0;
    virtual const AIConfig& GetConfig() const = 0;
    virtual const GameWorldBase& GetWorld() const = 0;
    virtual unsigned char GetPlayerId() const = 0;
    virtual AI::Level GetLevel() const = 0;

    virtual bool IsRecentlyLostMilitaryBuilding(MapPoint pt) const = 0;
    virtual void TrackCombatStart(const nobBaseMilitary& target) = 0;
};

} // namespace AIJH
