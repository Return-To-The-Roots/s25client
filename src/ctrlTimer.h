// $Id: ctrlTimer.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef CTRLTIMER_H_INCLUDED
#define CTRLTIMER_H_INCLUDED

#pragma once

#include "Window.h"

class ctrlTimer : public Window
{
    public:
        ctrlTimer(Window* parent, unsigned int id, unsigned int timeout);

        void Start(unsigned int timeout);
        void Stop(void);

        virtual void Msg_PaintBefore();

    protected:
        virtual bool Draw_(void) { return true; };

    private:
        unsigned int timeout;
        unsigned int timer;
};

#endif // !CTRLTIMER_H_INCLUDED
