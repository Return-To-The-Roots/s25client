// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <RTTR_Assert.h>
#include <cstdint>

class Serializer;
class GameWorld;
class GameCommandFactory;

namespace gc {

class GameCommand;
// Use this for safely using Pointers to GameCommands
using GameCommandPtr = boost::intrusive_ptr<GameCommand>;

enum class GCType : uint8_t
{
    SetFlag,
    DestroyFlag,
    BuildRoad,
    DestroyRoad,
    ChangeDistribution,
    ChangeBuildOrder,
    SetBuildingsite,
    DestroyBuilding,
    ChangeTransport,
    ChangeMilitary,
    ChangeTools,
    CallSpecialist,
    CallScout,
    Attack,
    SetCoinsAllowed,
    SetProductionEnabled,
    SetInventorySetting,
    SetAllInventorySettings,
    ChangeReserve,
    SuggestPact,
    AcceptPact,
    CancelPact,
    SetShipyardMode,
    StartStopExpedition,
    ExpeditionCommand,
    SeaAttack,
    StartStopExplorationExpedition,
    Trade,
    Surrender,
    CheatArmageddon,
    DestroyAll,
    UpgradeRoad,
    SetDesiredTroops,
    NotifyAlliesOfLocation
};
constexpr auto maxEnumValue(GCType)
{
    return GCType::NotifyAlliesOfLocation;
}

class GameCommand
{
private:
    /// Type of this command
    GCType gcType;
    unsigned refCounter_;
    friend void intrusive_ptr_add_ref(GameCommand* x);
    friend void intrusive_ptr_release(GameCommand* x);

public:
    GameCommand(const GameCommand& obj) : gcType(obj.gcType), refCounter_(0) // Do not copy refCounter!
    {}
    virtual ~GameCommand() = default;

    GameCommand& operator=(const GameCommand& obj)
    {
        gcType = obj.gcType;
        // Do not copy or reset refCounter!
        return *this;
    }

    /// Builds a GameCommand depending on Type
    static GameCommandPtr Deserialize(Serializer& ser);

    /// Serializes this GameCommand
    virtual void Serialize(Serializer& ser) const;

    /// Execute this GameCommand
    virtual void Execute(GameWorld& world, uint8_t playerId) = 0;

protected:
    GameCommand(const GCType gcType) : gcType(gcType), refCounter_(0) {}
};

inline void intrusive_ptr_add_ref(GameCommand* x)
{
    ++x->refCounter_;
}

inline void intrusive_ptr_release(GameCommand* x)
{
    RTTR_Assert(x->refCounter_);
    if(--x->refCounter_ == 0)
        delete x;
}

} // namespace gc

// Macro used by all derived GameCommands to allow specified class access to non-public members (e.g. constructor)
// Only factory classes should be in here
#define GC_FRIEND_DECL        \
    friend class GameCommand; \
    friend class ::GameCommandFactory
