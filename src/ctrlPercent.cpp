// $Id: ctrlPercent.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "ctrlPercent.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p ctrlPercent.
 *
 *  @author OLiver
 */
ctrlPercent::ctrlPercent(Window* parent,
                         unsigned int id,
                         unsigned short x,
                         unsigned short y,
                         unsigned short width,
                         unsigned short height,
                         TextureColor tc,
                         unsigned int text_color,
                         glArchivItem_Font* font,
                         const unsigned short* percentage)
    : Window(x, y, id, parent, width, height),
      tc(tc), text_color(text_color), font(font), percentage(percentage)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Zeichenmethode.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 *
 *  @author OLiver
 */
bool ctrlPercent::Draw_(void)
{
    // Wenn der Prozentsatzpointer = 0, dann wird 0 angezeigt und es soll nich abstürzen!
    unsigned short percentage = (this->percentage ?  *this->percentage : 0);

    // Farbe herausfinden
    unsigned int color = 0xFFFF0000;

    if(percentage > 100)
        percentage = 100;

    // Farbe wählen je nachdem wie viel Prozent
    if(percentage >= 60)
        color = 0xFF00E000;
    else if(percentage >= 30)
        color = 0xFFFFFF00;
    else if(percentage >= 20)
        color = 0xFFFF8000;

    // Box zeichnen
    Draw3D(GetX(), GetY(), width, height, tc, 2);

    // Fortschritt zeichnen
    DrawRectangle(GetX() + 4, GetY() + 4, (width - 8)*percentage / 100, height - 8, color);

    // Text zeichnen
    char caption[256];
    sprintf(caption, "%u%%", percentage);
    font->Draw(GetX() + width / 2, GetY() + height / 2, caption, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER, text_color);

    return true;
}
