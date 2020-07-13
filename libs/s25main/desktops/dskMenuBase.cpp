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

#include "dskMenuBase.h"
#include "Loader.h"
#include "RTTR_Version.h"
#include "ogl/FontStyle.h"
#include "s25util/colors.h"

dskMenuBase::dskMenuBase() : Desktop(LOADER.GetImageN("menu", 0))
{
    AddBottomTexts();
}

dskMenuBase::dskMenuBase(glArchivItem_Bitmap* background) : Desktop(background)
{
    AddBottomTexts();
}

void dskMenuBase::AddBottomTexts()
{
    AddFormattedText(ID_txtVersion, DrawPoint(0, 600), "Return To The Roots - %1%", COLOR_YELLOW, FontStyle::LEFT | FontStyle::BOTTOM,
                     NormalFont)
      % RTTR_Version::GetReadableVersion();
    AddText(ID_txtURL, DrawPoint(400, 600), "http://www.siedler25.org", COLOR_GREEN, FontStyle::CENTER | FontStyle::BOTTOM, NormalFont);
    AddFormattedText(ID_txtCopyright, DrawPoint(800, 600),
                     "\xC2\xA9"
                     " 2005 - %s Settlers Freaks",
                     COLOR_YELLOW, FontStyle::RIGHT | FontStyle::BOTTOM, NormalFont)
      % RTTR_Version::GetYear();
}
