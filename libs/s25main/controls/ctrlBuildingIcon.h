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
#ifndef CTRLBUILDINGICON_H_INCLUDED
#define CTRLBUILDINGICON_H_INCLUDED

#pragma once

#include "ctrlButton.h"
class Window;

class ctrlBuildingIcon : public ctrlButton
{
public:
    ctrlBuildingIcon(Window* const parent, unsigned id, const DrawPoint& pos, const BuildingType type, const Nation nation,
                     const unsigned short size = 36, const std::string& tooltip = "");
    /// liefert den GebäudeTyp des Icons.
    BuildingType GetType() const { return type; }

protected:
    /// zeichnet das Fenster.
    void Draw_() override;
    void DrawContent() const override;

protected:
    const BuildingType type; /// der GebäudeType des Icons.
    const Nation nation;     /// Volk
};

#endif // !CTRLBUILDINGICON_H_INCLUDED
