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

#pragma once

#include "s25util/MessageQueue.h"
#include "s25util/Socket.h"

class Message;
class MessageInterface;

/// A player with a network connection and send/recv queues
class NetworkPlayer
{
public:
    NetworkPlayer(unsigned playerId);
    virtual ~NetworkPlayer() = default;
    /// Close the socket and clear queues
    virtual void closeConnection();
    /// Receive all waiting messages from the socket. Return false on error
    bool receiveMsgs();
    /// Send at most maxNumMsgs (if non-negative). Return false on error
    bool sendMsgs(int maxNumMsgs);
    /// Enqueue a message to be send later
    void sendMsgAsync(Message* msg);
    /// Send a message synchronously
    void sendMsg(const Message& msg);
    /// Execute the handler function for all received messages. If usePlayerId is true, the player in the message will be replaced by this
    /// players id
    void executeMsgs(MessageInterface& msgHandler);

    unsigned playerId;
    MessageQueue recvQueue, sendQueue;
    Socket socket;
};

void swap(NetworkPlayer& lhs, NetworkPlayer& rhs);
