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

#include "Desktop.h"
#include "network/ClientInterface.h"
#include "gameData/GameLoader.h"
#include "liblobby/LobbyInterface.h"
#include <memory>

class dskGameInterface;

class dskGameLoader : public Desktop, public ClientInterface, public LobbyInterface
{
public:
    dskGameLoader(std::shared_ptr<Game> game);
    ~dskGameLoader() override;

    void LC_Status_Error(const std::string& error) override;
    void CI_GameStarted() override;
    void CI_Error(ClientError ce) override;

private:
    void Msg_MsgBoxResult(unsigned msgbox_id, MsgboxResult mbr) override;
    void Msg_Timer(unsigned ctrl_id) override;
    void ShowErrorMsg(const std::string& error);

    unsigned position;
    GameLoader loader_;
    std::unique_ptr<dskGameInterface> gameInterface;
};
