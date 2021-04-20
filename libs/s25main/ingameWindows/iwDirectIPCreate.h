// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"
#include "gameTypes/ServerType.h"

class iwDirectIPCreate : public IngameWindow
{
public:
    iwDirectIPCreate(ServerType server_type);

    void LC_Status_Error(const std::string& error);

protected:
    void Msg_EditChange(unsigned ctrl_id) override;
    void Msg_EditEnter(unsigned ctrl_id) override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_OptionGroupChange(unsigned ctrl_id, unsigned selection) override;

private:
    void SetText(const std::string& text, unsigned color, bool button);

    ServerType server_type;
};
