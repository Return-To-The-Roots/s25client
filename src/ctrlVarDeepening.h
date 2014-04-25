// $Id: ctrlVarDeepening.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef CTRLVARDEEPENING_H_INCLUDED
#define CTRLVARDEEPENING_H_INCLUDED

#pragma once

#include "ctrlVarText.h"

class ctrlVarDeepening : public ctrlVarText
{
    public:
        ctrlVarDeepening(Window* parent, unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height, TextureColor tc,  const std::string& text, glArchivItem_Font* font, unsigned int color, unsigned int count, va_list liste);

    protected:
        virtual bool Draw_(void);

    private:
        TextureColor tc;
};

#endif // !CTRLVARDEEPENING_H_INCLUDED

