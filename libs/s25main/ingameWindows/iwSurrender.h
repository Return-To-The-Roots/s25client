// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"

class iwSurrender : public IngameWindow
{
public:
    iwSurrender();

private:
    void Msg_ButtonClick(unsigned ctrl_id) override;
};
