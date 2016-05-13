// Copyright (c) 2005 - 2016 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU pragma: keep
#include "BasePlayerInfo.h"
#include "libutil/src/colors.h"
#include "libutil/src/Serializer.h"

BasePlayerInfo::BasePlayerInfo():
    ps(PS_FREE),
    nation(NAT_ROMANS),
    color(PLAYER_COLORS[0]),
    team(TM_NOTEAM)
{}

BasePlayerInfo::BasePlayerInfo(Serializer& ser, bool lightData):
    ps(static_cast<PlayerState>(ser.PopUnsignedChar())),
    aiInfo(!lightData || ps == PS_KI ? ser : AI::Info())
{
    if(lightData && !isUsed())
    {
        nation = NAT_ROMANS;
        team = TM_NOTEAM;
        color = PLAYER_COLORS[0];
    }else
    {
        name = ser.PopString();
        nation = static_cast<Nation>(ser.PopUnsignedChar());
        color = ser.PopUnsignedInt();
        team = static_cast<Team>(ser.PopUnsignedChar());
    }
}

void BasePlayerInfo::Serialize(Serializer& ser, bool lightData) const
{
    ser.PushUnsignedChar(static_cast<unsigned char>(ps));
    if(lightData && !isUsed())
        return;
    if(!lightData || ps == PS_KI)
        aiInfo.serialize(ser);
    ser.PushString(name);
    ser.PushUnsignedChar(static_cast<unsigned char>(nation));
    ser.PushUnsignedInt(color);
    ser.PushUnsignedChar(static_cast<unsigned char>(team));
}
