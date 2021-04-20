// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Window.h"
#include <array>

class ctrlGroup;
class MouseCoords;
class glArchivItem_Bitmap;

class ctrlTab : public Window
{
public:
    ctrlTab(Window* parent, unsigned id, const DrawPoint& pos, unsigned short width);

    /// fügt eine Tab hinzu.
    ctrlGroup* AddTab(glArchivItem_Bitmap* image, const std::string& tooltip, unsigned id);
    /// löscht alle Tabs.
    void DeleteAllTabs();
    /// aktiviert eine bestimmte Tabseite.
    void SetSelection(unsigned short nr, bool notify = false);
    /// Gibt ID des aktuell gewählten Tabs zurück
    unsigned GetCurrentTab() const { return tabs[tab_selection]; }
    /// Gibt Tab-Group zurück, über die die Steuerelemente der Tab angesprochen werden können
    ctrlGroup* GetGroup(unsigned tab_id);
    /// Gibt aktuell ausgewählte Tab-Gruppe zürck
    ctrlGroup* GetCurrentGroup() { return GetGroup(GetCurrentTab()); }

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
    void Msg_ButtonClick(unsigned ctrl_id) override;
    bool Msg_LeftDown(const MouseCoords& mc) override;
    bool Msg_LeftUp(const MouseCoords& mc) override;
    bool Msg_WheelUp(const MouseCoords& mc) override;
    bool Msg_WheelDown(const MouseCoords& mc) override;
    bool Msg_MouseMove(const MouseCoords& mc) override;

protected:
    void Draw_() override;

private:
    unsigned short tab_count;
    unsigned short tab_selection;

    std::array<unsigned, 20> tabs; //-V730_NOINIT
};
