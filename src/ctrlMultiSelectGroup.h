// $Id: ctrlMultiSelectGroup.h 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef CTRLMULTISELECTGROUP_H_INCLUDED
#define CTRLMULTISELECTGROUP_H_INCLUDED

#pragma once

#include "ctrlGroup.h"
#include "ctrlButton.h"
#include <set>

/// Verwaltet eine Gruppe von n Buttons, von denen 0 bis n gleichzeitig ausgewählt sind
class ctrlMultiSelectGroup : public ctrlGroup
{
    public:
        enum
        {
            ILLUMINATE = 0,
            CHECK,
            SHOW
        };

    public:
        /// Konstruktor von @p ctrlMultiSelectGroup.
        ctrlMultiSelectGroup(Window* parent, unsigned int id, int select_type, bool scale = false);

        /// Selektiert einen neuen Button
        void AddSelection(unsigned short selection, bool notify = false);
        /// Entfernt einen selektierten Button aus der Selektion
        void RemoveSelection(unsigned short selection, bool notify = false);
        /// Wechselt zwischen selektiert/nicht selektiert
        void ToggleSelection(unsigned short selection, bool notify = false);
        /// Gibt Liste der aktuell selektierten Buttons zurück
        const std::set<unsigned short> &GetSelection() const { return selection; }
        /// Prüft ob ein Button ausgewählt ist
        bool IsSelected(unsigned short selection) const;
        // Gibt einen Button aus der Gruppe zurück zum direkten Bearbeiten
        ctrlButton* GetButton(unsigned int id) { return GetCtrl<ctrlButton>(id); }

        virtual void Msg_ButtonClick(const unsigned int ctrl_id);
        virtual bool Msg_LeftDown(const MouseCoords& mc);
        virtual bool Msg_LeftUp(const MouseCoords& mc);
        virtual bool Msg_WheelUp(const MouseCoords& mc);
        virtual bool Msg_WheelDown(const MouseCoords& mc);
        virtual bool Msg_MouseMove(const MouseCoords& mc);

    protected:
        /// Zeichenmethode.
        virtual bool Draw_(void);

    private:
        std::set<unsigned short> selection; ///< aktuell ausgewählte Buttons
        int select_type;         ///< Typ der Selektierung
};

#endif // !CTRLMULTISELECTGROUP_H_INCLUDED
