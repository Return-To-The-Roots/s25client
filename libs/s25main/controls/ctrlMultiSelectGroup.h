// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ctrlButton.h"
#include "ctrlGroup.h"
#include <set>
struct MouseCoords;
class Window;

/// Verwaltet eine Gruppe von n Buttons, von denen 0 bis n gleichzeitig ausgewählt sind
class ctrlMultiSelectGroup : public ctrlGroup
{
public:
    ctrlMultiSelectGroup(Window* parent, unsigned id, GroupSelectType select_type);

    /// Selektiert einen neuen Button
    void AddSelection(unsigned selection, bool notify = false);
    /// Entfernt einen selektierten Button aus der Selektion
    void RemoveSelection(unsigned selection, bool notify = false);
    /// Wechselt zwischen selektiert/nicht selektiert
    void ToggleSelection(unsigned selection, bool notify = false);
    /// Gibt Liste der aktuell selektierten Buttons zurück
    const std::set<unsigned>& GetSelection() const { return selectedItems_; }
    /// Prüft ob ein Button ausgewählt ist
    bool IsSelected(unsigned selection) const;
    // Gibt einen Button aus der Gruppe zurück zum direkten Bearbeiten
    ctrlButton* GetButton(unsigned id) { return GetCtrl<ctrlButton>(id); }

    void Msg_ButtonClick(unsigned ctrl_id) override;
    bool Msg_LeftDown(const MouseCoords& mc) override;
    bool Msg_LeftUp(const MouseCoords& mc) override;
    bool Msg_WheelUp(const MouseCoords& mc) override;
    bool Msg_WheelDown(const MouseCoords& mc) override;
    bool Msg_MouseMove(const MouseCoords& mc) override;

private:
    std::set<unsigned> selectedItems_; /// aktuell ausgewählte Buttons
    GroupSelectType select_type;       /// Typ der Selektierung
};
