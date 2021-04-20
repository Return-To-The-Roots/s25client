// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>
class Serializer;

namespace AI {
enum class Level : uint8_t
{
    Easy,
    Medium,
    Hard
};
constexpr auto maxEnumValue(Level)
{
    return Level::Hard;
}

enum class Type : uint8_t
{
    Dummy,
    Default
};
constexpr auto maxEnumValue(Type)
{
    return Type::Default;
}

struct Info
{
    Type type;
    Level level;
    Info(Type t = Type::Dummy, Level l = Level::Easy) : type(t), level(l) {}
    Info(Serializer& ser);
    void serialize(Serializer& ser) const;
};
} // namespace AI
