// $Id: iwMsgbox.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef iwMSGBOX_H_INCLUDED
#define iwMSGBOX_H_INCLUDED

#pragma once

#include "IngameWindow.h"
#include "glArchivItem_Font.h"

class iwMsgbox : public IngameWindow
{
        /// Fenster, von dem aus die Box aufgerufen wurde
        Window* parent;
        /// Buttons, die auf der Box erscheinen sollen
        MsgboxButton button;
        /// ID für die Msgbox, um unterschiedliche
        unsigned int msgboxid;

        /// (Haupt)Text, der angezeigt werden soll
        const std::string& text;
        /// Einzelne Stringzeilen, die durch die Umbrechung ggf. zu Stande kommen
        std::string* strings;
        /// Anzahl der Stringzeilen
        unsigned lines_count;

    public:
        iwMsgbox(const std::string& title, const std::string& text, Window* parent, MsgboxButton button, unsigned short icon, unsigned int msgboxid = 0);
        ~iwMsgbox();

    private:

        void AddButton(unsigned short id, int x, const std::string& text, const TextureColor tc);

        void Msg_ButtonClick(const unsigned int ctrl_id);
        void Msg_PaintAfter();
};

#endif // !iwMSGBOX_H_INCLUDED
