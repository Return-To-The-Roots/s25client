// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "desktops/dskMenuBase.h"

class dskDirectIP : public dskMenuBase
{
public:
    dskDirectIP();

private:
    void Msg_ButtonClick(unsigned ctrl_id) override;
};
