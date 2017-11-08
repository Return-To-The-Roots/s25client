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
#include "drivers/VideoDriverWrapper.h"

#include <algorithm>

GameServerPlayer::GameServerPlayer()
    : connecttime(0), last_command_timeout(0), pinging(false), send_queue(&GameMessage::create_game), recv_queue(&GameMessage::create_game),
      lastping(0)
{}

GameServerPlayer::~GameServerPlayer() {}

///////////////////////////////////////////////////////////////////////////////
/// pingt ggf den Spieler
void GameServerPlayer::doPing()
{
    if((ps == PS_OCCUPIED) && (!pinging) && ((VIDEODRIVER.GetTickCount() - lastping) > 1000))
    {
        pinging = true;

        lastping = VIDEODRIVER.GetTickCount();

        // Ping Nachricht senden
        send_queue.push(new GameMessage_Ping(0xFF));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// prüft auf Ping-Timeout beim verbinden
void GameServerPlayer::checkConnectTimeout()
{
    if((ps == PS_RESERVED) && ((VIDEODRIVER.GetTickCount() - connecttime) > PING_TIMEOUT))
    {
        LOG.write("SERVER: Reserved slot freed due to ping timeout\n");
        CloseConnections();
    }
}

void GameServerPlayer::reserve(const Socket& sock)
{
    ps = PS_RESERVED;
    so = sock;
    connecttime = VIDEODRIVER.GetTickCount();
    pinging = false;
}

void GameServerPlayer::CloseConnections()
{
    // Free slot
    ps = PS_FREE;
    // Close socket and clear queues
    so.Close();
    send_queue.clear();
    recv_queue.clear();
    gc_queue.clear();
}

unsigned GameServerPlayer::GetTimeOut() const
{
    // Nach 35 Sekunden kicken
    const int timeout = 35 - int(s25util::Time::CurrentTime() - last_command_timeout) / 1000;
    return (timeout >= 0 ? timeout : 0);
}

/// Spieler laggt
void GameServerPlayer::Lagging()
{
    // Laggt neu?
    if(!last_command_timeout)
        // Anfangs des Laggens merken
        last_command_timeout = s25util::Time::CurrentTime();
}

/// Spieler laggt nicht (mehr)
void GameServerPlayer::NotLagging()
{
    /// Laggt nicht mehr
    last_command_timeout = 0;
}
