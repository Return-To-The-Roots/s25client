// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#include "GamePlayerInfo.h"
#include "Serializer.h"
#include "libutil/src/colors.h"
#include "mygettext/src/mygettext.h"
#include <algorithm>

GamePlayerInfo::GamePlayerInfo(const unsigned playerid) :
    playerid(playerid),
    defeated(false),
    is_host(false),
    ping(0),
    rating(0),
    ready(false)
{}

GamePlayerInfo::GamePlayerInfo(const unsigned playerid, Serializer& ser) :
    playerid(playerid),
    defeated(false),
    origin_name(ser.PopString()),
    is_host(ser.PopBool()),
    ping(ser.PopUnsignedInt()),
    rating(ser.PopUnsignedInt()),
    ready(ser.PopBool())
{
    BasePlayerInfo::Deserialize(ser, false);
}

GamePlayerInfo::~GamePlayerInfo()
{
}

///////////////////////////////////////////////////////////////////////////////
// Rausschmeisser
void GamePlayerInfo::clear()
{
    name.clear();
    defeated = false;
    ps = PS_FREE;
    /*nation = team = color = 0;*/
    ping = rating  = 0;
    ready = false;
}

/// serialisiert die Daten.
void GamePlayerInfo::serialize(Serializer& ser) const
{
    ser.PushString(origin_name);
    ser.PushBool(is_host);
    ser.PushUnsignedInt(ping);
    ser.PushUnsignedInt(rating);
    ser.PushBool(ready);
    BasePlayerInfo::Serialize(ser, false);
}

void GamePlayerInfo::SwapInfo(GamePlayerInfo& two)
{
    using std::swap;
    swap(defeated, two.defeated);
    swap(ps, two.ps);
    swap(aiInfo, two.aiInfo);
    swap(name, two.name);
    swap(is_host, two.is_host);
    swap(ping, two.ping);
    swap(rating, two.rating);
    swap(ready, two.ready);
}

int GamePlayerInfo::GetColorIdx() const
{
    return GetColorIdx(color);
}

int GamePlayerInfo::GetColorIdx(unsigned color)
{
    for(int i = 0; i < static_cast<int>(PLAYER_COLORS.size()); ++i)
    {
        if(PLAYER_COLORS[i] == color)
            return i;
    }
    return -1;
}

void GamePlayerInfo::InitRating()
{
    if(ps == PS_OCCUPIED)
        rating = 1000;
    else if(ps == PS_KI)
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

void GamePlayerInfo::SetAIName(unsigned playerId)
{
    RTTR_Assert(ps == PS_KI);
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
