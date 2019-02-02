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

#ifndef NOF_GEOLOGIST_H_
#define NOF_GEOLOGIST_H_

#include "nofFlagWorker.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/Resource.h"
#include <array>
#include <vector>

class SerializedGameData;
class noRoadNode;

class nofGeologist : public nofFlagWorker
{
private:
    /// Schilder, die er noch aufstellen sollte (max 15 abarbeiten)
    unsigned short signs;

    std::vector<MapPoint> available_nodes;
    /// Punkt, zu dem er gerade geht
    MapPoint node_goal;

    /// maximaler Radius wie weit die Geologen sich von der Flagge entfernen würde
    static const unsigned short MAX_RADIUS = 10;

    std::array<bool, Resource::TypeCount> resAlreadyFound;

private:
    void GoalReached() override;
    void Walked() override;
    void HandleDerivedEvent(unsigned id) override;

    /// Kann man an diesem Punkt ein Schild aufstellen?
    bool IsNodeGood(const MapPoint pt) const;
    /// Sucht im Umkreis von der Flagge neue Punkte wo man graben könnte
    void LookForNewNodes();
    /// Checks if the node is valid as a new target
    inline bool IsValidTargetNode(const MapPoint pt) const;
    /// Bestimmt einen neuen Punkt,wo man hingehen kann, falls es keinen mehr gibt, wird ein ungültiger
    /// Iterator gesetzt, liefert die Richtung in die man zum Punkt gehen muss, zurück
    unsigned char GetNextNode();
    /// Sucht sich einen neuen Punkt und geht dorthin oder geht wieder nach Hause wenn alle Schilder aufgestellt wurden
    /// oder es keinen Punkt mehr gibt
    void GoToNextNode();
    /// Setzt das Schild, wenn noch was frei ist
    void SetSign(Resource resources);

    bool IsSignInArea(Resource::Type type) const;

public:
    nofGeologist(const MapPoint pt, unsigned char player, noRoadNode* goal);
    nofGeologist(SerializedGameData& sgd, unsigned obj_id);

    /// Serialisierungsfunktionen
protected:
    void Serialize_nofGeologist(SerializedGameData& sgd) const;

public:
    void Serialize(SerializedGameData& sgd) const override { Serialize_nofGeologist(sgd); }

    GO_Type GetGOT() const override { return GOT_NOF_GEOLOGIST; }

    void Draw(DrawPoint drawPt) override;

    /// Wird aufgerufen, wenn die Flagge abgerissen wurde
    void LostWork() override;
};

#endif
