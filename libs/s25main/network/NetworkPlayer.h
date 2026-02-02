// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
    /// Execute the handler function for all received messages. If usePlayerId is true, the player in the message will
    /// be replaced by this players id
    void executeMsgs(MessageInterface& msgHandler);

    unsigned playerId;
    MessageQueue recvQueue, sendQueue;
    Socket socket;
};

void swap(NetworkPlayer& lhs, NetworkPlayer& rhs) noexcept;
