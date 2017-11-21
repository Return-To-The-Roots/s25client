// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
// Copyright (c) 2013 Nevik Rehnel (hai.kataker at gmx.de)
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

#include "rttrDefines.h" // IWYU pragma: keep
#include "iwTextfile.h"

#include "Loader.h"
#include "RttrConfig.h"
#include "Settings.h"
#include "controls/ctrlMultiline.h"
#include "files.h"
#include "gameData/const_gui_ids.h"
#include <boost/nowide/fstream.hpp>

/**
 *  Konstruktor von @p iwTextfile.
 *
 *  @todo Das Fenster ist zu klein, um 80 Zeichen pro Zeile darstellen zu
 *        koennen. Ein horizontaler Scrollbalken fuer 640x480 waere nicht
 *        schlecht.
 */
iwTextfile::iwTextfile(const std::string& filename, const std::string& title)
    : IngameWindow(CGI_README, IngameWindow::posLastOrCenter, Extent(640, 480), title, LOADER.GetImageN("resource", 41))
{
    ctrlMultiline* text = AddMultiline(2, DrawPoint(10, 20), Extent(GetSize().x - 20, 450), TC_GREEN1, NormalFont);

    // Pfad mit gewählter Sprache auswählen
    std::string path = RTTRCONFIG.ExpandPath(FILE_PATHS[88]) + "/" + SETTINGS.language.language + "/" + filename;

    bnw::ifstream file(path);
    if(!file)
    {
        // lokalisierte Vresion nicht gefunden, Standard öffnen
        path = RTTRCONFIG.ExpandPath(FILE_PATHS[88]) + "/" + filename;
        file.clear();
        file.open(path);
        if(!file)
        {
            // immer noch nichts gefunden? --> Dann Fehlermeldung
            text->AddString(_("The readme file was not found!"), COLOR_RED, false);
            return;
        }
    }

    std::string line; // buffer for one line
    while(std::getline(file, line))
        text->AddString(line, COLOR_YELLOW, false);

    text->SetWidth(text->GetContentSize().x);
    SetIwSize(Extent(text->GetSize().x, GetIwSize().y));
}
