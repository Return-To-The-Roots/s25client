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

#pragma once

#include "Window.h"
#include "controls/ctrlBaseTooltip.h"
class MouseCoords;

class ctrlProgress : public Window, public ctrlBaseTooltip
{
public:
    ctrlProgress(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, unsigned short button_minus,
                 unsigned short button_plus, unsigned short maximum, const Extent& padding, unsigned force_color,
                 const std::string& tooltip, const std::string& button_minus_tooltip = "", const std::string& button_plus_tooltip = "",
                 unsigned short* write_val = nullptr);

    void Resize(const Extent& newSize) override;
    void SetPosition(unsigned short position);
    const unsigned short& GetPosition() const { return position; }

    void Msg_ButtonClick(unsigned ctrl_id) override;
    bool Msg_LeftDown(const MouseCoords& mc) override;
    bool Msg_LeftUp(const MouseCoords& mc) override;
    bool Msg_WheelUp(const MouseCoords& mc) override;
    bool Msg_WheelDown(const MouseCoords& mc) override;
    bool Msg_MouseMove(const MouseCoords& mc) override;

protected:
    void Draw_() override;

private:
    TextureColor tc;

    unsigned short position;
    unsigned short maximum;

    // Abstand vom Button zur Leiste (Leiste wird entsprechend verkleinert!)
    Extent padding_;

    /// Falls der Balken immer eine bestimmte Farben haben soll, ansonsten 0 setzen!
    unsigned force_color;

    unsigned CalcBarWidth() const;
};
