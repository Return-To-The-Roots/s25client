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
#ifndef CTRLVARTEXT_H_INCLUDED
#define CTRLVARTEXT_H_INCLUDED

#pragma once

#include "ctrlText.h"
#include <cstdarg>
#include <vector>
#include <string>
class Window;
class glArchivItem_Font;

class ctrlVarText : public ctrlText
{
    public:
        /// liste contains pointers to int, unsigned or const char and must be valid for the lifetime of the var text!
        ctrlVarText(Window* parent, unsigned int id, unsigned short x, unsigned short y, const std::string& formatstr, unsigned int color, unsigned int format, glArchivItem_Font* font, unsigned int count, va_list liste);
        ~ctrlVarText() override;

        Rect GetBoundaryRect() const override;

    protected:
        void Draw_() override;
        /// Returns the text with placeholders replaced by the actual vars
        std::string GetFormatedText() const;

    protected:
        std::vector<void*> vars;
};

#endif // !CTRL_VARTEXT_H_INCLUDED
