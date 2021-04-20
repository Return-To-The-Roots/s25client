// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwPleaseWait.h"
#include "Loader.h"
#include "WindowManager.h"
#include "ogl/FontStyle.h"
#include "gameData/const_gui_ids.h"

/**
 *  Konstruktor von @p iwPleaseWait.
 *
 *  Fenster wird modal geöffnet, damit man ggf. einen "Weiter"-Button nicht
 *  mehrfach betätigen kann.
 */
iwPleaseWait::iwPleaseWait()
    : IngameWindow(CGI_PLEASEWAIT, IngameWindow::posLastOrCenter, Extent(300, 60), _("Please wait..."),
                   LOADER.GetImageN("resource", 41), true, false)
{
    WINDOWMANAGER.SetCursor(Cursor::Moon);
    AddText(0, GetSize() / 2, _("Please wait..."), COLOR_YELLOW, FontStyle::CENTER | FontStyle::VCENTER, NormalFont);
}

iwPleaseWait::~iwPleaseWait()
{
    WINDOWMANAGER.SetCursor();
}
