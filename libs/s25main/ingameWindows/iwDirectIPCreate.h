// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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
    void msgEditChange(unsigned ctrl_id) override;
    void msgEditEnter(unsigned ctrl_id) override;
    void msgButtonClick(unsigned ctrl_id) override;
    void msgOptionGroupChange(unsigned ctrl_id, unsigned selection) override;

private:
    void SetText(const std::string& text, unsigned color, bool button);

    ServerType server_type;
};
