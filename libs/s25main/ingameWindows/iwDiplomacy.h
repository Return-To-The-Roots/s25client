// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
