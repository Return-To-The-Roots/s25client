// Copyright (C) 2016 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "s25util/MessageQueue.h"
#include "s25util/Socket.h"
#include <vector>

struct Connection
{
    Socket so;
    MessageQueue sendQueue, recvQueue;
    Connection(CreateMsgFunction createMsg, Socket socket = Socket())
        : so(std::move(socket)), sendQueue(createMsg), recvQueue(createMsg)
    {}
};

class TestServer
{
public:
    virtual ~TestServer() = default;
    bool listen(int16_t port);
    bool run(bool waitForConnection = false);
    bool stop();
    virtual void handleMessages() {}
    virtual Connection acceptConnection(unsigned id, const Socket& so);

    Socket socket;
    std::vector<Connection> connections;
};
