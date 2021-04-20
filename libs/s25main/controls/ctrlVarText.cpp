// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
ctrlVarText::ctrlVarText(Window* parent, unsigned id, const DrawPoint& pos, const std::string& formatstr,
                         unsigned color, FontStyle format, const glFont* font, unsigned count, va_list fmtArgs)
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
