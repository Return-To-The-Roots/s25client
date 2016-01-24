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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h"
#include "GamePlayerInfo.h"

#include "drivers/VideoDriverWrapper.h"
#include "GameMessage.h"
#include <algorithm>

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
// Konstruktor
GamePlayerInfo::GamePlayerInfo(const unsigned playerid) :
    playerid(playerid),
    defeated(false),
    ps(PS_FREE),
    aiInfo(),
    is_host(false),
    nation(NAT_ROMANS),
    team(TM_NOTEAM),
    color(0),
    ping(0),
    rating(0),
    ready(false)
{
}

/// Deserialisierungskonstruktor
GamePlayerInfo::GamePlayerInfo(const unsigned playerid, Serializer& ser) :
    playerid(playerid),
    ps(PlayerState(ser.PopUnsignedChar())),
    aiInfo(),
    name(ser.PopString()),
    origin_name(ser.PopString()),
    is_host(ser.PopBool()),
    nation(Nation(ser.PopUnsignedChar())),
    team(Team(ser.PopUnsignedChar())),
    color(ser.PopUnsignedChar()),
    ping(ser.PopUnsignedInt()),
    rating(ser.PopUnsignedInt()),
    ready(ser.PopBool())
{
}

///////////////////////////////////////////////////////////////////////////////
// Destruktor
GamePlayerInfo::~GamePlayerInfo(void)
{
}

///////////////////////////////////////////////////////////////////////////////
// Rausschmeisser
void GamePlayerInfo::clear(void)
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
    ser.PushUnsignedChar(static_cast<unsigned char>(ps));
    ser.PushString(name);
    ser.PushString(origin_name);
    ser.PushBool(is_host);
    ser.PushUnsignedChar(static_cast<unsigned char>(nation));
    ser.PushUnsignedChar(team);
    ser.PushUnsignedChar(color);
    ser.PushUnsignedInt(ping);
    ser.PushUnsignedInt(rating);
    ser.PushBool(ready);
}

void GamePlayerInfo::SwapInfo(GamePlayerInfo& two)
{
    using std::swap;
    swap(ps, two.ps);
    swap(aiInfo, two.aiInfo);
    swap(defeated, two.defeated);
    swap(name, two.name);
    swap(is_host, two.is_host);
    swap(ping, two.ping);
    swap(rating, two.rating);
    swap(ready, two.ready);
}

