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

#include "ctrlVarText.h"
#include "ogl/glFont.h"

/**
 *  Konstruktor des Textcontrols, welches variablen Inhalt haben kann.
 *
 *  @param[in] parent    Handle zum übergeordneten Fenster
 *  @param[in] id        Steuerelement-ID
 *  @param[in] x         X-Position des Steuerelements
 *  @param[in] y         Y-Position des Steuerelements
 *  @param[in] formatstr Formatstring (vgl. printf)
 *  @param[in] color     Farbe des Textes
 *  @param[in] format    Format des Textes (links, mittig, rechts, usw)
 *  @param[in] font      Schrift des Textes
 *  @param[in] count     Anzahl der nachfolgenden Pointer
 *  @param[in] liste     Pointerliste der variablen Inhalte
 */
ctrlVarText::ctrlVarText(Window* parent, unsigned id, const DrawPoint& pos, const std::string& formatstr, unsigned color, FontStyle format,
                         const glFont* font, unsigned count, va_list fmtArgs)
    : Window(parent, id, pos), ctrlBaseVarText(formatstr, color, font, count, fmtArgs), format_(format)
{}

ctrlVarText::~ctrlVarText() = default;

Rect ctrlVarText::GetBoundaryRect() const
{
    return font->getBounds(GetDrawPos(), GetFormatedText(), format_);
}

void ctrlVarText::Draw_()
{
    font->Draw(GetDrawPos(), GetFormatedText(), format_, color_);
}
