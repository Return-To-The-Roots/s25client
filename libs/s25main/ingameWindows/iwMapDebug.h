// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"
#include <memory>

class GameWorldView;

class iwMapDebug : public IngameWindow
{
public:
    iwMapDebug(GameWorldView& gwv, bool allowCheating);
    ~iwMapDebug() override;

private:
    class DebugPrinter;
    class EventChecker;

    void Msg_ComboSelectItem(unsigned ctrl_id, unsigned select) override;
    void Msg_CheckboxChange(unsigned ctrl_id, bool checked) override;
    void Msg_Timer(unsigned ctrl_id) override;

    GameWorldView& gwv;
    std::unique_ptr<DebugPrinter> printer;
    std::unique_ptr<EventChecker> eventChecker;
};
