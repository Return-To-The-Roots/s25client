// $Id: GameProtocol.h 9557 2014-12-18 08:57:19Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef GAMEPROTOCOL_H_INCLUDED
#define GAMEPROTOCOL_H_INCLUDED

#include "Protocol.h"

#pragma once

///////////////////////////////////////////////////////////////////////////////
// Netzwerk Messages                       // client> | <server
enum
{
    NMS_PING = 0x0001, // 0
    NMS_PONG = 0x0002, // 0

    NMS_SERVER_TYPE = 0x0101,   // 1 servertyp, x server-version
    NMS_SERVER_TYPEOK,          // 1 servertyp, x server-version
    NMS_SERVER_PASSWORD,        // x serverpassword | 1 serverpasswordok
    NMS_SERVER_NAME,            // x servername
    NMS_SERVER_START,           //
    NMS_SERVER_CHAT,            // 1 destination, x text | 1 source, x text
    NMS_SERVER_ASYNC,           // playercount*4 int (Checksummen)
    NMS_SERVER_COUNTDOWN,       // 4 countdown
    NMS_SERVER_CANCELCOUNTDOWN, // 0

    NMS_PLAYER_ID = 0x0201, // 1 playerid
    NMS_PLAYER_NAME, // x playername
    NMS_PLAYER_LIST, // 1 playercount | x GamePlayerInfo
    NMS_PLAYER_TOGGLESTATE, // 1 playerid
    NMS_PLAYER_TOGGLENATION, // 0 | 1 playerid
    NMS_PLAYER_TOGGLETEAM, // 0 | 1 playerid
    NMS_PLAYER_TOGGLECOLOR, // 0 | 1 playerid
    NMS_PLAYER_KICKED, // 12 npk
    NMS_PLAYER_PING, // 1 playerid, 2 playerping
    NMS_PLAYER_NEW, // 1 playerid, x playername
    NMS_PLAYER_READY, // 1 status | 1 playerid, 1 status
    NMS_PLAYER_SWAP, // 1 playerid1, 1 playerid2

    NMS_MAP_NAME = 0x0301,  // x mapname
    NMS_MAP_INFO,           // 0 | 4 parts, 4 ziplength, 4 length
    NMS_MAP_DATA,           // 0 | x mappartdata
    NMS_MAP_CHECKSUM,       // 4 checksum
    NMS_MAP_CHECKSUMOK,     // 1 checksumok

    NMS_SERVER_NWF_DONE = 0x0401, // 0
    NMS_GAMECOMMANDS,
    NMS_PAUSE,
    NMS_SERVER_SPEED,

    NMS_GGS_CHANGE = 0x0501, //

    NMS_GET_ASYNC_LOG = 0x0600,
    NMS_SEND_ASYNC_LOG
};

