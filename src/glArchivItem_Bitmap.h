// $Id: glArchivItem_Bitmap.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef GLARCHIVITEM_BITMAP_INCLUDED
#define GLARCHIVITEM_BITMAP_INCLUDED

#pragma once

#include "../libutil/src/colors.h"

#ifdef _WIN32
typedef unsigned long long uint64_t;
#endif

/// Basisklasse für GL-Bitmapitems.
class glArchivItem_Bitmap : public virtual libsiedler2::baseArchivItem_Bitmap
{
    public:
        /// Konstruktor von @p glArchivItem_Bitmap.
        glArchivItem_Bitmap(void);
        /// Kopiekonstruktor von @p glArchivItem_Bitmap.
        glArchivItem_Bitmap(const glArchivItem_Bitmap* item);

        /// Destruktor von @p glArchivItem_Bitmap.
        virtual ~glArchivItem_Bitmap(void);

        /// Erzeugt und zeichnet die Textur.
        virtual void Draw(short dst_x, short dst_y, short dst_w = 0, short dst_h = 0, short src_x = 0, short src_y = 0, short src_w = 0, short src_h = 0, const unsigned int color = COLOR_WHITE, const unsigned int unused = COLOR_WHITE);

        /// liefert das GL-Textur-Handle.
        unsigned int GetTexture();
        /// Löscht die GL-Textur (z.B fürs Neuerstellen)
        virtual void DeleteTexture();
        /// Setzt den Texturfilter auf einen bestimmten Wert.
        virtual void setFilter(unsigned int filter);

        unsigned short GetTexWidth() {return(tex_width);}
        unsigned short GetTexHeight() {return(tex_width);}

    protected:
        /// Erzeugt die Textur.
        virtual void GenerateTexture();

    protected:
        unsigned int texture; ///< Das GL-Textur-Handle
        unsigned int filter;  ///< Der aktuell gewählte Texturfilter
};

#endif // !GLARCHIVITEM_BITMAP_INCLUDED
