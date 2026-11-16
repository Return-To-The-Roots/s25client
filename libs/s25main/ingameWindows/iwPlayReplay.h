// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"

/// Klasse des Replay-Listen-Fensters.
class iwPlayReplay : public IngameWindow
{
public:
    iwPlayReplay();

    void msgButtonClick(unsigned ctrl_id) override;
    void msgMsgBoxResult(unsigned msgbox_id, MsgboxResult mbr) override;
    void msgTableChooseItem(unsigned ctrl_id, unsigned selection) override;

private:
    void PopulateTable();

    /// Startet das Replay (aktuell ausgewählter Eintrag)
    void StartReplay();
};
