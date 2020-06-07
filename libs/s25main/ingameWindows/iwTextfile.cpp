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
#include "controls/ctrlMultiline.h"
#include "files.h"
#include "helpers/containerUtils.h"
#include "gameData/const_gui_ids.h"
#include <boost/filesystem/operations.hpp>
#include <boost/nowide/fstream.hpp>
#include <mygettext/mygettext.h>
#include <mygettext/utils.h>
#include <sstream>

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
    std::vector<bfs::path> paths;
    const bfs::path basePath = RTTRCONFIG.ExpandPath(s25::folders::texte);
    for(const auto& folderName : getPossibleFoldersForLangCode(mysetlocale(0, nullptr)))
        paths.push_back(basePath / folderName / filename);
    paths.push_back(basePath / filename);

    const auto existingFilePath = helpers::find_if(paths, [](const auto& path) { return bfs::exists(path); });
    bnw::ifstream file;
    if(existingFilePath != paths.end())
        file.open(*existingFilePath);
    if(!file)
    {
        text->AddString((boost::format(_("The file was not found!")) % filename).str(), COLOR_RED, false);
    } else
    {
        std::stringstream stream;
        stream << file.rdbuf();
        text->AddString(stream.str(), COLOR_YELLOW, false);
    }

    text->SetWidth(text->GetContentSize().x);
    SetIwSize(Extent(text->GetSize().x, GetIwSize().y));
}
