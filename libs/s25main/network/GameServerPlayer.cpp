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
#include <limits>

using std::chrono::seconds;

namespace {
template<class T_Duration>
int durationToInt(const T_Duration& duration)
{
    return helpers::clamp(duration.count(), std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
}
} // namespace

GameServerPlayer::GameServerPlayer(unsigned id, const Socket& socket) //-V818
    : NetworkPlayer(id), isPinging(false), ping(3), mapDataSent(false)
{
    connectTimer.start();
    this->socket = socket;
}

GameServerPlayer::~GameServerPlayer() {}

void GameServerPlayer::setConnected()
{
    connectTimer.stop();
}

void GameServerPlayer::doPing()
{
    if(isConnected() && !isPinging && (!pingTimer.isRunning() || pingTimer.getElapsed() >= seconds(PING_RATE)))
    {
        isPinging = true;
        pingTimer.restart();
        sendQueue.push(new GameMessage_Ping(0xFF));
    }
}

unsigned GameServerPlayer::calcPingTime()
{
    if(!isPinging)
        return 0u;
    int result = durationToInt(std::chrono::duration_cast<std::chrono::milliseconds>(pingTimer.getElapsed()));
    isPinging = false;
    pingTimer.restart();
    unsigned curPing = static_cast<unsigned>(std::max(1, result));
    ping.add(curPing);
    return ping.get();
}

bool GameServerPlayer::hasTimedOut() const
{
    if(!isConnected())
        return connectTimer.getElapsed() > seconds(CONNECT_TIMEOUT);
    else if(isPinging)
        return pingTimer.getElapsed() > seconds(PING_TIMEOUT);
    else
        return false;
}

unsigned GameServerPlayer::getLagTimeOut() const
{
    if(!lagTimer.isRunning())
        return LAG_TIMEOUT;
    int timeout = durationToInt(std::chrono::duration_cast<seconds>(seconds(LAG_TIMEOUT) - lagTimer.getElapsed()));
    return static_cast<unsigned>(std::max(0, timeout));
}

void GameServerPlayer::setLagging()
{
    lagTimer.restart();
}

void GameServerPlayer::setNotLagging()
{
    lagTimer.stop();
}
