// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ctrlButton.h"
#include "ctrlGroup.h"
#include <boost/optional.hpp>
struct MouseCoords;
class Window;

/// Verwaltet eine Gruppe von Buttons, die als Optionsbuttons benötigt werden
class ctrlOptionGroup : public ctrlGroup
{
public:
    ctrlOptionGroup(Window* parent, unsigned id, GroupSelectType select_type);

    /// Selektiert einen neuen Button
    void SetSelection(unsigned selection, bool notify = false);
    /// Gibt den aktuell selektierten Button zurück
    unsigned GetSelection() const;
    // Gibt einen Button aus der Gruppe zurück zum direkten Bearbeiten
    ctrlButton* GetButton(unsigned id) { return GetCtrl<ctrlButton>(id); }

    void Msg_ButtonClick(unsigned ctrl_id) override;
    bool Msg_LeftDown(const MouseCoords& mc) override;
    bool Msg_LeftUp(const MouseCoords& mc) override;
    bool Msg_WheelUp(const MouseCoords& mc) override;
    bool Msg_WheelDown(const MouseCoords& mc) override;
    bool Msg_MouseMove(const MouseCoords& mc) override;

private:
    boost::optional<unsigned>
      selection_;                /// Currently selected button ID, must be set via SetSelection after initialization
    GroupSelectType select_type; /// Typ der Selektierung
};
