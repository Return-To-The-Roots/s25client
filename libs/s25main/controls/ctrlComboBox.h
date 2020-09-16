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

    void AddString(const std::string& text);
    void DeleteAllItems();

    void SetSelection(unsigned short selection);
    const boost::optional<unsigned>& GetSelection() const { return GetCtrl<ctrlList>(0)->GetSelection(); };
    unsigned short GetNumItems() const { return GetCtrl<ctrlList>(0)->GetNumLines(); }
    const std::string& GetText(unsigned short item) const { return GetCtrl<ctrlList>(0)->GetItemText(item); }

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
