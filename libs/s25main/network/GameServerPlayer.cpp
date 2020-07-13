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

#include "GameServerPlayer.h"
#include "GameMessages.h"
#include "helpers/mathFuncs.h"
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
    : NetworkPlayer(id), state_(JustConnectedState())
{
    boost::get<JustConnectedState>(state_).timer.start();
    this->socket = socket;
}

GameServerPlayer::~GameServerPlayer() = default;

void GameServerPlayer::setMapSending(std::chrono::seconds estimatedSendTime)
{
    MapSendingState state;
    state.timer.start();
    state.estimatedSendTime = estimatedSendTime;
    state_ = std::move(state);
}

void GameServerPlayer::setActive()
{
    state_ = ActiveState(3);
}

void GameServerPlayer::doPing()
{
    ActiveState* state = boost::get<ActiveState>(&state_);
    if(state && !state->isPinging && (!state->pingTimer.isRunning() || state->pingTimer.getElapsed() >= seconds(PING_RATE)))
    {
        state->isPinging = true;
        state->pingTimer.restart();
        sendQueue.push(new GameMessage_Ping(0xFF));
    }
}

unsigned GameServerPlayer::calcPingTime()
{
    auto& state = boost::get<ActiveState>(state_);
    if(!state.isPinging)
        return 0u;
    int result = durationToInt(std::chrono::duration_cast<std::chrono::milliseconds>(state.pingTimer.getElapsed()));
    state.isPinging = false;
    state.pingTimer.restart();
    unsigned curPing = static_cast<unsigned>(std::max(1, result));
    state.ping.add(curPing);
    return state.ping.get();
}

bool GameServerPlayer::hasTimedOut() const
{
    return boost::apply_visitor(
      composeVisitor([](const JustConnectedState& s) { return s.timer.getElapsed() > seconds(CONNECT_TIMEOUT); },
                     [](const MapSendingState& s) { return s.timer.getElapsed() > seconds(CONNECT_TIMEOUT) + s.estimatedSendTime; },
                     [](const ActiveState& s) { return s.isPinging && s.pingTimer.getElapsed() > seconds(PING_TIMEOUT); }),
      state_);
}

unsigned GameServerPlayer::getLagTimeOut() const
{
    const ActiveState& state = boost::get<ActiveState>(state_);
    if(!state.lagTimer.isRunning())
        return LAG_TIMEOUT;
    int timeout = durationToInt(std::chrono::duration_cast<seconds>(seconds(LAG_TIMEOUT) - state.lagTimer.getElapsed()));
    return static_cast<unsigned>(std::max(0, timeout));
}

void GameServerPlayer::setLagging()
{
    boost::get<ActiveState>(state_).lagTimer.restart();
}

void GameServerPlayer::setNotLagging()
{
    boost::get<ActiveState>(state_).lagTimer.stop();
}
