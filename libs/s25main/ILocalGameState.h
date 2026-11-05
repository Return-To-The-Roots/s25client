// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

class ILocalGameState
{
public:
    /// Get the local player id
    virtual unsigned GetPlayerId() const = 0;
    /// Return true if the local player is the host
    virtual bool IsHost() const = 0;
    /// Convert a number of GameFrames into real time (HH:MM:SS or MM:SS if hours = 0)
    virtual std::string FormatGFTime(unsigned numGFs) const = 0;
    /// Send a chat message to the local player
    virtual void SystemChat(const std::string& text) = 0;
};

/// For headless contexts: acts as the host playing as player 0 and discards all output.
struct NullLocalGameState : ILocalGameState
{
    unsigned GetPlayerId() const override { return 0; }
    bool IsHost() const override { return true; }
    std::string FormatGFTime(unsigned) const override { return ""; }
    void SystemChat(const std::string&) override {}
};
