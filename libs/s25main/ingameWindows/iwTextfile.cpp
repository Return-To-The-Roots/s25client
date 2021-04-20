// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
// Copyright (c) 2013 Nevik Rehnel (hai.kataker at gmx.de)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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

namespace bfs = boost::filesystem;

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
    ctrlMultiline* text =
      AddMultiline(2, DrawPoint(10, 20), Extent(GetSize().x - 20, 450), TextureColor::Green1, NormalFont);

    // Pfad mit gewählter Sprache auswählen
    std::vector<bfs::path> paths;
    const bfs::path basePath = RTTRCONFIG.ExpandPath(s25::folders::texte);
    for(const auto& folderName : mygettext::getPossibleFoldersForLangCode(mygettext::setlocale(0, nullptr)))
        paths.push_back(basePath / folderName / filename);
    paths.push_back(basePath / filename);

    const auto existingFilePath = helpers::find_if(paths, [](const auto& path) { return bfs::exists(path); });
    boost::nowide::ifstream file;
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
