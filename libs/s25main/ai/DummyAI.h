// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AIPlayer.h"
class GameWorldBase;
class GlobalGameSettings;

/// Dummy AI that does nothing
class DummyAI final : public AIPlayer
{
public:
    DummyAI(unsigned char playerId, const GameWorldBase& gwb, const AI::Level level) : AIPlayer(playerId, gwb, level) {}

    void RunGF(unsigned /*gf*/, bool /*gfisnwf*/) override {}

    void OnChatMessage(unsigned /*sendPlayerId*/, ChatDestination, const std::string& /*msg*/) override {}
    void saveStats(unsigned int gf) const
    {
        gf = gf;
    }

};
