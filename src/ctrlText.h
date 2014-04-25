// $Id: ctrlText.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef CTRLTEXT_H_INCLUDED
#define CTRLTEXT_H_INCLUDED

#pragma once

#include "Window.h"


/// Basisklasse für Controls mit Texten wie auch Buttons, damit diese alle einheitlich verändert werden können
class ctrlBaseText
{
    public:

        ctrlBaseText(const std::string& text, const unsigned color, glArchivItem_Font* font);

        /// Setzt Text
        void SetText(const std::string& text) { this->text = text; }
        /// Setzt Schriftart
        void SetFont(glArchivItem_Font* font) { this->font = font; }
        /// Setzt Textfarbe
        void SetColor(const unsigned color) { this->color = color; }

        /// Gibt Text zurück
        const std::string& GetText() const { return text; }

    protected:

        std::string text;
        unsigned int color;
        glArchivItem_Font* font;
};


class ctrlText : public Window, public ctrlBaseText
{
    public:
        ctrlText(Window* parent, unsigned int id, unsigned short x, unsigned short y, const std::string& text, unsigned int color, unsigned int format, glArchivItem_Font* font);
    protected:
        virtual bool Draw_(void);

    protected:

        unsigned int format;
};

#endif // !CTRLTEXT_H_INCLUDED
