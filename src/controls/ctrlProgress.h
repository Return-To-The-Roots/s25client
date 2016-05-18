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
#ifndef CTRLPROGRESS_H_INCLUDED
#define CTRLPROGRESS_H_INCLUDED

#pragma once

#include "Window.h"
class MouseCoords;

class ctrlProgress : public Window
{
    public:
        ctrlProgress(Window* parent,
                     const unsigned int id,
                     const unsigned short x,
                     const unsigned short y,
                     const unsigned short width,
                     const unsigned short height,
                     const TextureColor tc,
                     unsigned short button_minus,
                     unsigned short button_plus,
                     const unsigned short maximum,
                     const unsigned short x_padding,
                     const unsigned short y_padding,
                     const unsigned int force_color,
                     const std::string& tooltip,
                     const std::string& button_minus_tooltip = NULL,
                     const std::string& button_plus_tooltip = NULL,
                     unsigned short* const write_val = NULL);

        void SetPosition(unsigned short position);
        const unsigned short& GetPosition() const { return position; }

        void Msg_ButtonClick(const unsigned int ctrl_id) override;
        bool Msg_LeftDown(const MouseCoords& mc) override;
        bool Msg_LeftUp(const MouseCoords& mc) override;
        bool Msg_WheelUp(const MouseCoords& mc) override;
        bool Msg_WheelDown(const MouseCoords& mc) override;
        bool Msg_MouseMove(const MouseCoords& mc) override;

    protected:
        bool Draw_() override;
        void Resize_(unsigned short width, unsigned short height) override;

    private:
        TextureColor tc;

        unsigned short position;
        unsigned short maximum;

        // Abstand vom Button zur Leiste (Leiste wird entsprechend verkleinert!)
        DrawPoint padding;

        /// Falls der Balken immer eine bestimmte Farben haben soll, ansonsten 0 setzen!
        unsigned int force_color;

        unsigned CalcBarWidth() const;
};

#endif // !CTRLPROGRESS_H_INCLUDED
