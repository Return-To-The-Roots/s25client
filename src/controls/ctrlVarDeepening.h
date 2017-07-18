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
#ifndef CTRLVARDEEPENING_H_INCLUDED
#define CTRLVARDEEPENING_H_INCLUDED

#pragma once

#include "ctrlVarText.h"
#include <cstdarg>
class Window;
class glArchivItem_Font;

class ctrlVarDeepening : public ctrlVarText
{
    public:
        /// liste contains pointers to int, unsigned or const char and must be valid for the lifetime of the var text!
        ctrlVarDeepening(Window* parent, unsigned int id, const DrawPoint& pos, const Extent& size, TextureColor tc,  const std::string& text, glArchivItem_Font* font, unsigned int color, unsigned int count, va_list liste);

    protected:
        void Draw_() override;

    private:
        TextureColor tc;
};

#endif // !CTRLVARDEEPENING_H_INCLUDED

