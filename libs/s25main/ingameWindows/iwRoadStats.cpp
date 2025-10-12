// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwRoadStats.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "gameData/const_gui_ids.h"

iwRoadStats::iwRoadStats(const GamePlayer& player, const noFlag* flag)
    : iwWares(CGI_INVENTORY, IngameWindow::posLastOrCenter, 0, _("Road stats"), false, SmallFont, flag->GetInventory(),
              player)
{}
