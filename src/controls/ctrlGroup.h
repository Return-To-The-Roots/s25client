// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef CTRLGROUP_H_INCLUDED
#define CTRLGROUP_H_INCLUDED

#pragma once

#include "Window.h"
class MouseCoords;
struct KeyEvent;
struct ScreenResizeEvent;

class ctrlGroup : public Window
{
    public:
        ctrlGroup(Window* parent, unsigned int id, bool scale = false);

        void Msg_ScreenResize(const ScreenResizeEvent& sr) override;

        void Msg_ButtonClick(const unsigned int ctrl_id) override;
        void Msg_EditEnter(const unsigned int ctrl_id) override;
        void Msg_EditChange(const unsigned int ctrl_id) override;
        void Msg_TabChange(const unsigned int ctrl_id, const unsigned short tab_id) override;
        void Msg_ListSelectItem(const unsigned int ctrl_id, const int selection) override;
        void Msg_ComboSelectItem(const unsigned int ctrl_id, const int selection) override;
        void Msg_CheckboxChange(const unsigned int ctrl_id, const bool checked) override;
        void Msg_ProgressChange(const unsigned int ctrl_id, const unsigned short position) override;
        void Msg_ScrollShow(const unsigned int ctrl_id, const bool visible) override;
        void Msg_OptionGroupChange(const unsigned int ctrl_id, const int selection) override;
        void Msg_Timer(const unsigned int ctrl_id) override;
        void Msg_TableSelectItem(const unsigned int ctrl_id, const int selection) override;
        void Msg_TableRightButton(const unsigned int ctrl_id, const int selection) override;
        void Msg_TableLeftButton(const unsigned int ctrl_id, const int selection) override;

        void Msg_Group_ButtonClick(const unsigned int group_id, const unsigned int ctrl_id) override;
        void Msg_Group_EditEnter(const unsigned int group_id, const unsigned int ctrl_id) override;
        void Msg_Group_EditChange(const unsigned int group_id, const unsigned int ctrl_id) override;
        void Msg_Group_TabChange(const unsigned int group_id, const unsigned int ctrl_id, const unsigned short tab_id) override;
        void Msg_Group_ListSelectItem(const unsigned int group_id, const unsigned int ctrl_id, const int selection) override;
        void Msg_Group_ComboSelectItem(const unsigned int group_id, const unsigned int ctrl_id, const int selection) override;
        void Msg_Group_CheckboxChange(const unsigned int group_id, const unsigned int ctrl_id, const bool checked) override;
        void Msg_Group_ProgressChange(const unsigned int group_id, const unsigned int ctrl_id, const unsigned short position) override;
        void Msg_Group_ScrollShow(const unsigned int group_id, const unsigned int ctrl_id, const bool visible) override;
        void Msg_Group_OptionGroupChange(const unsigned int group_id, const unsigned int ctrl_id, const int selection) override;
        void Msg_Group_Timer(const unsigned int group_id, const unsigned int ctrl_id) override;
        void Msg_Group_TableSelectItem(const unsigned int group_id, const unsigned int ctrl_id, const int selection) override;
        void Msg_Group_TableRightButton(const unsigned int group_id, const unsigned int ctrl_id, const int selection) override;
        void Msg_Group_TableLeftButton(const unsigned int group_id, const unsigned int ctrl_id, const int selection) override;

        bool Msg_LeftDown(const MouseCoords& mc) override;
        bool Msg_RightDown(const MouseCoords& mc) override;
        bool Msg_LeftUp(const MouseCoords& mc) override;
        bool Msg_RightUp(const MouseCoords& mc) override;
        bool Msg_WheelUp(const MouseCoords& mc) override;
        bool Msg_WheelDown(const MouseCoords& mc) override;
        bool Msg_MouseMove(const MouseCoords& mc) override;
        bool Msg_KeyDown(const KeyEvent& ke) override;

    protected:
        bool Draw_() override;
};

#endif // !CTRLGROUP_H_INCLUDED
