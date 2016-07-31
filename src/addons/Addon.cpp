// Copyright (c) 2005 - 2016 Settlers Freaks (sf-team at siedler25.org)
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
#include "Addon.h"
#include "Loader.h"
#include "Window.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlText.h"
#include "helpers/containerUtils.h"
#include "libutil/src/colors.h"

void Addon::hideGui(Window* window, unsigned int id) const
{
    ctrlText* text = window->GetCtrl<ctrlText>(id);
    if(text)
        text->SetVisible(false);

    ctrlImageButton* button = window->GetCtrl<ctrlImageButton>(id + 1);
    if(button)
        button->SetVisible(false);
}

void Addon::createGui(Window* window, unsigned int id, unsigned short& y, bool  /*readonly*/, unsigned int  /*status*/) const //-V669
{
    ctrlText* text = window->GetCtrl<ctrlText>(id);
    if(!text)
        text = window->AddText(id, 52, y + 4, name_, COLOR_YELLOW, 0, NormalFont);

    text->SetVisible(true);
    text->Move(52, y + 4);

    ctrlImageButton* button = window->GetCtrl<ctrlImageButton>(id + 1);
    if(!button)
        button = window->AddImageButton(id + 1, 20, y, 22, 22, TC_GREY, LOADER.GetImageN("io", 21), description_);

    button->SetVisible(true);
    button->Move(20, y);
}

unsigned int Addon::getGuiStatus(Window* /*window*/, unsigned int /*id*/, bool& failed) const
{
    failed = false;
    return getDefaultStatus();
}
