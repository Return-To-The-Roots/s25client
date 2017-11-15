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
#include "NetworkPlayer.h"
#include "GameMessage.h"

NetworkPlayer::NetworkPlayer(unsigned playerId)
    : playerId(playerId), recvQueue(GameMessage::create_game), sendQueue(GameMessage::create_game)
{}

void NetworkPlayer::closeConnection(bool flushMsgsFirst)
{
    if(flushMsgsFirst && socket.isValid())
        sendQueue.flush(socket);
    // Close socket and clear queues
    socket.Close();
    sendQueue.clear();
    recvQueue.clear();
}

bool NetworkPlayer::receiveMsgs()
{
    return recvQueue.recv(socket);
}

bool NetworkPlayer::sendMsgs(int maxNumMsgs)
{
    return sendQueue.send(socket, maxNumMsgs);
}

void NetworkPlayer::sendMsgAsync(Message* msg)
{
    sendQueue.push(msg);
}

void NetworkPlayer::sendMsg(const Message& msg)
{
    sendQueue.sendMessage(socket, msg);
}

void NetworkPlayer::executeMsgs(MessageInterface& msgHandler, bool usePlayerId)
{
    while(!recvQueue.empty())
    {
        // Do we want to overwrite the player id? Note: Do it IN the loop as a run command may change the playerId
        unsigned usedPlayerId = usePlayerId ? playerId : 0xFFFFFFFF;
        recvQueue.front()->run(&msgHandler, usedPlayerId);
        recvQueue.pop();
    }
}

void swap(NetworkPlayer& lhs, NetworkPlayer& rhs)
{
    using std::swap;
    swap(lhs.playerId, rhs.playerId);
    swap(lhs.recvQueue, rhs.recvQueue);
    swap(lhs.sendQueue, rhs.sendQueue);
    swap(lhs.socket, rhs.socket);
}
