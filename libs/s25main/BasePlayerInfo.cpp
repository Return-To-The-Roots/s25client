// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "BasePlayerInfo.h"
#include "enum_cast.hpp"
#include "helpers/serializeEnums.h"
#include "gameData/PortraitConsts.h"
#include "s25util/colors.h"

BasePlayerInfo::BasePlayerInfo()
    : ps(PlayerState::Free), portraitIndex(0), nation(Nation::Romans), color(PLAYER_COLORS[0]), team(Team::None)
{}

BasePlayerInfo::BasePlayerInfo(Serializer& ser, int serializedVersion, bool lightData)
    : ps(helpers::popEnum<PlayerState>(ser)), aiInfo(!lightData || ps == PlayerState::AI ? ser : AI::Info())
{
    if(lightData && !isUsed())
    {
        portraitIndex = 0;
        nation = Nation::Romans;
        team = Team::None;
        color = PLAYER_COLORS[0];
    } else
    {
        name = ser.PopLongString();
        portraitIndex = (serializedVersion >= 1) ? ser.PopUnsignedInt() : 0;
        if(portraitIndex >= Portraits.size())
        {
            portraitIndex = 0;
        }
        nation = helpers::popEnum<Nation>(ser);
        color = ser.PopUnsignedInt();
        team = helpers::popEnum<Team>(ser);
        if(serializedVersion >= 2)
            startWares = popStartWares(ser);
    }
}

void BasePlayerInfo::Serialize(Serializer& ser, bool lightData) const
{
    helpers::pushEnum<uint8_t>(ser, ps);
    if(lightData && !isUsed())
        return;
    if(!lightData || ps == PlayerState::AI)
        aiInfo.serialize(ser);
    ser.PushLongString(name);
    ser.PushUnsignedInt(portraitIndex);
    helpers::pushEnum<uint8_t>(ser, nation);
    ser.PushUnsignedInt(color);
    helpers::pushEnum<uint8_t>(ser, team);
    pushStartWares(ser, startWares);
}

void BasePlayerInfo::pushStartWares(Serializer& ser, const std::optional<StartWares>& startWares)
{
    // 0 = global setting, otherwise the value + 1
    ser.PushUnsignedChar(startWares ? static_cast<uint8_t>(rttr::enum_cast(*startWares) + 1) : uint8_t(0));
}

std::optional<StartWares> BasePlayerInfo::popStartWares(Serializer& ser)
{
    const unsigned value = ser.PopUnsignedChar();
    if(value == 0)
        return std::nullopt;
    if(value > helpers::MaxEnumValue_v<StartWares> + 1u)
        throw helpers::makeOutOfRange(value, helpers::MaxEnumValue_v<StartWares> + 1u);
    return static_cast<StartWares>(value - 1);
}

int BasePlayerInfo::GetColorIdx() const
{
    return GetColorIdx(color);
}

int BasePlayerInfo::GetColorIdx(unsigned color) //-V688
{
    for(int i = 0; i < static_cast<int>(PLAYER_COLORS.size()); ++i)
    {
        if(PLAYER_COLORS[i] == color)
            return i;
    }
    return -1;
}

int BasePlayerInfo::getCurrentVersion()
{
    // 0: Initial
    // 1: Added portraitIndex
    // 2: Added startWares
    return 2;
}
