// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dskMenuBase.h"
#include "Loader.h"
#include "RTTR_Version.h"
#include "ogl/FontStyle.h"
#include "s25util/colors.h"

dskMenuBase::dskMenuBase() : Desktop(LOADER.getImageN("menu", 0))
{
    addBottomTexts();
}

dskMenuBase::dskMenuBase(glArchivItem_Bitmap* background) : Desktop(background)
{
    addBottomTexts();
}

void dskMenuBase::addBottomTexts()
{
    addFormattedText(ID_txtVersion, DrawPoint(0, 600), "Return To The Roots - %1%", COLOR_YELLOW,
                     FontStyle::LEFT | FontStyle::BOTTOM, NormalFont)
      % rttr::version::getReadableVersion();
    addText(ID_txtURL, DrawPoint(400, 600), "http://www.siedler25.org", COLOR_GREEN,
            FontStyle::CENTER | FontStyle::BOTTOM, NormalFont);
    addFormattedText(ID_txtCopyright, DrawPoint(800, 600),
                     "\xC2\xA9"
                     " 2005 - %s Settlers Freaks",
                     COLOR_YELLOW, FontStyle::RIGHT | FontStyle::BOTTOM, NormalFont)
      % rttr::version::getYear();
}
