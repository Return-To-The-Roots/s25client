// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "NetworkPlayer.h"
#include "GameMessage.h"

NetworkPlayer::NetworkPlayer(unsigned playerId)
    : playerId(playerId), recvQueue(GameMessage::create_game), sendQueue(GameMessage::create_game)
{}

void NetworkPlayer::closeConnection()
{
    // Close socket and clear queues
    socket.Close();
    sendQueue.clear();
    recvQueue.clear();
}

bool NetworkPlayer::receiveMsgs()
{
    return recvQueue.recvAll(socket) >= 0;
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
    MessageQueue::sendMessage(socket, msg);
}

void NetworkPlayer::executeMsgs(MessageInterface& msgHandler)
{
    while(!recvQueue.empty())
    {
        recvQueue.front()->run(&msgHandler, playerId);
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
