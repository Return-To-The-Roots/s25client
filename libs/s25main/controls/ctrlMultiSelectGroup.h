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

#include "ctrlButton.h"
#include "ctrlGroup.h"
#include <set>
class MouseCoords;
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
