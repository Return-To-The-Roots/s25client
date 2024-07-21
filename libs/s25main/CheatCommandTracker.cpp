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

    if(checkSpeedKeyEvent(ke))
        lastChars_.clear();
    else if(ke.kt == KeyType::Char)
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
    else if(cmd == "spies")
        cheats_.toggleShowEnemyProductivityOverlay();
}

void CheatCommandTracker::onSpecialKeyEvent(const KeyEvent& ke)
{
    if(ke.ctrl && ke.shift)
    {
        if(ke.kt >= KeyType::F1 && ke.kt <= KeyType::F8)
            cheats_.destroyBuildings({static_cast<unsigned>(ke.kt) - static_cast<unsigned>(KeyType::F1)});
        else if(ke.kt == KeyType::F9)
            cheats_.destroyAllAIBuildings();

        return;
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
}

bool CheatCommandTracker::checkSpeedKeyEvent(const KeyEvent& ke)
{
    const char32_t c = ke.c;
    if(ke.alt && c >= '1' && c <= '6')
    {
        cheats_.setGameSpeed(c - '1');
        return true;
    }
    return false;
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
