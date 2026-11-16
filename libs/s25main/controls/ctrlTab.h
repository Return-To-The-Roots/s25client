// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Window.h"
#include <array>

class ctrlGroup;
struct MouseCoords;
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
    void setSelection(unsigned short nr, bool notify = false);
    /// Gibt ID des aktuell gewählten Tabs zurück
    unsigned getCurrentTab() const { return tabs[tab_selection]; }
    /// Gibt Tab-Group zurück, über die die Steuerelemente der Tab angesprochen werden können
    ctrlGroup* getGroup(unsigned tab_id);
    /// Gibt aktuell ausgewählte Tab-Gruppe zürck
    ctrlGroup* getCurrentGroup() { return getGroup(getCurrentTab()); }

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
    void msgButtonClick(unsigned ctrl_id) override;
    bool msgLeftDown(const MouseCoords& mc) override;
    bool msgLeftUp(const MouseCoords& mc) override;
    bool msgWheelUp(const MouseCoords& mc) override;
    bool msgWheelDown(const MouseCoords& mc) override;
    bool msgMouseMove(const MouseCoords& mc) override;

protected:
    void Draw_() override;

private:
    unsigned short tab_count;
    unsigned short tab_selection;

    std::array<unsigned, 20> tabs; //-V730_NOINIT
};
