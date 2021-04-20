// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwHelp.h"
#include "Loader.h"
#include "controls/ctrlMultiline.h"
#include "gameData/const_gui_ids.h"

/// Breite des Fensters
const unsigned short HELP_WINDOW_WIDTH = 280;

/// Maximale Anzahl von Zeilen, bis Scrollbar eingesetzt wird
const unsigned MAX_LINES = 15;

iwHelp::iwHelp(const std::string& content)
    : IngameWindow(CGI_HELP, IngameWindow::posAtMouse, Extent(HELP_WINDOW_WIDTH, 480), _("What is this?"),
                   LOADER.GetImageN("io", 1))
{
    // Größe des Fensters und des Controls nach der Anzahl der Zeilen
    ctrlMultiline* text = AddMultiline(2, DrawPoint(contentOffset), GetIwSize(), TextureColor::Green1, NormalFont);
    text->SetNumVisibleLines(MAX_LINES);
    text->ShowBackground(false);
    text->AddString(content, COLOR_YELLOW, false);
    // Shrink to content
    text->Resize(text->GetContentSize());
    SetIwSize(text->GetSize());
    MoveNextToMouse();
}
