// $Id: glArchivItem_Bob.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef GLARCHIVITEM_BOB_H_INCLUDED
#define GLARCHIVITEM_BOB_H_INCLUDED

#pragma once

/// Klasse für GL-Bobfiles.
class glArchivItem_Bob : public libsiedler2::ArchivItem_Bob
{
    public:
        /// Konstruktor von @p glArchivItem_Bob.
        glArchivItem_Bob(void) : ArchivItem_Bob() {};
        /// Kopierkonstruktor von @p glArchivItem_Bob.
        glArchivItem_Bob(const glArchivItem_Bob* item) : ArchivItem_Bob(item) {};

        /// Zeichnet einen Animationsstep.
        void Draw(unsigned int item, unsigned int direction, bool fat, unsigned int animationstep, short x, short y, unsigned int color);
};

#endif // !GLARCHIVITEM_BOB_H_INCLUDED
