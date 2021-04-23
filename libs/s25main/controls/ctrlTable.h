// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Window.h"
#include "gameTypes/TextureColor.h"
#include <boost/optional.hpp>
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
    Time,
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

    ctrlTable(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
              const glFont* font, Columns columns);

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
    const boost::optional<unsigned>& GetSelection() const { return selection_; }
    void SetSelection(const boost::optional<unsigned>& selection);

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
    boost::optional<unsigned> GetSelectionFromMouse(const MouseCoords& mc) const;
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

    /// Selected row.
    boost::optional<unsigned> selection_;
    /// Column to sort by. -1 for none
    int sortColumn_;
    TableSortDir sortDir_;

    struct Row
    {
        std::vector<std::string> columns;
    };
    std::vector<Row> rows_;
};
