// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"

class IChatCmdListener
{
public:
    virtual void OnChatCommand(const std::string& cmd) = 0;
};

class iwChat : public IngameWindow
{
public:
    iwChat(Window* parent);

private:
    void Msg_PaintBefore() override;
    void Msg_OptionGroupChange(unsigned ctrl_id, unsigned selection) override;
    void Msg_EditEnter(unsigned ctrl_id) override;
};
