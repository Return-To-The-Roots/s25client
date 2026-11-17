// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ctrlButton.h"
#include "ctrlGroup.h"
#include <optional>
struct MouseCoords;
class Window;

/// Verwaltet eine Gruppe von Buttons, die als Optionsbuttons benötigt werden
class ctrlOptionGroup : public ctrlGroup
{
public:
    ctrlOptionGroup(Window* parent, unsigned id, GroupSelectType select_type);

    /// Selektiert einen neuen Button
    void setSelection(unsigned selection, bool notify = false);
    /// Gibt den aktuell selektierten Button zurück
    unsigned getSelection() const;
    // Gibt einen Button aus der Gruppe zurück zum direkten Bearbeiten
    ctrlButton* getButton(unsigned id) { return getCtrl<ctrlButton>(id); }

    void msgButtonClick(unsigned ctrl_id) override;
    bool msgLeftDown(const MouseCoords& mc) override;
    bool msgLeftUp(const MouseCoords& mc) override;
    bool msgWheelUp(const MouseCoords& mc) override;
    bool msgWheelDown(const MouseCoords& mc) override;
    bool msgMouseMove(const MouseCoords& mc) override;

private:
    std::optional<unsigned>
      selection_;                /// Currently selected button ID, must be set via setSelection after initialization
    GroupSelectType select_type; /// Typ der Selektierung
};
