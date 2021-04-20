// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Desktop.h"

///  Klasse des Intro Desktops.
class dskIntro : public Desktop
{
public:
    dskIntro();

private:
    void Msg_ButtonClick(unsigned ctrl_id) override;
};
