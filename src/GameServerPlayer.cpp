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
#include "defines.h" // IWYU pragma: keep
#include "GameServerPlayer.h"
#include "GameMessage.h"
#include "GameMessages.h"
#include "drivers/VideoDriverWrapper.h"

#include <algorithm>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

GameServerPlayer::GameServerPlayer(const unsigned playerid)
    : GamePlayerInfo(playerid),
      connecttime(0),
      last_command_timeout(0),
      pinging(false),
      send_queue(&GameMessage::create_game),
      recv_queue(&GameMessage::create_game),
      lastping(0)
{
}

GameServerPlayer::GameServerPlayer(const unsigned playerid, Serializer& ser)
    : GamePlayerInfo(playerid, ser),
      connecttime(0),
      last_command_timeout(0),
      pinging(false),
      send_queue(&GameMessage::create_game),
      recv_queue(&GameMessage::create_game),
      lastping(0)
{
}

GameServerPlayer::~GameServerPlayer()
{
    so.Close();
}

///////////////////////////////////////////////////////////////////////////////
/// pingt ggf den Spieler
void GameServerPlayer::doPing()
{
    if( (ps == PS_OCCUPIED) && (!pinging) && ( ( VIDEODRIVER.GetTickCount() - lastping ) > 1000 ) )
    {
        pinging = true;

        lastping = VIDEODRIVER.GetTickCount();

        // Ping Nachricht senden
        send_queue.push(new GameMessage_Ping(0xFF));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// prüft auf Ping-Timeout beim verbinden
void GameServerPlayer::doTimeout()
{
    if( (ps == PS_RESERVED) && ( ( VIDEODRIVER.GetTickCount() - connecttime ) > PING_TIMEOUT ) )
    {
        LOG.lprintf("SERVER: Reserved slot %d freed due to ping timeout\n", playerid);

        /*// Todesnachricht absetzen
        Message_Dead dm();
        dm.send(&so);*/

        // und aufräumen
        clear();
    }
}

/** /////////////////////////////////////////////////////////////////////////////
// setzt den Player auf "reserviert"
// @param sock Socket
// @param id Spieler-ID                                                        */
void GameServerPlayer::reserve(const Socket& sock, unsigned char id)
{
    clear();
    playerid = id;
    connecttime = VIDEODRIVER.GetTickCount();
    so = sock;
    ps = PS_RESERVED;
}

void GameServerPlayer::clear()
{
    GamePlayerInfo::clear();

    connecttime = 0;
    last_command_timeout = 0;
    pinging = false;
    send_queue.clear();
    recv_queue.clear();
    lastping = 0;
    so.Close();
}

unsigned GameServerPlayer::GetTimeOut() const
{
    // Nach 35 Sekunden kicken
    const int timeout = 35 - int(TIME.CurrentTime() - last_command_timeout) / 1000;
    return (timeout >= 0 ? timeout : 0);
}

/// Tauscht Spieler
void GameServerPlayer::SwapInfo(GameServerPlayer& two)
{
    using std::swap;
    GamePlayerInfo::SwapInfo(two);

    swap(this->connecttime, two.connecttime);
    swap(this->last_command_timeout, two.last_command_timeout);

    swap(this->so, two.so);
    swap(this->pinging, two.pinging);

    swap(this->send_queue, two.send_queue);
    swap(this->recv_queue, two.recv_queue);
    swap(this->gc_queue, two.gc_queue);

    swap(this->lastping, two.lastping);
}

/// Spieler laggt
void GameServerPlayer::Lagging()
{
    // Laggt neu?
    if(!last_command_timeout)
        // Anfangs des Laggens merken
        last_command_timeout = TIME.CurrentTime();
}

/// Spieler laggt nicht (mehr)
void GameServerPlayer::NotLagging()
{
    /// Laggt nicht mehr
    last_command_timeout = 0;
}

