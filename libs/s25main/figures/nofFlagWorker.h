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

    void Destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    /// Wird aufgerufen, wenn die Flagge abgerissen wurde
    virtual void LostWork() = 0;

    /// Gibt Flagge zurück
    noFlag* GetFlag() const { return flag; }
};
