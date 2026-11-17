// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

Rect ctrlVarText::getBoundaryRect() const
{
    return font->getBounds(getDrawPos(), getFormatedText(), format_);
}

void ctrlVarText::drawImpl()
{
    font->draw(getDrawPos(), getFormatedText(), format_, color_);
}
