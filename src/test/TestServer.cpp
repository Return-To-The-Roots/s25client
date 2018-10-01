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
#include "TestServer.h"
#include "libutil/Message.h"
#include "libutil/SocketSet.h"
#include <boost/foreach.hpp>

bool TestServer::run()
{
    if(!socket.isValid())
        return false;

    SocketSet set;

    BOOST_FOREACH(Connection& con, connections)
        set.Add(con.so);

    // Check for errors
    if(set.Select(0, 2) > 0)
    {
        for(unsigned id = 0; id < connections.size();)
        {
            if(set.InSet(connections[id].so))
                connections.erase(connections.begin() + id);
            else
                ++id;
        }
    }

    set.Clear();
    set.Add(socket);
    if(set.Select(0, 0) > 0)
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
        BOOST_FOREACH(Connection& con, connections)
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

    BOOST_FOREACH(Connection& con, connections)
        con.sendQueue.send(con.so, 10);
    return true;
}

bool TestServer::stop()
{
    connections.clear();
    socket.Close();
    return true;
}

Connection TestServer::acceptConnection(unsigned id, const Socket& so)
{
    return Connection(Message::create_base, so);
}

bool TestServer::listen(int16_t port)
{
    return socket.Listen(port, false, false);
}
