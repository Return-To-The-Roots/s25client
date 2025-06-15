// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameMessage.h"
#include "GameMessage_GameCommand.h"
#include "GameMessages.h"
#include "commonDefines.h"

bool GameMessage::run(MessageInterface* callback, unsigned senderPlayerID)
{
    this->senderPlayerID = senderPlayerID;
    return Run(checkedCast<GameMessageInterface*>(callback));
}

Message* GameMessage::create_game(unsigned short id)
{
    Message* msg = nullptr;

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
        case NMS_CHAT: msg = new GameMessage_Chat(); break;
        case NMS_SERVER_ASYNC: msg = new GameMessage_Server_Async(); break;
        case NMS_COUNTDOWN: msg = new GameMessage_Countdown(); break;
        case NMS_CANCEL_COUNTDOWN: msg = new GameMessage_CancelCountdown(); break;
        case NMS_PLAYER_ID: msg = new GameMessage_Player_Id(); break;
        case NMS_PLAYER_NAME: msg = new GameMessage_Player_Name(); break;
        case NMS_PLAYER_PORTRAIT: msg = new GameMessage_Player_Portrait(); break;
        case NMS_PLAYER_LIST: msg = new GameMessage_Player_List(); break;
        case NMS_PLAYER_STATE: msg = new GameMessage_Player_State(); break;
        case NMS_PLAYER_NATION: msg = new GameMessage_Player_Nation(); break;
        case NMS_PLAYER_TEAM: msg = new GameMessage_Player_Team(); break;
        case NMS_PLAYER_COLOR: msg = new GameMessage_Player_Color(); break;
        case NMS_PLAYER_KICKED: msg = new GameMessage_Player_Kicked(); break;
        case NMS_PLAYER_PING: msg = new GameMessage_Player_Ping(); break;
        case NMS_PLAYER_NEW: msg = new GameMessage_Player_New(); break;
        case NMS_PLAYER_READY: msg = new GameMessage_Player_Ready(); break;
        case NMS_PLAYER_SWAP: msg = new GameMessage_Player_Swap(); break;
        case NMS_PLAYER_SWAP_CONFIRM: msg = new GameMessage_Player_SwapConfirm(); break;
        case NMS_MAP_INFO: msg = new GameMessage_Map_Info(); break;
        case NMS_MAP_REQUEST: msg = new GameMessage_MapRequest(); break;
        case NMS_MAP_DATA: msg = new GameMessage_Map_Data(); break;
        case NMS_MAP_CHECKSUM: msg = new GameMessage_Map_Checksum(); break;
        case NMS_MAP_CHECKSUMOK: msg = new GameMessage_Map_ChecksumOK(); break;
        case NMS_SERVER_NWF_DONE: msg = new GameMessage_Server_NWFDone(); break;
        case NMS_GAMECOMMANDS: msg = new GameMessage_GameCommand(); break;
        case NMS_PAUSE: msg = new GameMessage_Pause(); break;
        case NMS_SKIP_TO_GF: msg = new GameMessage_SkipToGF(); break;
        case NMS_SERVER_SPEED: msg = new GameMessage_Speed(); break;
        case NMS_GGS_CHANGE: msg = new GameMessage_GGSChange(); break;
        case NMS_REMOVE_LUA: msg = new GameMessage_RemoveLua(); break;
        case NMS_GET_ASYNC_LOG: msg = new GameMessage_GetAsyncLog(); break;
        case NMS_ASYNC_LOG: msg = new GameMessage_AsyncLog(); break;
    }

    return msg;
}

void GameMessageWithPlayer::Serialize(Serializer& ser) const
{
    Message::Serialize(ser);
    ser.PushUnsignedChar(player);
}

void GameMessageWithPlayer::Deserialize(Serializer& ser)
{
    Message::Deserialize(ser);
    player = ser.PopUnsignedChar();
}
