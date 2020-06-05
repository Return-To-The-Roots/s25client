// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef CTRLTABLE_H_INCLUDED
#define CTRLTABLE_H_INCLUDED

#pragma once

#include "Window.h"
#include "gameTypes/TextureColor.h"
#include <string>
#include <vector>

class MouseCoords;
class glFont;
struct KeyEvent;

enum class TableSortType
{
    String,
    MapSize,
    Number,
    Date,
    Default
};
enum class TableSortDir
{
    Ascending,
    Descending
};

struct TableColumn
{
    std::string title;
    /// Column width in relation to other cells
    unsigned short width;
    TableSortType sortType;
};
class ctrlTable : public Window
{
public:
    using Column = TableColumn;
    using SortType = TableSortType;
    using Columns = std::vector<Column>;

    ctrlTable(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const glFont* font, Columns columns);

    void Resize(const Extent& newSize) override;
    /// löscht alle Items.
    void DeleteAllItems();
    /// fügt eine Zeile hinzu.
    void AddRow(std::vector<std::string> row);
    void RemoveRow(unsigned rowIdx);
    /// liefert den Wert eines Feldes.
    const std::string& GetItemText(unsigned short row, unsigned short column) const;
    /// sortiert die Zeilen.
    void SortRows(unsigned column, TableSortDir sortDir);
    int GetSortColumn() const { return sortColumn_; }
    TableSortDir GetSortDirection() const { return sortDir_; }
    unsigned short GetNumRows() const { return static_cast<unsigned short>(rows_.size()); }
    unsigned short GetNumColumns() const { return static_cast<unsigned short>(columns_.size()); }
    int GetSelection() const { return selection_; }
    void SetSelection(int selection);

    bool Msg_LeftDown(const MouseCoords& mc) override;
    bool Msg_RightDown(const MouseCoords& mc) override;
    bool Msg_LeftUp(const MouseCoords& mc) override;
    bool Msg_WheelUp(const MouseCoords& mc) override;
    bool Msg_WheelDown(const MouseCoords& mc) override;
    bool Msg_MouseMove(const MouseCoords& mc) override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_ScrollShow(unsigned ctrl_id, bool visible) override;
    bool Msg_KeyDown(const KeyEvent& ke) override;

protected:
    void Draw_() override;

    /// Setzt die Breite und Position der Buttons ohne Scrolleiste
    void ResetButtonWidths();
    int GetSelectionFromMouse(const MouseCoords& mc);
    /// Get the area of the content (table itself w/o header)
    Rect GetContentDrawArea() const;
    /// Get the full area (element less spacing)
    Rect GetFullDrawArea() const;

private:
    TextureColor tc;
    const glFont* font;

    unsigned short header_height;
    unsigned short line_count;
    Columns columns_;

    /// Selected row. -1 for none
    int selection_;
    /// Column to sort by. -1 for none
    int sortColumn_;
    TableSortDir sortDir_;

    struct Row
    {
        std::vector<std::string> columns;
    };
    std::vector<Row> rows_;
};

#endif // !CTRLTABLE_H_INCLUDED
