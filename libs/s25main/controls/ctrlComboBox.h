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
    void SetSelection(unsigned selection);
    const boost::optional<unsigned>& GetSelection() const { return GetCtrl<ctrlList>(0)->GetSelection(); };
    unsigned GetNumItems() const { return GetCtrl<ctrlList>(0)->GetNumLines(); }
    const std::string& GetText(unsigned item) const { return GetCtrl<ctrlList>(0)->GetItemText(item); }
    void SetText(unsigned item, const std::string& text) { GetCtrl<ctrlList>(0)->SetItemText(item, text); }
    boost::optional<std::string> GetSelectedText() const;

    void Msg_PaintAfter() override;
    bool Msg_MouseMove(const MouseCoords& mc) override;
    bool Msg_LeftDown(const MouseCoords& mc) override;
    bool Msg_LeftUp(const MouseCoords& mc) override;
    bool Msg_RightDown(const MouseCoords& mc) override;
    bool Msg_WheelUp(const MouseCoords& mc) override;
    bool Msg_WheelDown(const MouseCoords& mc) override;

    void Msg_ListSelectItem(unsigned ctrl_id, int selection) override;

protected:
    void Draw_() override;
    /// Show or hide the list.
    void ShowList(bool show);
    Rect GetFullDrawRect(const ctrlList* list);

private:
    TextureColor tc;
    const glFont* font;
    unsigned short max_list_height;
    bool readonly;
    bool suppressSelectEvent;
};
