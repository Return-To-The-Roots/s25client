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

#include "IngameWindow.h"
#include "gameTypes/PactTypes.h"

class GameCommandFactory;
class GamePlayer;
class GameWorldViewer;

/// Diplomatiefenster: Übersicht über alle Spieler im Spiel und Schmieden von Bündnissen
class iwDiplomacy : public IngameWindow
{
public:
    iwDiplomacy(const GameWorldViewer& gwv, GameCommandFactory& gcFactory);

private:
    const GameWorldViewer& gwv;
    GameCommandFactory& gcFactory;
    void Msg_PaintBefore() override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
};

/// Kleines Fenster, über das einem Spieler ein neues Bündnis angeboten werden kann
class iwSuggestPact : public IngameWindow
{
    const PactType pt;
    const GamePlayer& player;
    GameCommandFactory& gcFactory;

public:
    iwSuggestPact(PactType pt, const GamePlayer& player, GameCommandFactory& gcFactory);

    void Msg_ButtonClick(unsigned ctrl_id) override;
};
