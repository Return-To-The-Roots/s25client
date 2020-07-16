// Copyright (c) 2016 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef ILobbyClient_h__
#define ILobbyClient_h__

#include <string>

class LobbyInterface;

/// Interface for lobby clients
class ILobbyClient
{
public:
    virtual ~ILobbyClient() = default;
    virtual bool IsLoggedIn() const = 0;
    virtual void AddListener(LobbyInterface* listener) = 0;
    virtual void RemoveListener(LobbyInterface* listener) = 0;
    virtual void SendServerJoinRequest() = 0;
    virtual void SendChat(const std::string& text) = 0;
};

#endif // ILobbyClient_h__
