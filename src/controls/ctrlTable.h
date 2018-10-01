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
#ifndef CTRLTABLE_H_INCLUDED
#define CTRLTABLE_H_INCLUDED

#pragma once

#include "Window.h"
#include "gameTypes/TextureColor.h"
#include <cstdarg>
#include <string>
#include <vector>

class MouseCoords;
class glArchivItem_Font;
struct KeyEvent;

class ctrlTable : public Window
{
public:
    ctrlTable(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, glArchivItem_Font* font,
              unsigned short column_count, va_list liste);
    ~ctrlTable() override;

    void Resize(const Extent& newSize) override;
    /// löscht alle Items.
    void DeleteAllItems();
    /// fügt eine Zeile hinzu.
    void AddRow(unsigned alwaysnull, ...);
    void RemoveRow(unsigned rowIdx);
    /// liefert den Wert eines Feldes.
    const std::string& GetItemText(unsigned short row, unsigned short column) const;
    /// sortiert die Zeilen.
    void SortRows(int column, bool* direction = NULL);
    int GetSortColumn() const { return sort_column; }
    bool GetSortDirection() const { return sort_direction; }
    unsigned short GetNumRows() const { return static_cast<unsigned short>(rows.size()); }
    unsigned short GetNumColumns() const { return static_cast<unsigned short>(columns.size()); }
    int GetSelection() const { return selection_; }
    void SetSelection(int selection);

    bool Msg_LeftDown(const MouseCoords& mc) override;
    bool Msg_RightDown(const MouseCoords& mc) override;
    bool Msg_LeftUp(const MouseCoords& mc) override;
    bool Msg_WheelUp(const MouseCoords& mc) override;
    bool Msg_WheelDown(const MouseCoords& mc) override;
    bool Msg_MouseMove(const MouseCoords& mc) override;
    void Msg_ButtonClick(const unsigned ctrl_id) override;
    void Msg_ScrollShow(const unsigned ctrl_id, const bool visible) override;
    bool Msg_KeyDown(const KeyEvent& ke) override;

    enum SortType
    {
        SRT_STRING,
        SRT_MAPSIZE,
        SRT_NUMBER,
        SRT_DATE,
        SRT_DEFAULT
    };

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
    glArchivItem_Font* font;

    unsigned short header_height;
    unsigned short line_count;

    struct COLUMN
    {
        /// Breite der Spalten in Promille von der Tabellenlänge
        unsigned short width;
        std::string title;
        SortType sortType;
    };
    std::vector<COLUMN> columns;

    /// Selected row. -1 for none
    int selection_;
    /// Column to sort by. -1 for none
    int sort_column;
    bool sort_direction;

    struct ROW
    {
        std::vector<std::string> columns;
    };
    std::vector<ROW> rows;

    int Compare(const std::string& a, const std::string& b, SortType sortType);
};

#endif // !CTRLTABLE_H_INCLUDED
