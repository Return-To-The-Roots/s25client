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
    const std::set<unsigned>& getSelection() const { return selectedItems_; }
    /// Prüft ob ein Button ausgewählt ist
    bool isSelected(unsigned selection) const;
    // Gibt einen Button aus der Gruppe zurück zum direkten Bearbeiten
    ctrlButton* getButton(unsigned id) { return getCtrl<ctrlButton>(id); }

    void msgButtonClick(unsigned ctrl_id) override;
    bool msgLeftDown(const MouseCoords& mc) override;
    bool msgLeftUp(const MouseCoords& mc) override;
    bool msgWheelUp(const MouseCoords& mc) override;
    bool msgWheelDown(const MouseCoords& mc) override;
    bool msgMouseMove(const MouseCoords& mc) override;

private:
    std::set<unsigned> selectedItems_; /// aktuell ausgewählte Buttons
    GroupSelectType select_type;       /// Typ der Selektierung
};
