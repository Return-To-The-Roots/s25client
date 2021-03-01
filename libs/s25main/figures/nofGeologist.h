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

#include "helpers/EnumArray.h"
#include "helpers/OptionalEnum.h"
#include "nofFlagWorker.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/Resource.h"
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

    helpers::EnumArray<bool, ResourceType> resAlreadyFound;

    void GoalReached() override;
    void Walked() override;
    void HandleDerivedEvent(unsigned id) override;

    /// Kann man an diesem Punkt ein Schild aufstellen?
    bool IsNodeGood(MapPoint pt) const;
    /// Sucht im Umkreis von der Flagge neue Punkte wo man graben könnte
    void LookForNewNodes();
    /// Checks if the node is valid as a new target
    inline bool IsValidTargetNode(MapPoint pt) const;
    /// Searches for a new point to go to. Returns the direction to walk in if any point was found, else sets node_goal
    /// to invalid
    helpers::OptionalEnum<Direction> GetNextNode();
    /// Sucht sich einen neuen Punkt und geht dorthin oder geht wieder nach Hause wenn alle Schilder aufgestellt wurden
    /// oder es keinen Punkt mehr gibt
    void GoToNextNode();
    /// Setzt das Schild, wenn noch was frei ist
    void SetSign(Resource resources);

    bool IsSignInArea(ResourceType type) const;

public:
    nofGeologist(MapPoint pos, unsigned char player, noRoadNode* goal);
    nofGeologist(SerializedGameData& sgd, unsigned obj_id);

    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::NofGeologist; }

    void Draw(DrawPoint drawPt) override;

    /// Wird aufgerufen, wenn die Flagge abgerissen wurde
    void LostWork() override;
};
