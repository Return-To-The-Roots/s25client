// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"
#include "liblobby/LobbyInterface.h"

class iwLobbyConnect : public IngameWindow, public LobbyInterface
{
public:
    iwLobbyConnect();
    ~iwLobbyConnect() override;

    void LC_LoggedIn(const std::string& email) override;

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
