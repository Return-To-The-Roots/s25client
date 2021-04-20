// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "TestServer.h"
#include "RTTR_Assert.h"
#include "s25util/Message.h"
#include "s25util/SocketSet.h"

bool TestServer::run(bool waitForConnection)
{
    if(!socket.isValid())
        return false;

    SocketSet set;
    set.Add(socket);
    if(set.Select(waitForConnection ? 2 * 60 * 1000 : 0, 0) > 0)
    {
        RTTR_Assert(set.InSet(socket));
        Socket newSocket = socket.Accept();

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
        set.Clear();
        for(Connection& con : connections)
            set.Add(con.so);
        if(set.Select(0, 0) <= 0)
            break;
        for(unsigned id = 0; id < connections.size();)
        {
            if(set.InSet(connections[id].so))
            {
                // nachricht empfangen
                if(connections[id].recvQueue.recv(connections[id].so) < 0)
                    connections.erase(connections.begin() + id);
                else
                {
                    msgReceived = true;
                    ++id;
                }
            } else
                ++id;
        }
    } while(msgReceived);

    handleMessages();

    for(Connection& con : connections)
        con.sendQueue.send(con.so, 10);
    return true;
}

bool TestServer::stop()
{
    connections.clear();
    socket.Close();
    return true;
}

Connection TestServer::acceptConnection(unsigned /*id*/, const Socket& so)
{
    return Connection(Message::create_base, so);
}

bool TestServer::listen(int16_t port)
{
    return socket.Listen(port, false, false);
}
