// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef CTRLOPTIONGROUP_H_INCLUDED
#define CTRLOPTIONGROUP_H_INCLUDED

#pragma once

#include "ctrlGroup.h"
#include "ctrlButton.h"
class MouseCoords;
class Window;

/// Verwaltet eine Gruppe von Buttons, die als Optionsbuttons benötigt werden
class ctrlOptionGroup : public ctrlGroup
{
    public:
        enum
        {
            ILLUMINATE = 0,
            CHECK,
            SHOW
        };

    public:
        ctrlOptionGroup(Window* parent, unsigned int id, int select_type, bool scale = false);

        /// Selektiert einen neuen Button
        void SetSelection(unsigned short selection, bool notify = false);
        /// Gibt den aktuell selektierten Button zurück
        unsigned short GetSelection() const { return selection_; }
        // Gibt einen Button aus der Gruppe zurück zum direkten Bearbeiten
        ctrlButton* GetButton(unsigned int id) { return GetCtrl<ctrlButton>(id); }

        void Msg_ButtonClick(const unsigned int ctrl_id) override;
        bool Msg_LeftDown(const MouseCoords& mc) override;
        bool Msg_LeftUp(const MouseCoords& mc) override;
        bool Msg_WheelUp(const MouseCoords& mc) override;
        bool Msg_WheelDown(const MouseCoords& mc) override;
        bool Msg_MouseMove(const MouseCoords& mc) override;

    protected:
        /// Zeichenmethode.
        bool Draw_() override;

    private:
        unsigned short selection_; /// aktuell ausgewählter Button ( @p 0xFFFF = nicht selektiert )
        int select_type;         /// Typ der Selektierung
};

#endif // !CTRLOPTIONGROUP_H_INCLUDED
