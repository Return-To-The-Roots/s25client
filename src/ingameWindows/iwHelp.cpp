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
#include "defines.h" // IWYU pragma: keep
#include "iwHelp.h"
#include "controls/ctrlMultiline.h"
#include "Loader.h"
#include "ogl/glArchivItem_Font.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

/// Breite des Fensters
const unsigned short HELP_WINDOW_WIDTH = 240;

/// Maximale Anzahl von Zeilen, bis Scrollbar eingesetzt wird
const unsigned MAX_LINES = 15;

iwHelp::iwHelp(const GUI_ID gui_id, const std::string& title, const std::string& content)
    : IngameWindow(gui_id, 0xFFFE, 0xFFFE, HELP_WINDOW_WIDTH, 480, title, LOADER.GetImageN("resource", 41))
{
    glArchivItem_Font::WrapInfo wi = NormalFont->GetWrapInfo(content, HELP_WINDOW_WIDTH - 28, HELP_WINDOW_WIDTH - 28);

    // Mehr Linien benötigt als die maximalen? Dann kommt ja noch die Scrollbar dran und der ganze Spaß muss
    // umgebrochen werden, also nochmal mit geringerer Breite berechnen
    if(wi.positions.size() > MAX_LINES)
    {
        wi = NormalFont->GetWrapInfo(content,
            HELP_WINDOW_WIDTH - 28 - ctrlMultiline::SCROLLBAR_WIDTH,
            HELP_WINDOW_WIDTH - 24 - ctrlMultiline::SCROLLBAR_WIDTH);
    }

    unsigned int show_lines = std::min( (unsigned int)wi.positions.size(), MAX_LINES);

    unsigned short text_height = show_lines * NormalFont->getHeight();

    // Höhe setzen
    SetIwHeight(text_height + 40);
    // Fenster neben die Maus schieben
    MoveNextToMouse();

    // Größe des Fensters und des Controls nach der Anzahl der Zeilen
    ctrlMultiline* text = AddMultiline(2, 10, 20, HELP_WINDOW_WIDTH - 20, text_height + 4, TC_GREEN1, NormalFont, glArchivItem_Font::DF_LEFT | glArchivItem_Font::DF_TOP);
    text->EnableBox(false);

    std::vector<std::string> lines = wi.CreateSingleStrings(content);
    for(std::vector<std::string>::const_iterator it = lines.begin(); it != lines.end(); ++it)
        text->AddString(*it, COLOR_YELLOW, false);

}
