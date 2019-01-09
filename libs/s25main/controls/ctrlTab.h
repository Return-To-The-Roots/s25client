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
#ifndef CTRLTAB_H_INCLUDED
#define CTRLTAB_H_INCLUDED

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
    ctrlGroup* AddTab(glArchivItem_Bitmap* image, const std::string& tooltip, const unsigned id);
    /// löscht alle Tabs.
    void DeleteAllTabs();
    /// aktiviert eine bestimmte Tabseite.
    void SetSelection(unsigned short nr, bool notify = false);
    /// Gibt ID des aktuell gewählten Tabs zurück
    unsigned GetCurrentTab() const { return tabs[tab_selection]; }
    /// Gibt Tab-Group zurück, über die die Steuerelemente der Tab angesprochen werden können
    ctrlGroup* GetGroup(const unsigned tab_id);
    /// Gibt aktuell ausgewählte Tab-Gruppe zürck
    ctrlGroup* GetCurrentGroup() { return GetGroup(GetCurrentTab()); }

    void Msg_Group_ButtonClick(const unsigned group_id, const unsigned ctrl_id) override;
    void Msg_Group_EditEnter(const unsigned group_id, const unsigned ctrl_id) override;
    void Msg_Group_EditChange(const unsigned group_id, const unsigned ctrl_id) override;
    void Msg_Group_TabChange(const unsigned group_id, const unsigned ctrl_id, const unsigned short tab_id) override;
    void Msg_Group_ListSelectItem(const unsigned group_id, const unsigned ctrl_id, const int selection) override;
    void Msg_Group_ComboSelectItem(const unsigned group_id, const unsigned ctrl_id, const int selection) override;
    void Msg_Group_CheckboxChange(const unsigned group_id, const unsigned ctrl_id, const bool checked) override;
    void Msg_Group_ProgressChange(const unsigned group_id, const unsigned ctrl_id, const unsigned short position) override;
    void Msg_Group_ScrollShow(const unsigned group_id, const unsigned ctrl_id, const bool visible) override;
    void Msg_Group_OptionGroupChange(const unsigned group_id, const unsigned ctrl_id, const int selection) override;
    void Msg_Group_Timer(const unsigned group_id, const unsigned ctrl_id) override;
    void Msg_Group_TableSelectItem(const unsigned group_id, const unsigned ctrl_id, const int selection) override;
    void Msg_Group_TableRightButton(const unsigned group_id, const unsigned ctrl_id, const int selection) override;
    void Msg_Group_TableLeftButton(const unsigned group_id, const unsigned ctrl_id, const int selection) override;
    void Msg_ButtonClick(const unsigned ctrl_id) override;
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

#endif // !CTRLTAB_H_INCLUDED
