// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofFlagWorker.h"
#include "gameTypes/MapCoordinates.h"
class SerializedGameData;
class noRoadNode;

/// Frei herumlaufender Erkunder
class nofScout_Free : public nofFlagWorker
{
    /// Nächster Punkt, wo der Späher hingehen soll
    MapPoint nextPos;
    /// Weg, weit weit er noch laufen soll
    unsigned rest_way;

private:
    void GoalReached() override;
    void Walked() override;
    void HandleDerivedEvent(unsigned id) override;

    /// Erkundet (quasi ein Umherirren)
    void Scout();

    /// Sucht einen neuen Zielpunkt und geht zu diesen
    void GoToNewNode();

    /// Gibt den Sichtradius dieser Figur zurück (0, falls nicht-spähend)
    unsigned GetVisualRange() const override;

public:
    nofScout_Free(MapPoint pos, unsigned char player, noRoadNode* goal);
    nofScout_Free(SerializedGameData& sgd, unsigned obj_id);

    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::NofScoutFree; }

    void Draw(DrawPoint drawPt) override;

    /// Wird aufgerufen, wenn die Flagge abgerissen wurde
    void LostWork() override;

    ///// Ist der Erkunder am erkunden (Sichtbereich um ihn herum)?
    // bool IsScouting() const { return (state == ScoutScouting || state == GoToFlag); }
};
