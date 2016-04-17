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

#include "defines.h" // IWYU pragma: keep
#include "iwMissionStatement.h"
#include "controls/ctrlMultiline.h"
#include "Loader.h"
#include "ogl/glArchivItem_Font.h"
#include "gameData/const_gui_ids.h"
#include <sstream>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

/**
 *  Konstruktor von @p iwTextfile.
 *
 *  @todo Das Fenster ist zu klein, um 80 Zeichen pro Zeile darstellen zu
 *        koennen. Ein horizontaler Scrollbalken fuer 640x480 waere nicht
 *        schlecht.
 */
iwMissionStatement::iwMissionStatement(const std::string& title, const std::string& content)
    : IngameWindow(CGI_MISSION_STATEMENT, 0xFFFF, 0xFFFF, 640, 480, title, LOADER.GetImageN("io", 5))
{
    ctrlMultiline* text = AddMultiline(0, 10, 20, width_ - 20, 450, TC_GREEN2, NormalFont, glArchivItem_Font::DF_LEFT | glArchivItem_Font::DF_TOP);

    unsigned short max_line_width = 0;
    
    std::stringstream ss(content);
    std::string line;
    
    while (std::getline(ss, line, '\n'))
    {
        text->AddString(line, COLOR_YELLOW, false); // add this line to the window contents
        unsigned short current_line_width = NormalFont->getWidth(line); // get the width of line in normal font
        if (current_line_width > max_line_width) // if wider than max, re-set max
        {
            max_line_width = current_line_width;
        }
    }

    SetWidth(max_line_width + 20 + 30); // set window width to our determined max width
    text->SetWidth(max_line_width + 30);
    
    AddTextButton(1, width_ / 2 - 100, 435, 200, 22, TC_GREEN2, _("Continue"), NormalFont);
}

void iwMissionStatement::Msg_ButtonClick(const unsigned int  /*ctrl_id*/)
{
    Close();
}

