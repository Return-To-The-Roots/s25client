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
#include "ogl/glArchivItem_Font.h"
#include "libutil/src/colors.h"

namespace{
    enum{
        ID_txtTitle = dskMenuBase::ID_FIRST_FREE,
        ID_btBack
    };
}

dskTest::dskTest()
{
    AddText(ID_txtTitle, 300, 20, _("Internal test screen for developers"), COLOR_ORANGE, glArchivItem_Font::DF_CENTER, LargeFont);
    AddTextButton(ID_btBack, 300, 550, 200, 22, TC_RED1, _("Back"), NormalFont);
}

void dskTest::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_btBack:
            WINDOWMANAGER.Switch(new dskMainMenu);
            break;
    }
}

