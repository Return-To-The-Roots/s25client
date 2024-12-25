// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dskCampaignVictory.h"
#include "Loader.h"
#include "Settings.h"
#include "WindowManager.h"
#include "desktops/dskMainMenu.h"

dskCampaignVictory::dskCampaignVictory()
    : Desktop(LOADER.GetImageN(ResourceId{SETTINGS.campaigns.getCompletedCampaign() ? "setup895" : "setup896"}, 0))
{
    if(!SETTINGS.campaigns.getCompletedCampaign())
        AddText(10, DrawPoint{800 / 2, 600 - 50},
                _("You have successfully completed chapter") + std::string{" "}
                  + std::to_string(*SETTINGS.campaigns.getCompletedChapter()) + ".",
                COLOR_YELLOW, FontStyle::CENTER, LargeFont);
    SETTINGS.campaigns.resetCompletionStatus();
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
