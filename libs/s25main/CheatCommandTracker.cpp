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
const auto enableCheatsStr = makeCircularBuffer("winter");
} // namespace

CheatCommandTracker::CheatCommandTracker(Cheats& cheats) : cheats_(cheats), lastChars_(enableCheatsStr.size()) {}

void CheatCommandTracker::onKeyEvent(const KeyEvent& ke)
{
    if(!cheats_.areCheatsAllowed())
        return;

    if(ke.kt == KeyType::Char)
        onCharKeyEvent(ke);
    else
    {
        onSpecialKeyEvent(ke);
        lastChars_.clear();
    }
}

void CheatCommandTracker::onChatCommand(const std::string& cmd)
{
    if(!cheats_.areCheatsAllowed())
        return;

    if(cmd == "apocalypsis")
        cheats_.armageddon();
    else if(cmd == "impulse9")
        cheats_.toggleAllBuildingsEnabled();
    else if(cmd == "iq")
        cheats_.toggleShowEnemyProductivityOverlay();
}

void CheatCommandTracker::onSpecialKeyEvent(const KeyEvent& ke)
{
    switch(ke.kt)
    {
        case KeyType::F7: cheats_.toggleAllVisible(); break;
        case KeyType::F10: cheats_.toggleHumanAIPlayer(); break;
        default: break;
    }
}

void CheatCommandTracker::onCharKeyEvent(const KeyEvent& ke)
{
    // Handle only ASCII chars
    if(ke.c > 0x7F)
        lastChars_.clear();
    else
    {
        lastChars_.push_back(static_cast<char>(ke.c));

        if(lastChars_ == enableCheatsStr)
            cheats_.toggleCheatMode();
    }
}
