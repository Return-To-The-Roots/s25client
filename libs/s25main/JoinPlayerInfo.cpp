// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "JoinPlayerInfo.h"
#include "RTTR_Assert.h"
#include "mygettext/mygettext.h"
#include "s25util/Serializer.h"
#include <boost/format.hpp>

JoinPlayerInfo::JoinPlayerInfo() = default;

JoinPlayerInfo::JoinPlayerInfo(const BasePlayerInfo& baseInfo) : PlayerInfo(baseInfo), originName(name) {}

JoinPlayerInfo::JoinPlayerInfo(const PlayerInfo& playerInfo) : PlayerInfo(playerInfo), originName(name) {}

JoinPlayerInfo::JoinPlayerInfo(Serializer& ser)
    : PlayerInfo(ser, BasePlayerInfo::getCurrentVersion()), originName(ser.popLongString()), isReady(ser.popBool())
{}

void JoinPlayerInfo::serialize(Serializer& ser) const
{
    PlayerInfo::serialize(ser);
    ser.pushLongString(originName);
    ser.pushBool(isReady);
}

void JoinPlayerInfo::fixSwappedSaveSlot(JoinPlayerInfo& other)
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

void JoinPlayerInfo::setAIName(unsigned playerId)
{
    RTTR_Assert(ps == PlayerState::AI);
    name = makeAIName(aiInfo, playerId);
}

std::string JoinPlayerInfo::makeAIName(const AI::Info& aiInfo, unsigned playerId)
{
    std::string name =
      (boost::format((aiInfo.type == AI::Type::Dummy) ? _("Dummy %u") : _("Computer %u")) % playerId).str();
    name += _(" (AI)");

    if(aiInfo.type == AI::Type::Default)
    {
        switch(aiInfo.level)
        {
            case AI::Level::Easy: name += _(" (easy)"); break;
            case AI::Level::Medium: name += _(" (medium)"); break;
            case AI::Level::Hard: name += _(" (hard)"); break;
        }
    }

    return name;
}
