// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef TestServer_h__
#define TestServer_h__

#include "s25util/MessageQueue.h"
#include "s25util/Socket.h"
#include <vector>

struct Connection
{
    Socket so;
    MessageQueue sendQueue, recvQueue;
    Connection(CreateMsgFunction createMsg, Socket socket = Socket()) : so(std::move(socket)), sendQueue(createMsg), recvQueue(createMsg) {}
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

#endif // TestServer_h__
