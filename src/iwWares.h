// $Id: iwWares.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef iwWARES_H_INCLUDED
#define iwWARES_H_INCLUDED

#pragma once

#include "IngameWindow.h"


class iwWares : public IngameWindow
{
    protected:
        const Goods* const inventory;     ///< Warenbestand
        unsigned char page;       ///< aktuelle Seite des Inventurfensters.
        unsigned char page_count; ///< maximale Seite des Inventurfensters.

    public:
        /// Konstruktor von @p iwInventory.
        iwWares(unsigned int id, unsigned short x, unsigned short y, const unsigned short width, const unsigned short height, const std::string& title, unsigned char page_count,
                bool allow_outhousing, glArchivItem_Font* font, const Goods* inventory);
        /// bestimmte Inventurseite zeigen.
        void SetPage(unsigned char page);
        /// setzt die maximale Seitenzahl.
        void SetPageCount(unsigned char page_count) { this->page_count = page_count; }

    protected:

        void Msg_ButtonClick(const unsigned int ctrl_id);
        void Msg_PaintBefore();
};

#endif // !iwINVENTORY_H_INCLUDED
