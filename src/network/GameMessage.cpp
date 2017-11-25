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
#include "GameMessage.h"
#include "GameMessage_GameCommand.h"
#include "GameMessages.h"

bool GameMessage::run(MessageInterface* callback, unsigned playerId)
{
    if(playerId != 0xFFFFFFFF)
        player = playerId;
    return Run(checkedCast<GameMessageInterface*>(callback));
}

Message* GameMessage::create_game(unsigned short id)
{
    Message* msg = NULL;

    switch(id)
    {
        default: return create_base(id);

        case NMS_PING: msg = new GameMessage_Ping(); break;
        case NMS_PONG: msg = new GameMessage_Pong(); break;
        case NMS_SERVER_TYPE: msg = new GameMessage_Server_Type(); break;
        case NMS_SERVER_TYPEOK: msg = new GameMessage_Server_TypeOK(); break;
        case NMS_SERVER_PASSWORD: msg = new GameMessage_Server_Password(); break;
        case NMS_SERVER_NAME: msg = new GameMessage_Server_Name(); break;
        case NMS_SERVER_START: msg = new GameMessage_Server_Start(); break;
        case NMS_SERVER_CHAT: msg = new GameMessage_Server_Chat(); break;
        case NMS_SERVER_ASYNC: msg = new GameMessage_Server_Async(); break;
        case NMS_SERVERCOUNTDOWN: msg = new GameMessage_Server_Countdown(); break;
        case NMS_SERVER_CANCELCOUNTDOWN: msg = new GameMessage_Server_CancelCountdown(); break;
        case NMS_PLAYER_ID: msg = new GameMessage_Player_Id(); break;
        case NMS_PLAYER_NAME: msg = new GameMessage_Player_Name(); break;
        case NMS_PLAYER_LIST: msg = new GameMessage_Player_List(); break;
        case NMS_PLAYER_SETSTATE: msg = new GameMessage_Player_Set_State(); break;
        case NMS_PLAYER_SET_NATION: msg = new GameMessage_Player_Set_Nation(); break;
        case NMS_PLAYER_SET_TEAM: msg = new GameMessage_Player_Set_Team(); break;
        case NMS_PLAYER_SET_COLOR: msg = new GameMessage_Player_Set_Color(); break;
        case NMS_PLAYER_KICKED: msg = new GameMessage_Player_Kicked(); break;
        case NMS_PLAYER_PING: msg = new GameMessage_Player_Ping(); break;
        case NMS_PLAYER_NEW: msg = new GameMessage_Player_New(); break;
        case NMS_PLAYER_READY: msg = new GameMessage_Player_Ready(); break;
        case NMS_PLAYER_SWAP: msg = new GameMessage_Player_Swap(); break;
        case NMS_MAP_INFO: msg = new GameMessage_Map_Info(); break;
        case NMS_MAP_REQUEST: msg = new GameMessage_MapRequest(); break;
        case NMS_MAP_DATA: msg = new GameMessage_Map_Data(); break;
        case NMS_MAP_CHECKSUM: msg = new GameMessage_Map_Checksum(); break;
        case NMS_MAP_CHECKSUMOK: msg = new GameMessage_Map_ChecksumOK(); break;
        case NMS_SERVER_NWF_DONE: msg = new GameMessage_Server_NWFDone; break;
        case NMS_GAMECOMMANDS: msg = new GameMessage_GameCommand; break;
        case NMS_PAUSE: msg = new GameMessage_Pause; break;
        case NMS_SERVER_SPEED: msg = new GameMessage_Server_Speed; break;
        case NMS_GGS_CHANGE: msg = new GameMessage_GGSChange; break;
        case NMS_REMOVE_LUA: msg = new GameMessage_RemoveLua; break;
        case NMS_GET_ASYNC_LOG: msg = new GameMessage_GetAsyncLog(); break;
        case NMS_SEND_ASYNC_LOG: msg = new GameMessage_SendAsyncLog(); break;
        case NMS_SYSTEM_CHAT: msg = new GameMessage_System_Chat(); break;
    }

    return msg;
}

void GameMessage::Serialize(Serializer& ser) const
{
    Message::Serialize(ser);
    ser.PushUnsignedChar(player);
}

void GameMessage::Deserialize(Serializer& ser)
{
    Message::Deserialize(ser);
    player = ser.PopUnsignedChar();
}
