// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "BasePlayerInfo.h"
#include "helpers/serializeEnums.h"
#include "s25util/Serializer.h"
#include "s25util/colors.h"

BasePlayerInfo::BasePlayerInfo()
    : ps(PlayerState::Free), nation(Nation::Romans), color(PLAYER_COLORS[0]), team(Team::None)
{}

BasePlayerInfo::BasePlayerInfo(Serializer& ser, bool lightData)
    : ps(helpers::popEnum<PlayerState>(ser)), aiInfo(!lightData || ps == PlayerState::AI ? ser : AI::Info())
{
    if(lightData && !isUsed())
    {
        nation = Nation::Romans;
        team = Team::None;
        color = PLAYER_COLORS[0];
    } else
    {
        name = ser.PopLongString();
        nation = helpers::popEnum<Nation>(ser);
        color = ser.PopUnsignedInt();
        // Temporary workaround: The random team was stored in the file but should not anymore, see PR #1331
        auto tmpTeam = ser.Pop<uint8_t>();
        if(tmpTeam > static_cast<uint8_t>(Team::Team4))
            tmpTeam -= 3; // Was random team 2-4
        else if(tmpTeam > helpers::MaxEnumValue_v<Team>)
            throw helpers::makeOutOfRange(tmpTeam, helpers::MaxEnumValue_v<Team>);
        team = Team(tmpTeam);
        if(team == Team::Random)
            team = Team::Team1; // Was random team 1
        // team = helpers::popEnum<Team>(ser);
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
