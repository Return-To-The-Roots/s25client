// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"

class glFont;
struct Inventory;
class GamePlayer;

class iwWares : public IngameWindow
{
protected:
    const Inventory& inventory; /// Warenbestand
    const GamePlayer& player;
    unsigned warePageID, peoplePageID;

public:
    iwWares(unsigned id, const DrawPoint& pos, unsigned additionalYSpace, const std::string& title,
            bool allow_outhousing, const glFont* font, const Inventory& inventory, const GamePlayer& player);

protected:
    /// bestimmte Inventurseite zeigen.
    virtual void setPage(unsigned page);
    /// Add a new page and return it. ID will be in range 100+
    ctrlGroup& AddPage();

    void msgButtonClick(unsigned ctrl_id) override;
    void msgPaintBefore() override;

    unsigned getCurPage() const { return curPage_; }

private:
    unsigned curPage_; /// aktuelle Seite des Inventurfensters.
    unsigned numPages; /// maximale Seite des Inventurfensters.
};
