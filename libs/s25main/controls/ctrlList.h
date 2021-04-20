// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Window.h"
#include "controls/ctrlBaseTooltip.h"
#include <boost/optional.hpp>
#include <string>
#include <vector>

class MouseCoords;
class glFont;

class ctrlList : public Window
{
public:
    ctrlList(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
             const glFont* font);
    ~ctrlList() override;

    /// Größe verändern
    void Resize(const Extent& newSize) override;

    /// Neuen String zur Listbox hinzufügen.
    void AddString(const std::string& text);
    /// Verändert einen String
    void SetString(const std::string& text, unsigned id);
    /// Listbox leeren.
    void DeleteAllItems();
    /// liefert den Wert einer Zeile.
    const std::string& GetItemText(unsigned short line) const;
    /// liefert den Wert der aktuell gewählten Zeile.
    const std::string& GetSelItemText() const;
    /// Vertauscht zwei Zeilen.
    void Swap(unsigned first, unsigned second);
    /// Löscht ein Element
    void Remove(unsigned short index);

    unsigned short GetNumLines() const { return static_cast<unsigned short>(lines.size()); }
    const boost::optional<unsigned>& GetSelection() const { return selection_; };
    void SetSelection(const boost::optional<unsigned>& selection);

    bool Msg_MouseMove(const MouseCoords& mc) override;
    bool Msg_LeftDown(const MouseCoords& mc) override;
    bool Msg_RightDown(const MouseCoords& mc) override;
    bool Msg_LeftUp(const MouseCoords& mc) override;
    bool Msg_WheelUp(const MouseCoords& mc) override;
    bool Msg_WheelDown(const MouseCoords& mc) override;

protected:
    /// Zeichenmethode.
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
