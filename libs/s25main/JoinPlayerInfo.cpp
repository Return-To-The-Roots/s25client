// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "JoinPlayerInfo.h"
#include "RTTR_Assert.h"
#include "mygettext/mygettext.h"
#include "s25util/Serializer.h"
#include <boost/format.hpp>

JoinPlayerInfo::JoinPlayerInfo() = default;

JoinPlayerInfo::JoinPlayerInfo(const BasePlayerInfo& baseInfo) : PlayerInfo(baseInfo), originName(name) {}

JoinPlayerInfo::JoinPlayerInfo(const PlayerInfo& playerInfo) : PlayerInfo(playerInfo), originName(name) {}

JoinPlayerInfo::JoinPlayerInfo(Serializer& ser)
    : PlayerInfo(ser), originName(ser.PopLongString()), isReady(ser.PopBool())
{}

void JoinPlayerInfo::Serialize(Serializer& ser) const
{
    PlayerInfo::Serialize(ser);
    ser.PushLongString(originName);
    ser.PushBool(isReady);
}

void JoinPlayerInfo::FixSwappedSaveSlot(JoinPlayerInfo& other)
{
    // TODO: This has a code smell.
    // Probably some composition instead of inheritance required?

    // Unswap fixed stuff
    using std::swap;
    swap(originName, other.originName);
    swap(nation, other.nation);
    swap(color, other.color);
    swap(team, other.team);
}

void JoinPlayerInfo::SetAIName(unsigned playerId)
{
    RTTR_Assert(ps == PS_AI);
    name = (boost::format((aiInfo.type == AI::DUMMY) ? _("Dummy %u") : _("Computer %u")) % playerId).str();
    name += _(" (AI)");

    if(aiInfo.type == AI::DEFAULT)
    {
        switch(aiInfo.level)
        {
            case AI::EASY: name += _(" (easy)"); break;
            case AI::MEDIUM: name += _(" (medium)"); break;
            case AI::HARD: name += _(" (hard)"); break;
        }
    }
}
