// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "postSystem/PostMsg.h"
#include "gameTypes/PactTypes.h"

struct GamePlayerInfo;

/// Post message asking for a change in the diplomacy contracts. Includes Yes/No buttons
class DiplomacyPostQuestion : public PostMsg
{
public:
    /// Create contract
    DiplomacyPostQuestion(unsigned sendFrame, PactType pact, unsigned id, const GamePlayerInfo& otherPlayer,
                          int duration);
    /// Cancel contract
    DiplomacyPostQuestion(unsigned sendFrame, PactType pact, unsigned id, const GamePlayerInfo& otherPlayer);

    bool IsAccept() const { return acceptOrCancel; }
    PactType GetPactType() const { return pact; }
    unsigned GetPactId() const { return pactId; }
    unsigned GetPlayerId() const { return player; }

private:
    /// Type of the question. True for accepting a new contract, false for canceling existing one
    bool acceptOrCancel;
    /// Which pact this refers to
    PactType pact;
    /// ID of the contract
    unsigned pactId;
    /// Player ID who asked for the change
    unsigned char player;
};
