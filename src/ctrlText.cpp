// $Id: ctrlText.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "ctrlText.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

ctrlBaseText::ctrlBaseText(const std::string& text, const unsigned color, glArchivItem_Font* font) :
    text(text), color(color), font(font)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p ctrlText.
 *
 *  @author OLiver
 */
ctrlText::ctrlText(Window* parent,
                   unsigned int id,
                   unsigned short x,
                   unsigned short y,
                   const std::string& text,
                   unsigned int color,
                   unsigned int format,
                   glArchivItem_Font* font)
    : Window(x, y, id, parent), ctrlBaseText(text, color, font), format(format)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  zeichnet das Fenster.
 *
 *  @author OLiver
 */
bool ctrlText::Draw_(void)
{
    if(text.length())
        font->Draw(GetX(), GetY(), text.c_str(), format, color);

    return true;
}
