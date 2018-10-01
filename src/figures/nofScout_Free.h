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
#ifndef NOF_SCOUT_FREE_H_
#define NOF_SCOUT_FREE_H_

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
    void HandleDerivedEvent(const unsigned id) override;

    /// Erkundet (quasi ein Umherirren)
    void Scout();

    /// Sucht einen neuen Zielpunkt und geht zu diesen
    void GoToNewNode();

    /// Gibt den Sichtradius dieser Figur zurück (0, falls nicht-spähend)
    unsigned GetVisualRange() const override;

public:
    nofScout_Free(const MapPoint pt, const unsigned char player, noRoadNode* goal);
    nofScout_Free(SerializedGameData& sgd, const unsigned obj_id);

    /// Serialisierungsfunktionen
protected:
    void Serialize_nofScout_Free(SerializedGameData& sgd) const;

public:
    void Serialize(SerializedGameData& sgd) const override { Serialize_nofScout_Free(sgd); }

    GO_Type GetGOT() const override { return GOT_NOF_SCOUT_FREE; }

    void Draw(DrawPoint drawPt) override;

    /// Wird aufgerufen, wenn die Flagge abgerissen wurde
    void LostWork() override;

    ///// Ist der Erkunder am erkunden (Sichtbereich um ihn herum)?
    // bool IsScouting() const { return (state == STATE_SCOUT_SCOUTING || state == STATE_GOTOFLAG); }
};

#endif //! NOF_SCOUT_FREE_H_
