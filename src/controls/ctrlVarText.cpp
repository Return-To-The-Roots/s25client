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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h"
#include "ctrlVarText.h"
#include "ogl/glArchivItem_Font.h"
#include <sstream>

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
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
 *
 *  @author FloSoft
 */
ctrlVarText::ctrlVarText(Window* parent,
                         unsigned int id,
                         unsigned short x,
                         unsigned short y,
                         const std::string& formatstr,
                         unsigned int color,
                         unsigned int format,
                         glArchivItem_Font* font,
                         unsigned int count,
                         va_list liste)
    : ctrlText(parent, id, x, y, formatstr, color, format, font)
{
    // Pointerliste einlesen
    if(count > 0)
    {
        // und zuweisen
        for(unsigned int i = 0; i < count; ++i)
            vars.push_back(va_arg(liste, void*));
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Destruktor des variablen Textcontrols
 *
 *  @author FloSoft
 */
ctrlVarText::~ctrlVarText()
{}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Zeichenmethode
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 *
 *  @author FloSoft
 */
bool ctrlVarText::Draw_(void)
{
    font->Draw( GetX(), GetY(), GetFormatedText(), format, color_);

    return true;
}

std::string ctrlVarText::GetFormatedText() const
{
    std::stringstream str;

    unsigned curVar = 0;
    bool isInFormat = false;

    for(std::string::const_iterator it = text.begin(); it != text.end(); ++it)
    {
        if(isInFormat)
        {
            isInFormat = false;
            switch(*it)
            {
            case 'd':
                str << *reinterpret_cast<int*>(vars[curVar]);
                curVar++;
                break;
            case 'u':
                str << *reinterpret_cast<unsigned*>(vars[curVar]);
                curVar++;
                break;
            case 's':
                str << reinterpret_cast<const char*>(vars[curVar]);
                curVar++;
                break;
            case '%':
                str << '%';
                break;
            default:
                RTTR_Assert(false); // Invalid format string
                str << '%' << *it;
                break;
            }
        }
        else if(*it == '%')
            isInFormat = true;
        else
            str << *it;
    }

    if(isInFormat)
    {
        RTTR_Assert(false); // Invalid format string
        str << '%';
    }

    return str.str();
}