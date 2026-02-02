// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Window.h"
struct MouseCoords;
struct KeyEvent;

enum class GroupSelectType : unsigned
{
    Illuminate,
    Check,
    Show
};

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
    void Msg_Group_TableSelectItem(unsigned group_id, unsigned ctrl_id,
                                   const boost::optional<unsigned>& selection) override;
    void Msg_Group_TableRightButton(unsigned group_id, unsigned ctrl_id,
                                    const boost::optional<unsigned>& selection) override;
    void Msg_Group_TableLeftButton(unsigned group_id, unsigned ctrl_id,
                                   const boost::optional<unsigned>& selection) override;

    bool Msg_LeftDown(const MouseCoords& mc) override;
    bool Msg_RightDown(const MouseCoords& mc) override;
    bool Msg_LeftUp(const MouseCoords& mc) override;
    bool Msg_RightUp(const MouseCoords& mc) override;
    bool Msg_WheelUp(const MouseCoords& mc) override;
    bool Msg_WheelDown(const MouseCoords& mc) override;
    bool Msg_MouseMove(const MouseCoords& mc) override;
    bool Msg_KeyDown(const KeyEvent& ke) override;
};
