// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "figures/noFigure.h"

class noFlag;
class SerializedGameData;
class noRoadNode;

/// Basisklasse für Geologen und Späher, also die, die an eine Flagge gebunden sind zum Arbeiten
class nofFlagWorker : public noFigure
{
protected:
    /// Flaggen-Ausgangspunkt
    noFlag* flag;

    enum class State : uint8_t
    {
        FigureWork, // Zur Flagge und zurückgehen, Rumirren usw
        GoToFlag,   // geht zurück zur Flagge um anschließend nach Hause zu gehen

        GeologistGotoNextNode, // Zum nächsten Punkt gehen, um dort zu graben
        GeologistDig,          // graben (mit Hammer auf Berg hauen)
        GeologistCheer,        // Jubeln, dass man etwas gefunden hat

        ScoutScouting // läuft umher und erkundet
    } state;
    friend constexpr auto maxEnumValue(State) { return State::ScoutScouting; }

    /// Kündigt bei der Flagge
    void AbrogateWorkplace() override;
    /// Geht wieder zurück zur Flagge und dann nach Hause
    void GoToFlag();

public:
    nofFlagWorker(Job job, MapPoint pos, unsigned char player, noRoadNode* goal);
    nofFlagWorker(SerializedGameData& sgd, unsigned obj_id);
    nofFlagWorker(const nofFlagWorker&) = delete;

    void Destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    /// Wird aufgerufen, wenn die Flagge abgerissen wurde
    virtual void LostWork() = 0;

    /// Gibt Flagge zurück
    noFlag* GetFlag() const { return flag; }
};
