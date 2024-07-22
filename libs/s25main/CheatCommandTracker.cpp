// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CheatCommandTracker.h"
#include "Cheats.h"
#include "driver/KeyEvent.h"

namespace {
auto makeCircularBuffer(const std::string& str)
{
    return boost::circular_buffer<char>{cbegin(str), cend(str)};
}
const auto cheatStr = makeCircularBuffer("winter");
} // namespace

CheatCommandTracker::CheatCommandTracker(Cheats& cheats) : cheats_(cheats), lastChars_(cheatStr.size()) {}

void CheatCommandTracker::trackKeyEvent(const KeyEvent& ke)
{
    if(trackSpecialKeyEvent(ke))
    {
        lastChars_.clear();
        return;
    }

    trackCharKeyEvent(ke);
}

void CheatCommandTracker::trackChatCommand(const std::string& cmd)
{
    if(cmd == "apocalypsis")
        cheats_.armageddon();
}

bool CheatCommandTracker::trackSpecialKeyEvent(const KeyEvent& ke)
{
    if(ke.kt == KeyType::Char)
        return false;

    if(ke.ctrl && ke.shift)
    {
        if(ke.kt >= KeyType::F1 && ke.kt <= KeyType::F8)
            cheats_.destroyBuildings({static_cast<unsigned>(ke.kt) - static_cast<unsigned>(KeyType::F1)});
        else if(ke.kt == KeyType::F9)
            cheats_.destroyAllAIBuildings();

        return true;
    }

    switch(ke.kt)
    {
        case KeyType::F7:
        {
            if(ke.alt)
                cheats_.toggleResourceRevealMode();
            else
                cheats_.toggleAllVisible();
        }
        break;
        case KeyType::F10: cheats_.toggleHumanAIPlayer(); break;
        default: break;
    }

    return true;
}

bool CheatCommandTracker::trackCharKeyEvent(const KeyEvent& ke)
{
    lastChars_.push_back(ke.c);

    if(lastChars_ == cheatStr)
        cheats_.toggleCheatMode();

    return true;
}
