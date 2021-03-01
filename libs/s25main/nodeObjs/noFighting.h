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

#include "noBase.h"

class nofActiveSoldier;
class SerializedGameData;
class GameEvent;

/// Kampf an einem Punkt zwischen 2 Soldaten, der erstgenannt ist immer der, der links steht
class noFighting : public noBase
{
    /// die kämpfenden Soldaten
    nofActiveSoldier* soldiers[2];
    // Wer ist an der Reihe mit angreifen (2 = Beginn des Kampfes)
    unsigned char turn;
    /// Verteidigungsanimation (3 = keine Verteidigung,  Treffer)
    unsigned char defending_animation;
    /// Event
    const GameEvent* current_ev;
    /// Spieler des Soldaten, der gewonnen hat
    unsigned char player_won;

private:
    /// Bestimmt, ob der Angreifer erfolgreich angreift oder ob der Verteidiger sich verteidigt usw
    /// bereitet also alles für eine solche Angrifsseinheit vor
    void StartAttack();

public:
    noFighting(nofActiveSoldier* soldier1, nofActiveSoldier* soldier2);
    noFighting(SerializedGameData& sgd, unsigned obj_id);
    ~noFighting() override;

    void Destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::Fighting; }

    void Draw(DrawPoint drawPt) override;
    void HandleEvent(unsigned id) override;

    /// Dürfen andern Figuren diesen Kampf schon durchqueren?
    bool IsActive() const;
    bool IsFighter(nofActiveSoldier* as) { return as == soldiers[0] || as == soldiers[1]; }

    /// Prüfen, ob ein Soldat von einem bestimmten Spieler in den Kampf verwickelt ist
    bool IsSoldierOfPlayer(unsigned char player) const;
};
