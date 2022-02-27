// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Window.h"
#include "ctrlList.h"
class MouseCoords;
class glFont;

class ctrlComboBox final : public Window
{
public:
    ctrlComboBox(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                 const glFont* font, unsigned short max_list_height, bool readonly);

    void Resize(const Extent& newSize) override;

    bool isReadOnly() const { return readonly; }

    void AddString(const std::string& text);
    void DeleteAllItems();

    void SetSelection(unsigned short selection);
    const boost::optional<unsigned>& GetSelection() const { return GetCtrl<ctrlList>(0)->GetSelection(); };
    unsigned short GetNumItems() const { return GetCtrl<ctrlList>(0)->GetNumLines(); }
    const std::string& GetText(unsigned short item) const { return GetCtrl<ctrlList>(0)->GetItemText(item); }
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
    void ShowList(bool show);
    Rect GetFullDrawRect(const ctrlList* list);

private:
    TextureColor tc;
    const glFont* font;
    unsigned short max_list_height;
    bool readonly;
    bool suppressSelectEvent;
};
