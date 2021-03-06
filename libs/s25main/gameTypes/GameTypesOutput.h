// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "AIInfo.h"
#include "AnimalTypes.h"
#include "BuildingType.h"
#include "FlagType.h"
#include "GO_Type.h"
#include "GameSettingTypes.h"
#include "JobTypes.h"
#include "MapType.h"
#include "Nation.h"
#include "PactTypes.h"
#include "PlayerState.h"
#include "Resource.h"
#include "gameTypes/BuildingQuality.h"
#include "gameTypes/Direction.h"
#include "gameTypes/FoWNode.h"
#include "gameTypes/MapTypes.h"
#include "gameTypes/TeamTypes.h"
#include "gameData/DescIdx.h"
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <ostream>

// LCOV_EXCL_START

#define RTTR_ENUM_CASE_SINGLE(s, EnumName, Enumerator) \
    case EnumName::Enumerator: return os << #EnumName "::" BOOST_PP_STRINGIZE(Enumerator);

/// Generates an output operator for enum
///
/// Usage: ENUM_OUTPUT(MyFancyEnum, Value1, Value2, Value3, ...)
#define RTTR_ENUM_OUTPUT(EnumName, ...)                                                                   \
    inline std::ostream& operator<<(std::ostream& os, const EnumName& e)                                  \
    {                                                                                                     \
        switch(e)                                                                                         \
        {                                                                                                 \
            BOOST_PP_SEQ_FOR_EACH(RTTR_ENUM_CASE_SINGLE, EnumName, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) \
        }                                                                                                 \
        return os;                                                                                        \
    }

RTTR_ENUM_OUTPUT(BorderStonePos, OnPoint, HalfEast, HalfSouthEast, HalfSouthWest)
RTTR_ENUM_OUTPUT(BuildingQuality, Nothing, Flag, Hut, House, Castle, Mine, Harbor)
RTTR_ENUM_OUTPUT(Direction, West, NorthWest, NorthEast, East, SouthEast, SouthWest)
RTTR_ENUM_OUTPUT(Exploration, Disabled, Classic, FogOfWar, FogOfWarExplored)
RTTR_ENUM_OUTPUT(FlagType, Normal, Large, Water)
RTTR_ENUM_OUTPUT(GameObjective, None, Conquer3_4, TotalDomination, EconomyMode, Tournament1, Tournament2, Tournament3,
                 Tournament4, Tournament5)
RTTR_ENUM_OUTPUT(GameSpeed, VerySlow, Slow, Normal, Fast, VeryFast)
RTTR_ENUM_OUTPUT(MapType, OldMap, Savegame)
RTTR_ENUM_OUTPUT(PlayerState, Free, Occupied, Locked, AI)
RTTR_ENUM_OUTPUT(PointRoad, None, Normal, Donkey, Boat)
RTTR_ENUM_OUTPUT(PactType, TreatyOfAlliance, NonAgressionPact)
RTTR_ENUM_OUTPUT(ResourceType, Nothing, Iron, Gold, Coal, Granite, Water, Fish)
RTTR_ENUM_OUTPUT(RoadDir, East, SouthEast, SouthWest)
RTTR_ENUM_OUTPUT(Species, PolarBear, RabbitWhite, RabbitGrey, Fox, Stag, Deer, Duck, Sheep)
RTTR_ENUM_OUTPUT(StartWares, VLow, Low, Normal, ALot)
RTTR_ENUM_OUTPUT(Visibility, Invisible, FogOfWar, Visible)
RTTR_ENUM_OUTPUT(Team, None, Random, Team1, Team2, Team3, Team4, Random1To2, Random1To3, Random1To4)

namespace AI {
RTTR_ENUM_OUTPUT(Type, Dummy, Default)
RTTR_ENUM_OUTPUT(Level, Easy, Medium, Hard)
} // namespace AI

#undef RTTR_ENUM_CASE_SINGLE
#undef RTTR_ENUM_OUTPUT

// Simple only
#define RTTR_ENUM_OUTPUT(EnumName)                                                 \
    inline std::ostream& operator<<(std::ostream& out, const EnumName e)           \
    {                                                                              \
        return out << #EnumName "::" << static_cast<unsigned>(rttr::enum_cast(e)); \
    }

RTTR_ENUM_OUTPUT(BuildingType)
RTTR_ENUM_OUTPUT(Job)
RTTR_ENUM_OUTPUT(Nation)
RTTR_ENUM_OUTPUT(GO_Type)

#undef RTTR_ENUM_OUTPUT

template<class T>
std::ostream& operator<<(std::ostream& out, const DescIdx<T>& d)
{
    return out << d.value;
}

inline std::ostream& operator<<(std::ostream& out, const Resource r)
{
    return out << r.getType() << ":" << unsigned(r.getAmount());
}

// LCOV_EXCL_STOP
