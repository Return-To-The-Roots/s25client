// Copyright (c) 2005 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

#include "iwVictory.h"
#include "Loader.h"
#include "WindowManager.h"
#include "controls/ctrlMultiline.h"
#include "iwEndgame.h"
#include "ogl/FontStyle.h"
#include "gameData/const_gui_ids.h"

namespace {
enum
{
    ID_TITLE,
    ID_NAMES,
    ID_CONTINUE,
    ID_END_GAME
};
}

iwVictory::iwVictory(const std::vector<std::string>& winnerNames)
    : IngameWindow(CGI_VICTORY, IngameWindow::posLastOrCenter, Extent(240, 240), _("End of game"),
                   LOADER.GetImageN("io", 5))
{
    AddText(ID_TITLE, DrawPoint(120, 40), winnerNames.size() > 1u ? _("The winners:") : _("The winner:"), COLOR_YELLOW,
            FontStyle::CENTER, NormalFont);
    auto* names = AddMultiline(ID_NAMES, DrawPoint(0, 70), Extent(800, 600), TextureColor::Invisible, NormalFont);
    names->SetNumVisibleLines(winnerNames.size());
    names->ShowBackground(false);
    names->SetScrollBarAllowed(false);
    for(const auto& name : winnerNames)
        names->AddString(name, COLOR_YELLOW);
    const auto txtBounds = Rect(names->GetPos(), names->GetContentSize());
    Extent iwSize = GetIwSize();
    if(iwSize.x + 20u < txtBounds.getSize().x)
        iwSize.x = txtBounds.getSize().x + 20u;
    if(static_cast<int>(iwSize.y + 60) < txtBounds.bottom)
        iwSize.y = txtBounds.bottom + 60u;
    if(iwSize != GetIwSize())
        SetIwSize(iwSize);
    names->SetPos(DrawPoint((GetSize().x - txtBounds.getSize().x) / 2, names->GetPos().y));
    int btPosY = std::max(170, txtBounds.bottom + 20);
    AddTextButton(ID_CONTINUE, DrawPoint(50, btPosY), Extent(140, 20), TextureColor::Grey, _("Continue game"),
                  NormalFont);
    AddTextButton(ID_END_GAME, DrawPoint(50, btPosY + 25), Extent(140, 20), TextureColor::Red1, _("End game"),
                  NormalFont);
}

void iwVictory::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_CONTINUE: Close(); break;
        case ID_END_GAME:
            WINDOWMANAGER.ReplaceWindow(std::make_unique<iwEndgame>());
            Close();
            break;
    }
}
