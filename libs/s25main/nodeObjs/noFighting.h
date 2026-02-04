// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "noBase.h"
#include "gameData/BuildingConsts.h"
#include <array>
#include <memory>

class nofActiveSoldier;
class SerializedGameData;
class GameEvent;

/// Kampf an einem Punkt zwischen 2 Soldaten, der erstgenannt ist immer der, der links steht
class noFighting : public noBase
{
    enum class CombatRole
    {
        Attacker,
        Defender,
        Unknown
    };

    /// die kämpfenden Soldaten
    std::array<std::unique_ptr<nofActiveSoldier>, 2> soldiers;
    std::array<CombatRole, 2> roles_{CombatRole::Unknown, CombatRole::Unknown};
    std::array<unsigned, 2> startHitpoints_{};
    BuildingType targetBuildingType_ = BuildingType::Headquarters;
    unsigned targetBuildingObjId_ = 0;
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
    void InitCombatLogState();

public:
    noFighting(nofActiveSoldier& soldier1, nofActiveSoldier& soldier2);
    noFighting(SerializedGameData& sgd, unsigned obj_id);
    ~noFighting() override;

    void Destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::Fighting; }

    void Draw(DrawPoint drawPt) override;
    void HandleEvent(unsigned id) override;

    /// Dürfen andern Figuren diesen Kampf schon durchqueren?
    bool IsActive() const;
    bool IsFighter(const nofActiveSoldier& as) const { return &as == soldiers[0].get() || &as == soldiers[1].get(); }

    /// Prüfen, ob ein Soldat von einem bestimmten Spieler in den Kampf verwickelt ist
    bool IsSoldierOfPlayer(unsigned char player) const;
};
