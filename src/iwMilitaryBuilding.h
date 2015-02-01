// $Id: iwMilitaryBuilding.h 9592 2015-02-01 09:39:38Z marcus $
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
#ifndef iwMILITARYBUILDING_H_INCLUDED
#define iwMILITARYBUILDING_H_INCLUDED

#pragma once

#include "IngameWindow.h"

class nobMilitary;
class GameWorldViewer;
class dskGameInterface;

class iwMilitaryBuilding : public IngameWindow
{
    private:
        nobMilitary* const building;
        GameWorldViewer* const gwv;
		dskGameInterface* const gi;

    public:
        iwMilitaryBuilding(GameWorldViewer* const gwv, dskGameInterface* const gi, nobMilitary* const building);

        /// Zeigt Messagebox an, dass das Militärgebäude nicht abgerissen werden kann (Abriss-Verbot)
        static void DemolitionNotAllowed();

    private:
        void Msg_PaintAfter();
        void Msg_ButtonClick(const unsigned int ctrl_id);
};

#endif // !iwMILITARYBUILDING_H_INCLUDED
