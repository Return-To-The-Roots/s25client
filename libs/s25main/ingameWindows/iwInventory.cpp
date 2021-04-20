// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwInventory.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "gameData/const_gui_ids.h"

iwInventory::iwInventory(const GamePlayer& player)
    : iwWares(CGI_INVENTORY, IngameWindow::posLastOrCenter, Extent(167, 376), _("Stock"), false, SmallFont,
              player.GetInventory(), player)
{}
