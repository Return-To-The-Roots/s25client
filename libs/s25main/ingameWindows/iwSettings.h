// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"

struct VideoMode;

class iwSettings : public IngameWindow
{
public:
    iwSettings();
    ~iwSettings() override;

private:
    std::vector<VideoMode> video_modes; /// Vector für die Auflösungen
    void Msg_OptionGroupChange(unsigned ctrl_id, unsigned selection) override;
    void Msg_CheckboxChange(unsigned ctrl_id, bool checked) override;
};
