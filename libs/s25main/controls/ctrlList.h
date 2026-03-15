// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Window.h"
#include "controls/ctrlBaseTooltip.h"
#include <boost/optional.hpp>
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
    void SetItemText(unsigned id, const std::string& text);
    void DeleteAllItems();
    const std::string& GetItemText(unsigned line) const;
    /// Get the value of the currently selected item
    const std::string& GetSelItemText() const;
    /// Exchange the text of 2 items
    void Swap(unsigned first, unsigned second);
    /// Deletes an item. If the deleted item is selected then the selection is cleared.
    void Remove(unsigned index);

    unsigned GetNumLines() const { return static_cast<unsigned>(lines.size()); }
    const boost::optional<unsigned>& GetSelection() const { return selection_; };
    void SetSelection(const boost::optional<unsigned>& selection);

    bool Msg_MouseMove(const MouseCoords& mc) override;
    bool Msg_LeftDown(const MouseCoords& mc) override;
    bool Msg_RightDown(const MouseCoords& mc) override;
    bool Msg_LeftUp(const MouseCoords& mc) override;
    bool Msg_WheelUp(const MouseCoords& mc) override;
    bool Msg_WheelDown(const MouseCoords& mc) override;

protected:
    void Draw_() override;

private:
    boost::optional<unsigned> GetItemFromPos(const Position& pos) const;
    Rect GetFullDrawArea() const;
    Rect GetListDrawArea() const;

    TextureColor tc;
    const glFont* font;
    ctrlBaseTooltip tooltip_;

    std::vector<std::string> lines;

    boost::optional<unsigned> selection_;
    boost::optional<unsigned> mouseover_;
    unsigned pagesize;
};
