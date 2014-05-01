// $Id: GameMessageInterface.cpp 9381 2014-05-01 10:27:24Z FloSoft $
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "GameMessageInterface.h"

void GameMessageInterface::OnNMSPing(const GameMessage_Ping& msg) {}
void GameMessageInterface::OnNMSPong(const GameMessage_Pong& msg) {}

void GameMessageInterface::OnNMSServerType(const GameMessage_Server_Type& msg) {}
void GameMessageInterface::OnNMSServerTypeOK(const GameMessage_Server_TypeOK& msg) {}
void GameMessageInterface::OnNMSServerPassword(const GameMessage_Server_Password& msg) {}
void GameMessageInterface::OnNMSServerName(const GameMessage_Server_Name& msg) {}
void GameMessageInterface::OnNMSServerStart(const GameMessage_Server_Start& msg) {}
void GameMessageInterface::OnNMSServerChat(const GameMessage_Server_Chat& msg) {}
void GameMessageInterface::OnNMSServerAsync(const GameMessage_Server_Async& msg) {}
void GameMessageInterface::OnNMSServerCountdown(const GameMessage_Server_Countdown& msg) {}
void GameMessageInterface::OnNMSServerCancelCountdown(const GameMessage_Server_CancelCountdown& msg) {}

void GameMessageInterface::OnNMSPlayerId(const GameMessage_Player_Id& msg) {}
void GameMessageInterface::OnNMSPlayerName(const GameMessage_Player_Name& msg) {}
void GameMessageInterface::OnNMSPlayerList(const GameMessage_Player_List& msg) {}
void GameMessageInterface::OnNMSPlayerToggleState(const GameMessage_Player_Toggle_State& msg) {}
void GameMessageInterface::OnNMSPlayerToggleNation(const GameMessage_Player_Toggle_Nation& msg) {}
void GameMessageInterface::OnNMSPlayerToggleTeam(const GameMessage_Player_Toggle_Team& msg) {}
void GameMessageInterface::OnNMSPlayerToggleColor(const GameMessage_Player_Toggle_Color& msg) {}
void GameMessageInterface::OnNMSPlayerKicked(const GameMessage_Player_Kicked& msg) {}
void GameMessageInterface::OnNMSPlayerPing(const GameMessage_Player_Ping& msg) {}
void GameMessageInterface::OnNMSPlayerNew(const GameMessage_Player_New& msg) {}
void GameMessageInterface::OnNMSPlayerReady(const GameMessage_Player_Ready& msg) {}
void GameMessageInterface::OnNMSPlayerSwap(const GameMessage_Player_Swap& msg) {}

void GameMessageInterface::OnNMSMapInfo(const GameMessage_Map_Info& msg) {}
void GameMessageInterface::OnNMSMapData(const GameMessage_Map_Data& msg) {}
void GameMessageInterface::OnNMSMapChecksum(const GameMessage_Map_Checksum& msg) {}
void GameMessageInterface::OnNMSMapChecksumOK(const GameMessage_Map_ChecksumOK& msg) {}


void GameMessageInterface::OnNMSPause(const GameMessage_Pause& msg) {}
void GameMessageInterface::OnNMSServerDone(const GameMessage_Server_NWFDone& msg) {}
void GameMessageInterface::OnNMSGameCommand(const GameMessage_GameCommand& msg) {}
void GameMessageInterface::OnNMSServerSpeed(const GameMessage_Server_Speed& msg) {}

void GameMessageInterface::OnNMSGGSChange(const GameMessage_GGSChange& msg) {}

void GameMessageInterface::OnNMSGetAsyncLog(const GameMessage_GetAsyncLog& msg) {}
void GameMessageInterface::OnNMSSendAsyncLog(const GameMessage_SendAsyncLog& msg, std::list<RandomEntry>* his, bool last) {}

