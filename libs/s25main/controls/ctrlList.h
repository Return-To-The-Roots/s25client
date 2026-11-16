// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Window.h"
#include "controls/ctrlBaseTooltip.h"
#include <optional>
#include <string>
#include <vector>

struct MouseCoords;
class glFont;

class ctrlList : public Window
{
public:
    ctrlList(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
             const glFont* font);
    ~ctrlList() override;

    /// Change size
    void Resize(const Extent& newSize) override;

    /// Add item to listbox.
    void AddItem(const std::string& text);
    /// Change text of an item.
    void setItemText(unsigned id, const std::string& text);
    void DeleteAllItems();
    const std::string& getItemText(unsigned line) const;
    /// Get the value of the currently selected item
    const std::string& getSelItemText() const;
    /// Exchange the text of 2 items
    void Swap(unsigned first, unsigned second);
    /// Deletes an item. If the deleted item is selected then the selection is cleared.
    void Remove(unsigned index);

    unsigned getNumLines() const { return static_cast<unsigned>(lines.size()); }
    const std::optional<unsigned>& getSelection() const { return selection_; };
    void setSelection(const std::optional<unsigned>& selection);

    bool msgMouseMove(const MouseCoords& mc) override;
    bool msgLeftDown(const MouseCoords& mc) override;
    bool msgRightDown(const MouseCoords& mc) override;
    bool msgLeftUp(const MouseCoords& mc) override;
    bool msgWheelUp(const MouseCoords& mc) override;
    bool msgWheelDown(const MouseCoords& mc) override;

protected:
    void Draw_() override;

private:
    std::optional<unsigned> getItemFromPos(const Position& pos) const;
    Rect getFullDrawArea() const;
    Rect getListDrawArea() const;

    TextureColor tc;
    const glFont* font;
    ctrlBaseTooltip tooltip_;

    std::vector<std::string> lines;

    std::optional<unsigned> selection_;
    std::optional<unsigned> mouseover_;
    unsigned pagesize;
};
