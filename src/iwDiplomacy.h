// $Id: iwDiplomacy.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef iwDIPLOMACY_H_INCLUDED
#define iwDIPLOMACY_H_INCLUDED


#include "IngameWindow.h"

/// Diplomatiefenster: Übersicht über alle Spieler im Spiel und Schmieden von Bündnissen
class iwDiplomacy : public IngameWindow
{
    public:

        iwDiplomacy();

    private:

        void Msg_PaintBefore();
        void Msg_PaintAfter();
        void Msg_ButtonClick(const unsigned int ctrl_id);
};

/// Kleines Fenster, über das einem Spieler ein neues Bündnis angeboten werden kann
class iwSuggestPact : public IngameWindow
{
        const PactType pt;
        const unsigned char player;
    public:

        iwSuggestPact(const PactType pt, const unsigned char player);

    public:

        void Msg_ButtonClick(const unsigned int ctrl_id);

};




#endif // !iwBUILDORDER_H_INCLUDED
