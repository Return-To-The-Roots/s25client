// $Id: ctrlProgress.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef CTRLPROGRESS_H_INCLUDED
#define CTRLPROGRESS_H_INCLUDED

#pragma once

#include "Window.h"

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
        const unsigned short& GetPosition(void) const { return position; }

        virtual void Msg_ButtonClick(const unsigned int ctrl_id);
        virtual bool Msg_LeftDown(const MouseCoords& mc);
        virtual bool Msg_LeftUp(const MouseCoords& mc);
        virtual bool Msg_WheelUp(const MouseCoords& mc);
        virtual bool Msg_WheelDown(const MouseCoords& mc);
        virtual bool Msg_MouseMove(const MouseCoords& mc);

    protected:
        virtual bool Draw_(void);

    private:
        TextureColor tc;

        unsigned short position;
        unsigned short minimum;
        unsigned short maximum;

        // Abstand vom Button zur Leiste (Leiste wird entsprechend verkleinert!)
        unsigned short x_padding, y_padding;

        /// Falls der Balken immer eine bestimmte Farben haben soll, ansonsten 0 setzen!
        unsigned int force_color;

        /// Optionaler Pointer auf eine Variable,
};

#endif // !CTRLPROGRESS_H_INCLUDED
