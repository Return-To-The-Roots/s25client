// Copyright (c) 2005 - 2018 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"

class iwVictory : public IngameWindow
{
public:
    iwVictory(const std::vector<std::string>& winnerNames);

private:
    void Msg_ButtonClick(unsigned ctrl_id) override;
};
