// $Id: iwSkipGFs.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "iwSkipGFs.h"

#include "GameClient.h"
#include "Loader.h"
#include "controls.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p iwPlayReplay.
 *
 *  @author OLiver
 */
iwSkipGFs::iwSkipGFs(void)
    : IngameWindow(CGI_SKIPGFS, 0xFFFF, 0xFFFF, 300, 110, _("Skip GameFrames"), LOADER.GetImageN("resource", 41))
{
    // Text vor Editfeld
    AddText(0, 50, 36, _("to GameFrame:"), COLOR_YELLOW, 0, NormalFont);

    // Editfeld zum Eingeben des Ziel-GF
    ctrlEdit* edit = AddEdit(1, 126, 32, 120, 20, TC_GREY, NormalFont);
    edit->SetFocus();

    // OK-Button
    AddTextButton(2, 110, 65, 80, 22, TC_GREEN2, _("OK"), NormalFont);
}

void iwSkipGFs::SkipGFs()
{
    int gf = atoi(GetCtrl<ctrlEdit>(1)->GetText().c_str());
    GAMECLIENT.SkipGF(gf);
}

void iwSkipGFs::Msg_ButtonClick(const unsigned int ctrl_id)
{
    SkipGFs();
}

void iwSkipGFs::Msg_EditEnter(const unsigned int ctrl_id)
{
    SkipGFs();
}
