// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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
    void msgEditChange(unsigned ctrl_id) override;
    void msgEditEnter(unsigned ctrl_id) override;
    void msgButtonClick(unsigned ctrl_id) override;
    void msgOptionGroupChange(unsigned ctrl_id, unsigned selection) override;
    bool msgKeyDown(const KeyEvent&) override;

private:
    void setText(const std::string& text, unsigned color, bool button);
    void ReadFromEditAndSaveLobbyData(std::string& user, std::string& pass);
};
