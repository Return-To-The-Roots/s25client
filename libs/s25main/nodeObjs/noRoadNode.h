// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "RoadSegment.h"
#include "helpers/EnumArray.h"
#include "noCoordBase.h"
#include "gameTypes/Direction.h"
#include "gameTypes/RoadPathDirection.h"

class Ware;
class SerializedGameData;

// Basisklasse für Gebäude und Flagge (alles, was als "Straßenknoten" dient
class noRoadNode : public noCoordBase
{
protected:
    unsigned char player;

private:
    helpers::EnumArray<RoadSegment*, Direction> routes;

public:
    // For Pathfinding
    // cost from start
    mutable unsigned cost; //-V730_NOINIT
    // distance to target
    mutable unsigned targetDistance; //-V730_NOINIT
    // estimated total distance (cost + distance)
    mutable unsigned estimate; //-V730_NOINIT
    mutable unsigned last_visit;
    mutable const noRoadNode* prev; //-V730_NOINIT
    /// Direction to previous node, includes SHIP_DIR
    mutable RoadPathDirection dir_; //-V730_NOINIT

    noRoadNode(NodalObjectType nop, MapPoint pos, unsigned char player);
    noRoadNode(SerializedGameData& sgd, unsigned obj_id);
    noRoadNode(const noRoadNode&) = delete;
    ~noRoadNode() override;

    void Destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    RoadSegment* GetRoute(const Direction dir) const { return routes[dir]; }
    void SetRoute(const Direction dir, RoadSegment* route) { routes[dir] = route; }
    const auto& getRoutes() const { return routes; }
    noRoadNode* GetNeighbour(Direction dir) const;

    void DestroyRoad(Direction dir);
    void UpgradeRoad(Direction dir) const;
    /// Vernichtet Alle Straße um diesen Knoten
    void DestroyAllRoads();

    unsigned char GetPlayer() const { return player; }

    /// Legt eine Ware am Objekt ab (an allen Straßenknoten (Gebäude, Baustellen und Flaggen) kann man Waren ablegen
    virtual void AddWare(std::unique_ptr<Ware> ware) = 0;

    /// Nur für Flagge, Gebäude können 0 zurückgeben, gibt Wegstrafpunkte für das Pathfinden für Waren, die in eine
    /// bestimmte Richtung noch transportiert werden müssen
    virtual unsigned GetPunishmentPoints(Direction) const { return 0; }
};

inline noRoadNode* noRoadNode::GetNeighbour(const Direction dir) const
{
    const RoadSegment* route = GetRoute(dir);
    if(!route)
        return nullptr;
    else if(route->GetF1() == this)
        return route->GetF2();
    else
        return route->GetF1();
}
