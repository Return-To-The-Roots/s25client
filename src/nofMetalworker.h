// $Id: nofMetalworker.h 9447 2014-06-20 21:40:55Z jh $
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

#ifndef NOF_METALWORKER_H_
#define NOF_METALWORKER_H_

#include "nofWorkman.h"

class nobUsualBuilding;

/// Klasse für den Schreiner
class nofMetalworker : public nofWorkman
{
        /// Zeichnet ihn beim Arbeiten
        void DrawWorking(int x, int y);
        /// Gibt die ID in JOBS.BOB zurück, wenn der Beruf Waren rausträgt (bzw rein)
        unsigned short GetCarryID() const;
        /// Der Arbeiter erzeugt eine Ware
        GoodType ProduceWare();
        
        unsigned ToolsOrderedTotal() const;

    public:

        nofMetalworker(const unsigned short x, const unsigned short y, const unsigned char player, nobUsual* workplace);
        nofMetalworker(SerializedGameData* sgd, const unsigned obj_id);

        GO_Type GetGOT() const { return GOT_NOF_METALWORKER; }
};

#endif
