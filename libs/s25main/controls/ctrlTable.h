// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Window.h"
#include "gameTypes/TextureColor.h"
#include <optional>
#include <string>
#include <vector>

struct MouseCoords;
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
    const std::string& getItemText(unsigned short row, unsigned short column) const;
    /// sortiert die Zeilen.
    void SortRows(unsigned column, TableSortDir sortDir);
    int getSortColumn() const { return sortColumn_; }
    TableSortDir getSortDirection() const { return sortDir_; }
    unsigned short getNumRows() const { return static_cast<unsigned short>(rows_.size()); }
    unsigned short getNumColumns() const { return static_cast<unsigned short>(columns_.size()); }
    const std::optional<unsigned>& getSelection() const { return selection_; }
    void setSelection(const std::optional<unsigned>& selection);

    bool msgLeftDown(const MouseCoords& mc) override;
    bool msgRightDown(const MouseCoords& mc) override;
    bool msgLeftUp(const MouseCoords& mc) override;
    bool msgWheelUp(const MouseCoords& mc) override;
    bool msgWheelDown(const MouseCoords& mc) override;
    bool msgMouseMove(const MouseCoords& mc) override;
    void msgButtonClick(unsigned ctrl_id) override;
    void msgScrollShow(unsigned ctrl_id, bool visible) override;
    bool msgKeyDown(const KeyEvent& ke) override;

protected:
    void Draw_() override;

    /// Setzt die Breite und Position der Buttons ohne Scrolleiste
    void ResetButtonWidths();
    std::optional<unsigned> getSelectionFromMouse(const MouseCoords& mc) const;
    /// Get the area of the content (table itself w/o header)
    Rect getContentDrawArea() const;
    /// Get the full area (element less spacing)
    Rect getFullDrawArea() const;

private:
    TextureColor tc;
    const glFont* font;

    unsigned short header_height;
    unsigned short line_count;
    Columns columns_;

    /// Selected row.
    std::optional<unsigned> selection_;
    /// Column to sort by. -1 for none
    int sortColumn_;
    TableSortDir sortDir_;

    struct Row
    {
        std::vector<std::string> columns;
    };
    std::vector<Row> rows_;
};
