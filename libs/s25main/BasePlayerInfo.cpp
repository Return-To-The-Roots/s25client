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
