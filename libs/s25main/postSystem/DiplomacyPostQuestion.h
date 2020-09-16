// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

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
