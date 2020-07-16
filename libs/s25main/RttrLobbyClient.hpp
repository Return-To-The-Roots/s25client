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

#ifndef RttrLobbyClient_h__
#define RttrLobbyClient_h__

#include "ILobbyClient.hpp"

class LobbyClient;

/// Default lobby client for RTTR
class RttrLobbyClient : public ILobbyClient
{
    LobbyClient& client_;

public:
    explicit RttrLobbyClient(LobbyClient& client);
    bool IsLoggedIn() const override;
    void AddListener(LobbyInterface* listener) override;
    void RemoveListener(LobbyInterface* listener) override;
    void SendServerJoinRequest() override;
    void SendChat(const std::string& text) override;
};

#endif // RttrLobbyClient_h__
