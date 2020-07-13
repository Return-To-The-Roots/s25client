// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "iwHelp.h"
#include "Loader.h"
#include "controls/ctrlMultiline.h"
#include "gameData/const_gui_ids.h"

/// Breite des Fensters
const unsigned short HELP_WINDOW_WIDTH = 280;

/// Maximale Anzahl von Zeilen, bis Scrollbar eingesetzt wird
const unsigned MAX_LINES = 15;

iwHelp::iwHelp(const std::string& content)
    : IngameWindow(CGI_HELP, IngameWindow::posAtMouse, Extent(HELP_WINDOW_WIDTH, 480), _("What is this?"), LOADER.GetImageN("io", 1))
{
    // Größe des Fensters und des Controls nach der Anzahl der Zeilen
    ctrlMultiline* text = AddMultiline(2, DrawPoint(contentOffset), GetIwSize(), TC_GREEN1, NormalFont);
    text->SetNumVisibleLines(MAX_LINES);
    text->ShowBackground(false);
    text->AddString(content, COLOR_YELLOW, false);
    // Shrink to content
    text->Resize(text->GetContentSize());
    SetIwSize(text->GetSize());
    MoveNextToMouse();
}
