// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"

class SoundManager;

class iwOptionsWindow : public IngameWindow
{
    SoundManager& soundManager;

public:
    iwOptionsWindow(SoundManager& soundManager);

private:
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_ProgressChange(unsigned ctrl_id, unsigned short position) override;
    void Msg_CheckboxChange(unsigned ctrl_id, bool checked) override;
};
