// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <RTTR_Assert.h>
#include <cstdint>

class Serializer;
class GameWorldGame;
class GameCommandFactory;

namespace gc {

class GameCommand;
// Use this for safely using Pointers to GameCommands
using GameCommandPtr = boost::intrusive_ptr<GameCommand>;

class GameCommand
{
protected:
    enum Type
    {
        SET_FLAG,
        DESTROY_FLAG,
        BUILD_ROAD,
        DESTROY_ROAD,
        CHANGE_DISTRIBUTION,
        CHANGE_BUILDORDER,
        SET_BUILDINGSITE,
        DESTROY_BUILDING,
        CHANGE_TRANSPORT,
        CHANGE_MILITARY,
        CHANGE_TOOLS,
        CALL_SPECIALIST,
        CALL_SCOUT,
        ATTACK,
        SET_COINS_ALLOWED,
        SET_PRODUCTION_ENABLED,
        SET_INVENTORY_SETTING,
        SET_ALL_INVENTORY_SETTINGS,
        CHANGE_RESERVE,
        SUGGEST_PACT,
        ACCEPT_PACT,
        CANCEL_PACT,
        SET_SHIPYARD_MODE,
        START_STOP_EXPEDITION,
        EXPEDITION_COMMAND,
        SEA_ATTACK,
        START_STOP_EXPLORATION_EXPEDITION,
        TRADE,
        SURRENDER,
        CHEAT_ARMAGEDDON,
        DESTROY_ALL,
        UPGRADE_ROAD,
        SEND_SOLDIERS_HOME,
        ORDER_NEW_SOLDIERS,
        NOTIFY_ALLIES_OF_LOCATION
    };

private:
    /// Type of this command
    Type gcType;
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
    virtual void Execute(GameWorldGame& gwg, uint8_t playerId) = 0;

protected:
    GameCommand(const Type gcType) : gcType(gcType), refCounter_(0) {}
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
