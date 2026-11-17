// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"

class IChatCmdListener
{
public:
    virtual void onChatCommand(const std::string& cmd) = 0;
};

class iwChat : public IngameWindow
{
public:
    iwChat(Window* parent);

private:
    void msgPaintBefore() override;
    void msgOptionGroupChange(unsigned ctrl_id, unsigned selection) override;
    void msgEditEnter(unsigned ctrl_id) override;
};
