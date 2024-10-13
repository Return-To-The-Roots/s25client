// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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
    void Msg_EditEnter(unsigned ctrl_id) override;
    bool Msg_KeyDown(const KeyEvent& ke);

    void SetActive(bool activate) override;

    GameWorldView& gwv;
    std::unique_ptr<DebugPrinter> printer;
    std::unique_ptr<EventChecker> eventChecker;
};
