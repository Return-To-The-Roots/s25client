// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwSkipGFs.h"
#include "Loader.h"
#include "controls/ctrlEdit.h"
#include "network/GameClient.h"
#include "gameData/const_gui_ids.h"
#include "s25util/StringConversion.h"
#include "s25util/colors.h"

iwSkipGFs::iwSkipGFs(GameWorldView& gwv)
    : IngameWindow(CGI_SKIPGFS, IngameWindow::posLastOrCenter, Extent(300, 110), _("Skip GameFrames"),
                   LOADER.GetImageN("resource", 41)),
      gwv(gwv)
{
    // Text vor Editfeld
    AddText(0, DrawPoint(50, 36), _("to GameFrame:"), COLOR_YELLOW, FontStyle{}, NormalFont);

    // Editfeld zum Eingeben des Ziel-GF
    ctrlEdit* edit = AddEdit(1, DrawPoint(126, 32), Extent(120, 20), TextureColor::Grey, NormalFont);
    edit->SetFocus();

    // OK-Button
    AddTextButton(2, DrawPoint(110, 65), Extent(80, 22), TextureColor::Green2, _("OK"), NormalFont);
}

void iwSkipGFs::SkipGFs()
{
    int gf = s25util::fromStringClassicDef(GetCtrl<ctrlEdit>(1)->GetText(), 0);
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
