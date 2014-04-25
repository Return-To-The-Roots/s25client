// $Id: iwDemolishBuilding.h
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
#ifndef iwDEMOLISHBUILDING_H_INCLUDED
#define iwDEMOLISHBUILDING_H_INCLUDED

#pragma once

#include "IngameWindow.h"
#include "GameConsts.h"
#include "GameObject.h"
#include "noBuilding.h"

class dskGameInterface;
class GameWorldViewer;

/// Fenster, welches eine Sicherheitsabfrage vor dem Abreißen eines Gebäudes durchführt
class iwDemolishBuilding : public IngameWindow
{
        GameWorldViewer* const gwv;
        const noBaseBuilding* building;
        const bool flag;

    public:

        /// Konstruktor von @p iwBuilding.
//  iwDemolishBuilding(GameWorldViewer * const gwv,const GO_Type got,const unsigned short building_x, const unsigned short building_y,const BuildingType building, const Nation nation, const unsigned guiid);
        iwDemolishBuilding(GameWorldViewer* const gwv, const noBaseBuilding* building, const bool flag = false);

    private:

        void Msg_ButtonClick(const unsigned int ctrl_id);
        void Msg_PaintBefore();

};

#endif // !iwDEMOLISHBUILDING_H_INCLUDED
