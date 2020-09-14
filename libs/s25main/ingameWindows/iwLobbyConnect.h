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

#include "IngameWindow.h"
#include "liblobby/LobbyInterface.h"

class iwLobbyConnect : public IngameWindow, public LobbyInterface
{
public:
    iwLobbyConnect();
    ~iwLobbyConnect() override;

    void LC_LoggedIn(const std::string& email) override;
    void LC_Registered() override;

    void LC_Status_Waiting() override;
    void LC_Status_Error(const std::string& error) override;

protected:
    void Msg_EditChange(unsigned ctrl_id) override;
    void Msg_EditEnter(unsigned ctrl_id) override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_OptionGroupChange(unsigned ctrl_id, unsigned selection) override;
    bool Msg_KeyDown(const KeyEvent&) override;

private:
    void SetText(const std::string& text, unsigned color, bool button);
    void ReadFromEditAndSaveLobbyData(std::string& user, std::string& pass);
};
