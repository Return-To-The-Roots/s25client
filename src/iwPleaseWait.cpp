// $Id: iwPleaseWait.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "iwPleaseWait.h"

#include "Loader.h"

#include "GameManager.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p iwPleaseWait.
 *
 *  Fenster wird modal geöffnet, damit man ggf. einen "Weiter"-Button nicht
 *  mehrfach betätigen kann.
 *
 *  @author OLiver
 */
iwPleaseWait::iwPleaseWait(void) : IngameWindow(CGI_PLEASEWAIT, 0xFFFF, 0xFFFF, 300, 60, _("Please wait..."), LOADER.GetImageN("resource", 41), true)
{
    GAMEMANAGER.SetCursor(CURSOR_MOON);
    AddText(0, GetWidth() / 2, GetHeight() / 2, _("Please wait..."), COLOR_YELLOW, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER, NormalFont);
}

iwPleaseWait::~iwPleaseWait()
{
    GAMEMANAGER.SetCursor();
}
