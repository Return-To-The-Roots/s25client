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

    void msgComboSelectItem(unsigned ctrl_id, unsigned select) override;
    void msgCheckboxChange(unsigned ctrl_id, bool checked) override;
    void msgTimer(unsigned ctrl_id) override;
    void msgEditEnter(unsigned ctrl_id) override;
    bool msgKeyDown(const KeyEvent& ke) override;

    void SetActive(bool activate) override;

    GameWorldView& gwv;
    std::unique_ptr<DebugPrinter> printer;
    std::unique_ptr<EventChecker> eventChecker;
};
