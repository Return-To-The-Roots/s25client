// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dskCampaignVictory.h"
#include "Loader.h"
#include "WindowManager.h"
#include "desktops/dskMainMenu.h"
#include "network/GameClient.h"

dskCampaignVictory::dskCampaignVictory(unsigned char chapter)
    : Desktop(LOADER.GetImageN(ResourceId{chapter ? "setup896" : "setup895"}, 0))
{
    GAMECLIENT.SetCampaignChapterCompleted(0);
    GAMECLIENT.SetCampaignCompleted(false);

    if(!chapter)
        return;

    AddText(10, DrawPoint{800 / 2, 600 - 50},
            _("You have successfully completed chapter") + std::string{" "} + std::to_string(chapter) + ".",
            COLOR_YELLOW, FontStyle::CENTER, LargeFont);
}

bool dskCampaignVictory::Msg_LeftDown(const MouseCoords&)
{
    return ShowMenu();
}

bool dskCampaignVictory::Msg_KeyDown(const KeyEvent&)
{
    return ShowMenu();
}

bool dskCampaignVictory::ShowMenu()
{
    WINDOWMANAGER.Switch(std::make_unique<dskMainMenu>());
    return true;
}
