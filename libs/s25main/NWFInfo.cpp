// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "NWFInfo.h"
#include "FramesInfo.h"
#include "helpers/containerUtils.h"
#include <algorithm>
#include <stdexcept>

void NWFPlayerInfo::checkLagging()
{
    isLagging = commands.empty();
}

void NWFInfo::init(unsigned nextNWF, unsigned cmdDelay)
{
    if(cmdDelay < 1u)
        throw std::runtime_error("Command delay must be at least 1");
    nextNWF_ = nextNWF;
    cmdDelay_ = cmdDelay;
    playerInfos_.clear();
    while(!serverInfos_.empty())
        serverInfos_.pop();
}

void NWFInfo::addPlayer(unsigned playerId)
{
    if(!helpers::contains_if(playerInfos_, [playerId](const auto& info) { return info.id == playerId; }))
        playerInfos_.push_back(NWFPlayerInfo(playerId));
}

void NWFInfo::removePlayer(unsigned playerId)
{
    auto it = std::find_if(playerInfos_.begin(), playerInfos_.end(),
                           [playerId](const auto& info) { return info.id == playerId; });
    if(it != playerInfos_.end())
        playerInfos_.erase(it);
}

bool NWFInfo::addPlayerCmds(unsigned playerId, const PlayerGameCommands& cmds)
{
    auto it = std::find_if(playerInfos_.begin(), playerInfos_.end(),
                           [playerId](const auto& info) { return info.id == playerId; });
    if(it == playerInfos_.end())
        throw std::runtime_error("Player with given player id does not exist");
    // Commands in NWF n are sent for NWF n + cmdDelay. Clients can only execute an NWF (and send their cmds) when all
    // others are received. This means no one can execute NWF n + cmdDelay before we executed NWF n. So the last NWF one
    // can have executed is n + cmDelay - 1 with the commands for n + cmdDelay - 1 + cmdDelay. Counting those leads to
    // cmdDelay*2 pending commands.
    if(it->commands.size() >= 2 * cmdDelay_)
        return false;
    it->commands.push(cmds);
    return true;
}

bool NWFInfo::addServerInfo(const NWFServerInfo& info)
{
    // Next NWF must be in the future
    if(info.nextNWF <= info.gf)
        return false;
    // GF must be the next NWF according to the last NWFServerInfo
    if(serverInfos_.empty())
    {
        if(info.gf != nextNWF_)
            return false;
    } else if(info.gf != serverInfos_.back().nextNWF)
        return false;
    serverInfos_.push(info);
    return true;
}

bool NWFInfo::isReady()
{
    bool result = true;
    for(NWFPlayerInfo& player : playerInfos_)
    {
        player.checkLagging();
        result &= !player.isLagging;
    }
    return result && !serverInfos_.empty();
}

const NWFPlayerInfo& NWFInfo::getPlayerInfo(unsigned playerId) const
{
    auto it = std::find_if(playerInfos_.begin(), playerInfos_.end(),
                           [playerId](const auto& info) { return info.id == playerId; });
    if(it == playerInfos_.end())
        throw std::runtime_error("Player with given player id does not exist");
    else
        return *it;
}

const PlayerGameCommands& NWFInfo::getPlayerCmds(unsigned playerId) const
{
    const NWFPlayerInfo& player = getPlayerInfo(playerId);
    if(player.commands.empty())
        throw std::runtime_error("Player is not ready yet");
    return player.commands.front();
}

const NWFServerInfo& NWFInfo::getServerInfo() const
{
    if(serverInfos_.empty())
        throw std::runtime_error("Server is not ready yet");
    return serverInfos_.front();
}

void NWFInfo::execute(FramesInfo& info)
{
    if(!isReady())
        throw std::runtime_error("Cannot execute NWF if not ready");
    const NWFServerInfo serverInfo = getServerInfo();
    serverInfos_.pop();
    for(NWFPlayerInfo& player : playerInfos_)
    {
        if(player.commands.empty())
            throw std::runtime_error("Cannot execute NWF if not ready");
        player.commands.pop();
    }

    info.gf_length = FramesInfo::milliseconds32_t(serverInfo.newGFLen);
    info.nwf_length = serverInfo.nextNWF - serverInfo.gf;
    nextNWF_ = serverInfo.nextNWF;
}

unsigned NWFInfo::getLastNWF() const
{
    if(serverInfos_.empty())
        throw std::runtime_error("Server is not ready yet");
    return serverInfos_.back().nextNWF;
}
