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

#include "rttrDefines.h" // IWYU pragma: keep
#include "GameServerPlayer.h"
#include "GameMessage_GameCommand.h"
#include "GameMessages.h"
#include <algorithm>

using boost::chrono::seconds;

GameServerPlayer::GameServerPlayer(unsigned id, const Socket& socket) //-V818
    : NetworkPlayer(id), isConnecting(true), pinging(false), isLagging(false)
{
    connectTime = Clock::now();
    this->socket = socket;
}

GameServerPlayer::~GameServerPlayer() {}

void GameServerPlayer::doPing()
{
    if(!isConnecting && !pinging && (Clock::now() - lastPingTime) > seconds(PING_RATE))
    {
        pinging = true;
        lastPingTime = Clock::now();
        sendQueue.push(new GameMessage_Ping(0xFF));
    }
}

unsigned GameServerPlayer::calcPingTime()
{
    if(!pinging)
        return 0u;
    pinging = false;
    TimePoint now = Clock::now();
    int result = boost::chrono::duration_cast<boost::chrono::duration<int> >(now - lastPingTime).count();
    lastPingTime = now;
    return result > 0 ? static_cast<unsigned>(result) : 1u;
}

///////////////////////////////////////////////////////////////////////////////
/// prüft auf Ping-Timeout beim verbinden
bool GameServerPlayer::hasConnectTimedOut() const
{
    if(isConnecting && (Clock::now() - connectTime) > seconds(CONNECT_TIMEOUT))
        return true;
    else
        return false;
}

void GameServerPlayer::closeConnection(bool flushMsgsFirst)
{
    NetworkPlayer::closeConnection(flushMsgsFirst);
    checksumOfNextNWF = std::queue<AsyncChecksum>();
}

unsigned GameServerPlayer::getLagTimeOut() const
{
    const int timeout =
      boost::chrono::duration_cast<boost::chrono::duration<int> >(lagStartTime + seconds(LAG_TIMEOUT) - Clock::now()).count();
    return static_cast<unsigned>(std::max(0, timeout));
}

void GameServerPlayer::setLagging()
{
    /// Start lagging time if we are not yet lagging
    if(!isLagging)
    {
        isLagging = true;
        lagStartTime = Clock::now();
    }
}

void GameServerPlayer::setNotLagging()
{
    isLagging = false;
}
