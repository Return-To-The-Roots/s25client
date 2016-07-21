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
#include "JoinPlayerInfo.h"
#include "libutil/src/Serializer.h"
#include "mygettext/src/mygettext.h"
#include <cstdio>

JoinPlayerInfo::JoinPlayerInfo():
    rating(0),
    isReady(false)
{}

JoinPlayerInfo::JoinPlayerInfo(const BasePlayerInfo& baseInfo) :
    PlayerInfo(baseInfo),
    originName(name),
    rating(0),
    isReady(false)
{}

JoinPlayerInfo::JoinPlayerInfo(const PlayerInfo& playerInfo):
    PlayerInfo(playerInfo),
    originName(name),
    rating(0),
    isReady(false)
{}

JoinPlayerInfo::JoinPlayerInfo(Serializer& ser):
    PlayerInfo(ser),
    originName(ser.PopString()),
    rating(ser.PopUnsignedInt()),
    isReady(ser.PopBool())
{}

void JoinPlayerInfo::Serialize(Serializer& ser) const
{
    PlayerInfo::Serialize(ser);
    ser.PushString(originName);
    ser.PushUnsignedInt(rating);
    ser.PushBool(isReady);
}

void JoinPlayerInfo::InitRating()
{
    if(ps == PS_OCCUPIED)
        rating = 1000;
    else if(ps == PS_AI)
    {
        if(aiInfo.type == AI::DEFAULT)
        {
            switch(aiInfo.level)
            {
            case AI::EASY:
                rating = 42;
                break;
            case AI::MEDIUM:
                rating = 666;
                break;
            case AI::HARD:
                rating = 1337;
                break;
            }
        } else
            rating = 0;
    } else
        rating = 0;
}

void JoinPlayerInfo::SetAIName(unsigned playerId)
{
    RTTR_Assert(ps == PS_AI);
    char str[128];
    if(aiInfo.type == AI::DUMMY)
        sprintf(str, _("Dummy %u"), playerId);
    else
        sprintf(str, _("Computer %u"), playerId);

    name = str;
    name += _(" (AI)");

    if(aiInfo.type == AI::DEFAULT)
    {
        switch(aiInfo.level)
        {
        case AI::EASY:
            name += _(" (easy)");
            break;
        case AI::MEDIUM:
            name += _(" (medium)");
            break;
        case AI::HARD:
            name += _(" (hard)");
            break;
        }
    }
}
