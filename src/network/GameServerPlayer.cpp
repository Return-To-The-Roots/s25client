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
#include "helpers/mathFuncs.h"
#include "libutil/Log.h"
#include <algorithm>

using boost::chrono::seconds;

template<class T_Duration>
inline typename boost::enable_if<boost::chrono::detail::is_duration<T_Duration>, int>::type toIntSeconds(const T_Duration& duration)
{
    seconds sec = boost::chrono::duration_cast<seconds>(duration);
    return static_cast<int>(helpers::clamp<seconds::rep>(sec.count(), std::numeric_limits<int>::min(), std::numeric_limits<int>::max()));
}

GameServerPlayer::GameServerPlayer(unsigned id, const Socket& socket) //-V818
    : NetworkPlayer(id), isConnecting(true), isPinging(false), isLagging(false), mapDataSent(false)
{
    lastPingTime = Clock::now();
    this->socket = socket;
}

GameServerPlayer::~GameServerPlayer() {}

void GameServerPlayer::doPing()
{
    if(!isConnecting && !isPinging && (Clock::now() - lastPingTime) > seconds(PING_RATE))
    {
        isPinging = true;
        lastPingTime = Clock::now();
        sendQueue.push(new GameMessage_Ping(0xFF));
    }
}

unsigned GameServerPlayer::calcPingTime()
{
    if(!isPinging)
        return 0u;
    isPinging = false;
    TimePoint now = Clock::now();
    int result = toIntSeconds(now - lastPingTime);
    lastPingTime = now;
    return result > 0 ? static_cast<unsigned>(result) : 1u;
}

///////////////////////////////////////////////////////////////////////////////
/// prüft auf Ping-Timeout beim verbinden
bool GameServerPlayer::hasTimedOut() const
{
    if(isConnecting)
        return (Clock::now() - lastPingTime) > seconds(CONNECT_TIMEOUT);
    else if(isPinging)
        return (Clock::now() - lastPingTime) > seconds(PING_TIMEOUT);
    else
        return false;
}

unsigned GameServerPlayer::getLagTimeOut() const
{
    if(!isLagging)
        return LAG_TIMEOUT;
    int timeout = toIntSeconds(lagStartTime + seconds(LAG_TIMEOUT) - Clock::now());
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
