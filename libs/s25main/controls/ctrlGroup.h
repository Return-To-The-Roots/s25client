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
class MouseCoords;
struct KeyEvent;

class ctrlGroup : public Window
{
public:
    ctrlGroup(Window* parent, unsigned id);

    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_EditEnter(unsigned ctrl_id) override;
    void Msg_EditChange(unsigned ctrl_id) override;
    void Msg_TabChange(unsigned ctrl_id, unsigned short tab_id) override;
    void Msg_ListSelectItem(unsigned ctrl_id, int selection) override;
    void Msg_ComboSelectItem(unsigned ctrl_id, unsigned selection) override;
    void Msg_CheckboxChange(unsigned ctrl_id, bool checked) override;
    void Msg_ProgressChange(unsigned ctrl_id, unsigned short position) override;
    void Msg_ScrollShow(unsigned ctrl_id, bool visible) override;
    void Msg_OptionGroupChange(unsigned ctrl_id, unsigned selection) override;
    void Msg_Timer(unsigned ctrl_id) override;
    void Msg_TableSelectItem(unsigned ctrl_id, const boost::optional<unsigned>& selection) override;
    void Msg_TableRightButton(unsigned ctrl_id, const boost::optional<unsigned>& selection) override;
    void Msg_TableLeftButton(unsigned ctrl_id, const boost::optional<unsigned>& selection) override;

    void Msg_Group_ButtonClick(unsigned group_id, unsigned ctrl_id) override;
    void Msg_Group_EditEnter(unsigned group_id, unsigned ctrl_id) override;
    void Msg_Group_EditChange(unsigned group_id, unsigned ctrl_id) override;
    void Msg_Group_TabChange(unsigned group_id, unsigned ctrl_id, unsigned short tab_id) override;
    void Msg_Group_ListSelectItem(unsigned group_id, unsigned ctrl_id, int selection) override;
    void Msg_Group_ComboSelectItem(unsigned group_id, unsigned ctrl_id, unsigned selection) override;
    void Msg_Group_CheckboxChange(unsigned group_id, unsigned ctrl_id, bool checked) override;
    void Msg_Group_ProgressChange(unsigned group_id, unsigned ctrl_id, unsigned short position) override;
    void Msg_Group_ScrollShow(unsigned group_id, unsigned ctrl_id, bool visible) override;
    void Msg_Group_OptionGroupChange(unsigned group_id, unsigned ctrl_id, unsigned selection) override;
    void Msg_Group_Timer(unsigned group_id, unsigned ctrl_id) override;
    void Msg_Group_TableSelectItem(unsigned group_id, unsigned ctrl_id, const boost::optional<unsigned>& selection) override;
    void Msg_Group_TableRightButton(unsigned group_id, unsigned ctrl_id, const boost::optional<unsigned>& selection) override;
    void Msg_Group_TableLeftButton(unsigned group_id, unsigned ctrl_id, const boost::optional<unsigned>& selection) override;

    bool Msg_LeftDown(const MouseCoords& mc) override;
    bool Msg_RightDown(const MouseCoords& mc) override;
    bool Msg_LeftUp(const MouseCoords& mc) override;
    bool Msg_RightUp(const MouseCoords& mc) override;
    bool Msg_WheelUp(const MouseCoords& mc) override;
    bool Msg_WheelDown(const MouseCoords& mc) override;
    bool Msg_MouseMove(const MouseCoords& mc) override;
    bool Msg_KeyDown(const KeyEvent& ke) override;
};
