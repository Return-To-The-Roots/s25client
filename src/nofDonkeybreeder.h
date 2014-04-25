// $Id: nofDonkeybreeder.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef NOFDONKEYBREEDER_H_INCLUDED
#define NOFDONKEYBREEDER_H_INCLUDED

#pragma once

#include "nofWorkman.h"

class nobUsualBuilding;

/// Klasse für den Eselzüchter
class nofDonkeybreeder : public nofWorkman
{
    public:
        nofDonkeybreeder(unsigned short x, unsigned short y, unsigned char player, nobUsual* workplace);
        nofDonkeybreeder(SerializedGameData* sgd, unsigned int obj_id);

        GO_Type GetGOT() const { return GOT_NOF_DONKEYBREEDER; }

    private:
        /// Zeichnet ihn beim Arbeiten.
        void DrawWorking(int x, int y);
        /// Der Arbeiter erzeugt eine Ware.
        GoodType ProduceWare();
        /// Wird aufgerufen, wenn er fertig mit arbeiten ist
        void WorkFinished();

        /// Gibt die ID in JOBS.BOB zurück, wenn der Beruf Waren rausträgt (bzw rein)
        unsigned short GetCarryID() const { return 0; }
};

#endif // !NOFDONKEYBREEDER_H_INCLUDED
