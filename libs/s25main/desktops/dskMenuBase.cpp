// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
    AddFormattedText(ID_txtVersion, DrawPoint(0, 600), "Return To The Roots - %1%", COLOR_YELLOW,
                     FontStyle::LEFT | FontStyle::BOTTOM, NormalFont)
      % RTTR_Version::GetReadableVersion();
    AddText(ID_txtURL, DrawPoint(400, 600), "http://www.siedler25.org", COLOR_GREEN,
            FontStyle::CENTER | FontStyle::BOTTOM, NormalFont);
    AddFormattedText(ID_txtCopyright, DrawPoint(800, 600),
                     "\xC2\xA9"
                     " 2005 - %s Settlers Freaks",
                     COLOR_YELLOW, FontStyle::RIGHT | FontStyle::BOTTOM, NormalFont)
      % RTTR_Version::GetYear();
}
