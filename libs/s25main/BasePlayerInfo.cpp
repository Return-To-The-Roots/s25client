// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "BasePlayerInfo.h"
#include "helpers/serializeEnums.h"
#include "s25util/VersionedDeserializer.h"
#include "s25util/colors.h"

BasePlayerInfo::BasePlayerInfo()
    : ps(PlayerState::Free), portraitIndex(0), nation(Nation::Romans), color(PLAYER_COLORS[0]), team(Team::None)
{}

BasePlayerInfo::BasePlayerInfo(Serializer& ser, int version, bool lightData)
    : ps(helpers::popEnum<PlayerState>(ser)), aiInfo(!lightData || ps == PlayerState::AI ? ser : AI::Info())
{
    auto versionedDeserializer = s25util::VersionedDeserializer<BasePlayerInfo>(ser, version);

    if(lightData && !isUsed())
    {
        portraitIndex = 0;
        nation = Nation::Romans;
        team = Team::None;
        color = PLAYER_COLORS[0];
    } else
    {
        name = ser.PopLongString();
        portraitIndex = (version >= 1) ? ser.PopUnsignedInt() : 0;
        nation = helpers::popEnum<Nation>(ser);
        color = ser.PopUnsignedInt();
        team = helpers::popEnum<Team>(ser);
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
