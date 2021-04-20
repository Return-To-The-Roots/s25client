// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"

/// Bitte-Warten Fenster, welches aufgerufen wird, nachdem der Server gestartet wurde bis zum Weitergehen
class iwPleaseWait : public IngameWindow
{
public:
    iwPleaseWait();
    ~iwPleaseWait() override;
};
