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

#include "rttrDefines.h" // IWYU pragma: keep
#include "iwSkipGFs.h"
#include "Loader.h"
#include "controls/ctrlEdit.h"
#include "network/GameClient.h"
#include "gameData/const_gui_ids.h"
#include "libutil/StringConversion.h"
#include "libutil/colors.h"

iwSkipGFs::iwSkipGFs(GameWorldView& gwv)
    : IngameWindow(CGI_SKIPGFS, IngameWindow::posLastOrCenter, Extent(300, 110), _("Skip GameFrames"), LOADER.GetImageN("resource", 41)),
      gwv(gwv)
{
    // Text vor Editfeld
    AddText(0, DrawPoint(50, 36), _("to GameFrame:"), COLOR_YELLOW, 0, NormalFont);

    // Editfeld zum Eingeben des Ziel-GF
    ctrlEdit* edit = AddEdit(1, DrawPoint(126, 32), Extent(120, 20), TC_GREY, NormalFont);
    edit->SetFocus();

    // OK-Button
    AddTextButton(2, DrawPoint(110, 65), Extent(80, 22), TC_GREEN2, _("OK"), NormalFont);
}

void iwSkipGFs::SkipGFs()
{
    int gf = s25util::fromStringClassicDef(GetCtrl<ctrlEdit>(1)->GetText().c_str(), 0);
    GAMECLIENT.SkipGF(gf, gwv);
}

void iwSkipGFs::Msg_ButtonClick(const unsigned /*ctrl_id*/)
{
    SkipGFs();
}

void iwSkipGFs::Msg_EditEnter(const unsigned /*ctrl_id*/)
{
    SkipGFs();
}