///////////////////////////////////////////////////////////////////////////////
/* Hinweise:

bc = broadcast
ec = broadcast to enemy
tc = broadcast to team

*/
///////////////////////////////////////////////////////////////////////////////
/* Reihenfolge bei Client:

NMS_NULL_MSG            --> ignore

NMS_PING                --> NMS_PONG

NMS_PLAYER_ID           --> ok ? NMS_SERVER_TYP : disconnect
NMS_PLAYER_TOGGLESTATE  -->
NMS_PLAYER_TOGGLENATION --> NMS_PLAYER_TOGGLENATION
NMS_PLAYER_TOGGLETEAM   --> NMS_PLAYER_TOGGLETEAM
NMS_PLAYER_TOGGLECOLOR  --> NMS_PLAYER_TOGGLECOLOR
NMS_PLAYER_KICKED       -->

NMS_SERVER_TYP          --> ok ? NMS_SERVER_PASSWORD : disconnect
NMS_SERVER_PASSWORD     --> ok ? NMS_PLAYER_NAME : disconnect

NMS_MAP_NAME            --> NMS_MAP_INFO
NMS_MAP_INFO            --> parts*NMS_MAP_DATA
NMS_MAP_DATA            --> part >= parts ? NMS_MAP_CHECKSUM
NMS_MAP_CHECKSUM        --> ( !ok ) ? disconnect

NMS_GGS_CHANGE          -->

NMS_NFC_ANSWER          -->

NMS_DEAD_MSG            --> disconnect

*/
///////////////////////////////////////////////////////////////////////////////
/* Reihenfolge bei Server:

NMS_NULL_MSG            --> ignore

NMS_PONG                --> bc(NMS_PLAYER_PING)

connect                 --> NMS_PLAYER_ID

NMS_SERVER_TYP          --> NMS_SERVER_TYP, ( !ok ) ? kick
NMS_SERVER_PASSWORD     --> NMS_SERVER_PASSWORD, ( !ok ) ? kick
NMS_SERVER_CHAT         --> toteam ? tc(NMS_SERVER_CHAT) : toenemy ? ec(NMS_SERVER_CHAT) : bc(NMS_SERVER_CHAT)

NMS_PLAYER_NAME         --> NMS_MAP_NAME
NMS_PLAYER_TOGGLESTATE  --> (playerstate != KI) ? kick(playerid) : bc(NMS_PLAYER_TOGGLESTATE)
NMS_PLAYER_TOGGLENATION --> bc(NMS_PLAYER_TOGGLENATION)
NMS_PLAYER_TOGGLETEAM   --> bc(NMS_PLAYER_TOGGLETEAM)
NMS_PLAYER_TOGGLECOLOR  --> bc(NMS_PLAYER_TOGGLECOLOR)

NMS_MAP_INFO            --> NMS_MAP_INFO
NMS_MAP_DATA            --> NMS_MAP_DATA
NMS_MAP_CHECKSUM        --> NMS_MAP_CHECKSUM, ( !ok ) ? kick : ( bc(NMS_PLAYER_NEW), NMS_SERVER_NAME, NMS_PLAYER_LIST, NMS_GGS_CHANGE )

NMS_NFC_COMMAND         --> NMS_NFC_ANSWER

NMS_DEAD_MSG            --> kick

kick                    --> bc(NMS_PLAYER_KICK), NMS_DEAD_MSG

*/
///////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////
// Strukturen fr einzelne Messages
struct NS_PlayerKicked
{
    unsigned char playerid;
    unsigned char cause;
    unsigned short param;
};

//////////////////////////////////////////
// Sonstige Konstanten

// Gründe fürs Kicken
enum
{
    NP_NOCAUSE = 0, // Ohne Grund --> manuell vom GameServer rausgehauen, weiß der Teufel warum
    NP_CONNECTIONLOST, // Verbindung verloren/abgebrochen, wie auch immer
    NP_INVALIDMSG, // Unglütige Message, (evtl Cheater bzw. Asynchronität)
    NP_INCOMPLETEMSG, // zu wenig gesendet
    NP_PINGTIMEOUT, // Ping Timeout
    NP_WRONGPASSWORD, // falsches passwort
    NP_WRONGCHECKSUM, // falsche Checksumme
    NP_ASYNC // asynchron
};

// Servertypen
enum
{
    NP_LOBBY = 0,
    NP_DIRECT,
    NP_LOCAL
};

// Wie lange maximal warten, bis Rausschmiss des Spielers (in milliseconds)
const unsigned PING_TIMEOUT     = 2 * 60 * 1000; // 2min

// Ziele fürs Chatten (Ingame)
enum ChatDestination
{
    CD_SYSTEM = 0,
    CD_ALL,
    CD_ALLIES,
    CD_ENEMIES
};

/// Map-Typ
enum MapType
{
    MAPTYPE_OLDMAP = 0,
    MAPTYPE_SAVEGAME,
    MAPTYPE_RTTRMAP,
    MAPTYPE_RANDOMMAP
};

/// Größe eines Map-Paketes
/// ACHTUNG: IPV4 garantiert nur maximal 576!!
const unsigned MAP_PART_SIZE = 512;

#endif // !GAMEPROTOCOL_H_INCLUDED
