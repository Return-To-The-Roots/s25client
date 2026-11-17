// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "TestServer.h"
#include "RTTR_Assert.h"
#include "rttr/test/random.hpp"
#include "s25util/Message.h"
#include "s25util/SocketSet.h"
#include <iostream>

bool TestServer::run(bool waitForConnection)
{
    if(!socket.isValid())
        return false;

    SocketSet set;
    set.add(socket);
    if(set.select(waitForConnection ? 2 * 60 * 1000 : 0, 0) > 0)
    {
        RTTR_Assert(set.inSet(socket));
        Socket newSocket = socket.accept();

        if(newSocket.isValid())
        {
            Connection con = acceptConnection(connections.size(), newSocket);
            if(con.so.isValid())
                connections.push_back(con);
        }
    }

    bool msgReceived;
    do
    {
        msgReceived = true;
        set.clear();
        for(Connection& con : connections)
            set.add(con.so);
        if(set.select(0, 0) <= 0)
            break;
        for(unsigned id = 0; id < connections.size();)
        {
            if(set.inSet(connections[id].so))
            {
                if(connections[id].recvQueue.recv(connections[id].so) < 0)
                    connections.erase(connections.begin() + id);
                else
                {
                    msgReceived = true;
                    ++id;
                }
            } else if(!connections[id].so.isValid())
                connections.erase(connections.begin() + id); // LCOV_EXCL_LINE
            else
                ++id;
        }
    } while(msgReceived);

    handleMessages();

    for(Connection& con : connections)
    {
        if(!con.sendQueue.send(con.so, 10))
        {
            // LCOV_EXCL_START
            std::cerr << "TestServer: Failed to send messages to connection " << (&con - connections.data())
                      << std::endl;
            // LCOV_EXCL_STOP
        }
    }
    return true;
}

bool TestServer::stop()
{
    connections.clear();
    socket.close();
    return true;
}

Connection TestServer::acceptConnection(unsigned /*id*/, const Socket& so)
{
    return Connection(Message::create_base, so);
}

bool TestServer::listen(int16_t port)
{
    return socket.listen(port, false, false);
}

int TestServer::tryListen(const unsigned maxTries)
{
    for(unsigned i = 0; i < maxTries; i++)
    {
        const auto curPort = rttr::test::randomValue(1024, 49151);
        if(listen(curPort))
            return curPort;
    }
    return -1; // LCOV_EXCL_LINE
}
