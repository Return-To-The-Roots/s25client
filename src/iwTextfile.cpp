// $Id: iwTextfile.cpp 7084 2011-03-26 21:31:12Z OLiver $
//
// Copyright (c) 2005 - 2010 Settlers Freaks (sf-team at siedler25.org)
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
#include "stdafx.h"
#include "main.h"
#include "iwTextfile.h"

#include "Loader.h"
#include "controls.h"
#include "VideoDriverWrapper.h"
#include "files.h"
#include "Settings.h"
#include "iwMsgbox.h"
#include "WindowManager.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
	#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p iwTextfile.
 *
 *  @todo Das Fenster ist zu klein, um 80 Zeichen pro Zeile darstellen zu
 *        koennen. Ein horizontaler Scrollbalken fuer 640x480 waere nicht
 *        schlecht.
 *
 *  @author Devil
 *  @author FloSoft
 *  @author OLiver
 */
iwTextfile::iwTextfile(const std::string& filename, const std::string& title)
	: IngameWindow(CGI_README, 0xFFFF, 0xFFFF, 640, 480, title, LOADER.GetImageN("resource", 41))
{
	// Pfad mit gewählter Sprache auswählen
	std::string path = GetFilePath(FILE_PATHS[88]) + Settings::inst().language.language + "/" + filename;

	std::ifstream file(path.c_str());

	ctrlMultiline *text = AddMultiline(2, 10, 20, width - 20, 450, TC_GREEN1, NormalFont, glArchivItem_Font::DF_LEFT | glArchivItem_Font::DF_TOP);

	if(!file.good())
	{
		// lokalisierte Vresion nicht gefunden, Standard öffnen
		path = FILE_PATHS[88] + filename;
		file.clear();
		file.open(path.c_str());
		if(!file.good())
		{
			// immer noch nichts gefunden? --> Dann Fehlermeldung
			text->AddString(_("The readme file was not found!"), COLOR_RED, false);
			// und raus
			return;
		}	
	}
	// Erste Zeile auslesen und die Breite des Fensters und des Multilinecontrols danach bestimmen
	std::string line;
	std::getline(file, line);
	file.seekg(0);
	unsigned short first_line_width = NormalFont->getWidth(line);
	SetWidth(first_line_width + 20 + 30);
	text->SetWidth(first_line_width + 30);

	while(!file.eof())
	{
		std::getline(file, line);
		text->AddString(line.c_str(), COLOR_YELLOW, false);
	}
	file.close();
}
