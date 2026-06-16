// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ctrlButton.h"
class Window;

class ctrlBuildingIcon : public ctrlButton
{
public:
    ctrlBuildingIcon(Window* parent, unsigned id, const DrawPoint& pos, BuildingType type, Nation nation,
                     unsigned short size = 36, const std::string& tooltip = "");
    /// liefert den GebäudeTyp des Icons.
    BuildingType GetType() const { return type; }

protected:
    /// zeichnet das Fenster.
    void Draw_() override;
    void DrawContent() const override;

    const BuildingType type; /// der GebäudeType des Icons.
    const Nation nation;     /// Volk
};
