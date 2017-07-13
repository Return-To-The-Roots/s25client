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

#include "defines.h" // IWYU pragma: keep
#include "dskTest.h"

#include "WindowManager.h"
#include "Loader.h"
#include "desktops/dskMainMenu.h"
#include "controls/ctrlButton.h"
#include "ogl/glArchivItem_Font.h"
#include "libutil/src/colors.h"

namespace{
    enum{
        ID_txtTitle = dskMenuBase::ID_FIRST_FREE,
        ID_grpBtStart,
        ID_grpBtEnd = ID_grpBtStart + 5 * 4,
        ID_btDisable,
        ID_btBack
    };
}

dskTest::dskTest()
{
    AddText(ID_txtTitle, 300, 20, _("Internal test screen for developers"), COLOR_ORANGE, glArchivItem_Font::DF_CENTER, LargeFont);
    boost::array<TextureColor, 4> textures = { { TC_GREEN1, TC_GREEN2, TC_RED1, TC_GREY } };
    boost::array<std::string, 4> labels = { { "Green1", "Green2", "Red1", "Grey" } };
    unsigned yPos = 50;
    unsigned curId = ID_grpBtStart;
    ctrlTextButton* bt;
    for(unsigned i = 0; i < textures.size(); i++){
        AddText(curId, 10, yPos + 3, labels.at(i), COLOR_YELLOW, glArchivItem_Font::DF_LEFT, NormalFont);
        bt = AddTextButton(curId + 1, 120, yPos, 95, 22, textures[i], "Nothing", NormalFont);
        bt->SetIlluminated(false);
        bt->SetBorder(false);
        bt = AddTextButton(curId + 2, 220, yPos, 95, 22, textures[i], "Illuminated", NormalFont);
        bt->SetIlluminated(true);
        bt->SetBorder(false);
        bt = AddTextButton(curId + 3, 320, yPos, 95, 22, textures[i], "Border", NormalFont);
        bt->SetIlluminated(false);
        bt->SetBorder(true);
        bt = AddTextButton(curId + 4, 420, yPos, 95, 22, textures[i], "Both", NormalFont);
        bt->SetIlluminated(true);
        bt->SetBorder(true);
        curId += 5;
        yPos += 30;
    }
    RTTR_Assert(curId == ID_grpBtEnd);

    AddTextButton(ID_btDisable, 10, 550, 200, 22, TC_GREEN1, "Enable/Disable buttons", NormalFont);
    AddTextButton(ID_btBack, 590, 550, 200, 22, TC_RED1, _("Back"), NormalFont);
}

void dskTest::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_btBack:
            WINDOWMANAGER.Switch(new dskMainMenu);
            break;
        case ID_btDisable:
            for(unsigned i = ID_grpBtStart; i < ID_grpBtEnd; i++)
            {
                ctrlTextButton* bt = GetCtrl<ctrlTextButton>(i);
                if(bt)
                    bt->SetEnabled(!bt->GetEnabled());
            }
            break;
    }
}

