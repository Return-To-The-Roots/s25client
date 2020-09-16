// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

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
    iwWares(unsigned id, const DrawPoint& pos, const Extent& size, const std::string& title, bool allow_outhousing,
            const glFont* font, const Inventory& inventory, const GamePlayer& player);

protected:
    /// bestimmte Inventurseite zeigen.
    virtual void SetPage(unsigned page);
    /// Add a new page and return it. ID will be in range 100+
    ctrlGroup& AddPage();

    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_PaintBefore() override;

    unsigned GetCurPage() const { return curPage_; }

private:
    unsigned curPage_; /// aktuelle Seite des Inventurfensters.
    unsigned numPages; /// maximale Seite des Inventurfensters.
};
