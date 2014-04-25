// $Id: ctrlVarDeepening.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "ctrlVarDeepening.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p ctrlVarDeepening.
 *
 *  @author FloSoft
 */
ctrlVarDeepening::ctrlVarDeepening(Window* parent,
                                   unsigned int id,
                                   unsigned short x,
                                   unsigned short y,
                                   unsigned short width,
                                   unsigned short height,
                                   TextureColor tc,
                                   const std::string& text,
                                   glArchivItem_Font* font,
                                   unsigned int color,
                                   unsigned int count,
                                   va_list liste)
    : ctrlVarText(parent, id, x, y, text, color, 0, font, count, liste),
      tc(tc)
{
    // We don't want to pass these through all those constructors
    // of only-text objects down to the Window class. This is a special
    // situation, as we are a Deepening _and_ a VarText instead
    // of owning the VarText.
    this->width  = width;
    this->height = height;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  zeichnet das Fenster.
 *
 *  @author FloSoft
 */
bool ctrlVarDeepening::Draw_(void)
{
    Draw3D(x, y, width, height, tc, 2);

    char buffer[1025];

    // variablen Inhalt erzeugen
    for(unsigned int i = 0, j = 0, k = 0; i < text.length() && j < 1024; ++i)
    {
        if(text[i] == '%')
        {
            ++i;
            char temp[1025];
            switch(text[i])
            {
                case 'd':
                {
                    snprintf(temp, 1024, "%d", *(int*)vars[k++]);
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
    //vsnprintf(buffer, 1024, text, *(va_list*)&vars);

    // letzte byte nullen (safety, vsnprintf schreibt bei zu großem string kein null-terminator)
    buffer[1024] = '\0';

    font->Draw(x + width / 2, y + height / 2, buffer, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER, color);

    return true;
}
