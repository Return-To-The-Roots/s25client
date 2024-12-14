// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

class GameWorldBase;

class Cheats
{
public:
    Cheats(GameWorldBase& world);

    void toggleCheatMode();
    bool isCheatModeOn() const { return isCheatModeOn_; }

    void toggleHumanAIPlayer() const;
    void armageddon() const;

private:
    bool areCheatsAllowed() const;

    bool isCheatModeOn_ = false;
    GameWorldBase& world_;
};
