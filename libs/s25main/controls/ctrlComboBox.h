// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Window.h"
#include "ctrlList.h"
struct MouseCoords;
class glFont;

class ctrlComboBox final : public Window
{
public:
    ctrlComboBox(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                 const glFont* font, unsigned short max_list_height, bool readonly);

    void Resize(const Extent& newSize) override;

    bool isReadOnly() const { return readonly; }

    void AddItem(const std::string& text);
    void DeleteAllItems();
    /// Set selection to an item if within bounds. Does not trigger a notification.
    void setSelection(unsigned selection);
    const std::optional<unsigned>& getSelection() const { return getCtrl<ctrlList>(0)->getSelection(); };
    unsigned getNumItems() const { return getCtrl<ctrlList>(0)->getNumLines(); }
    const std::string& getText(unsigned item) const { return getCtrl<ctrlList>(0)->getItemText(item); }
    void setText(unsigned item, const std::string& text) { getCtrl<ctrlList>(0)->setItemText(item, text); }
    std::optional<std::string> getSelectedText() const;

    void msgPaintAfter() override;
    bool msgMouseMove(const MouseCoords& mc) override;
    bool msgLeftDown(const MouseCoords& mc) override;
    bool msgLeftUp(const MouseCoords& mc) override;
    bool msgRightDown(const MouseCoords& mc) override;
    bool msgWheelUp(const MouseCoords& mc) override;
    bool msgWheelDown(const MouseCoords& mc) override;

    void msgListSelectItem(unsigned ctrl_id, int selection) override;

protected:
    void Draw_() override;
    /// Show or hide the list.
    void ShowList(bool show);
    Rect getFullDrawRect(const ctrlList* list);

private:
    TextureColor tc;
    const glFont* font;
    unsigned short max_list_height;
    bool readonly;
    bool suppressSelectEvent;
};
