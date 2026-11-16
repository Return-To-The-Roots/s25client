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

    void msgButtonClick(unsigned ctrl_id) override;
    void msgEditEnter(unsigned ctrl_id) override;
    void msgEditChange(unsigned ctrl_id) override;
    void msgTabChange(unsigned ctrl_id, unsigned short tab_id) override;
    void msgListSelectItem(unsigned ctrl_id, int selection) override;
    void msgComboSelectItem(unsigned ctrl_id, unsigned selection) override;
    void msgCheckboxChange(unsigned ctrl_id, bool checked) override;
    void msgProgressChange(unsigned ctrl_id, unsigned short position) override;
    void msgScrollShow(unsigned ctrl_id, bool visible) override;
    void msgOptionGroupChange(unsigned ctrl_id, unsigned selection) override;
    void msgTimer(unsigned ctrl_id) override;
    void msgTableSelectItem(unsigned ctrl_id, const std::optional<unsigned>& selection) override;
    void msgTableRightButton(unsigned ctrl_id, const std::optional<unsigned>& selection) override;
    void msgTableLeftButton(unsigned ctrl_id, const std::optional<unsigned>& selection) override;

    void msgGroupButtonClick(unsigned group_id, unsigned ctrl_id) override;
    void msgGroupEditEnter(unsigned group_id, unsigned ctrl_id) override;
    void msgGroupEditChange(unsigned group_id, unsigned ctrl_id) override;
    void msgGroupTabChange(unsigned group_id, unsigned ctrl_id, unsigned short tab_id) override;
    void msgGroupListSelectItem(unsigned group_id, unsigned ctrl_id, int selection) override;
    void msgGroupComboSelectItem(unsigned group_id, unsigned ctrl_id, unsigned selection) override;
    void msgGroupCheckboxChange(unsigned group_id, unsigned ctrl_id, bool checked) override;
    void msgGroupProgressChange(unsigned group_id, unsigned ctrl_id, unsigned short position) override;
    void msgGroupScrollShow(unsigned group_id, unsigned ctrl_id, bool visible) override;
    void msgGroupOptionGroupChange(unsigned group_id, unsigned ctrl_id, unsigned selection) override;
    void msgGroupTimer(unsigned group_id, unsigned ctrl_id) override;
    void msgGroupTableSelectItem(unsigned group_id, unsigned ctrl_id,
                                 const std::optional<unsigned>& selection) override;
    void msgGroupTableRightButton(unsigned group_id, unsigned ctrl_id,
                                  const std::optional<unsigned>& selection) override;
    void msgGroupTableLeftButton(unsigned group_id, unsigned ctrl_id,
                                 const std::optional<unsigned>& selection) override;

    bool msgLeftDown(const MouseCoords& mc) override;
    bool msgRightDown(const MouseCoords& mc) override;
    bool msgLeftUp(const MouseCoords& mc) override;
    bool msgRightUp(const MouseCoords& mc) override;
    bool msgWheelUp(const MouseCoords& mc) override;
    bool msgWheelDown(const MouseCoords& mc) override;
    bool msgMouseMove(const MouseCoords& mc) override;
    bool msgKeyDown(const KeyEvent& ke) override;
};
