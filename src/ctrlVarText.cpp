// $Id: ctrlVarText.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "ctrlVarText.h"

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
        // Pointerliste anlegen
        vars = new void*[count];

        // und zuweisen
        for(unsigned int i = 0; i < count; ++i)
            vars[i] = va_arg(liste, void*);
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Destruktor des variablen Textcontrols
 *
 *  @author FloSoft
 */
ctrlVarText::~ctrlVarText()
{
    // Pointerliste aufräumen
    delete[] vars;
}

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
    char buffer[1025];

    for(unsigned int i = 0, j = 0, k = 0; i < text.length() && j < 1024; ++i)
    {
        if(text[i] == '%')
        {
            ++i;
            char temp[1025];
            switch(text[i])
            {
                case 'd':
                case 'u':
                {
                    snprintf(temp, 1024, (text[i] == 'd') ? "%d" : "%u", *(int*)vars[k++]);
                    for(unsigned int x = 0; x < strlen(temp); ++x)
                        buffer[j++] = temp[x];
                } break;
                case 's':
                {
                    snprintf(temp, 1024, "%s", (char*)vars[k++]);
                    for(unsigned int x = 0; x < strlen(temp); ++x)
                        buffer[j++] = temp[x];
                } break;
                default:
                {
                    buffer[j++] = text[i - 1];
                    buffer[j++] = text[i];
                } break;
            }
        }
        else
            buffer[j++] = text[i];
        buffer[j] = '\0';
    }
    // variablen Inhalt erzeugen
    //vsnprintf(buffer, 1024, text, *(va_list*)&vars);

    // letzte byte nullen (safety, vsnprintf schreibt bei zu großem string kein null-terminator)
    buffer[1024] = '\0';

    // und zeichnen
    font->Draw( GetX(), GetY(), buffer, format, color);

    return true;
}
