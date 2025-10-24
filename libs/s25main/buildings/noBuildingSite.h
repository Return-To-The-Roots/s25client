// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "noBaseBuilding.h"
#include "gameTypes/GoodTypes.h"
#include <cstdint>
#include <list>

class nofBuilder;
class nofPlaner;
class FOWObject;
class SerializedGameData;
class Ware;
class noFigure;
class noRoadNode;

/// Represents a construction site
class noBuildingSite : public noBaseBuilding
{
    enum class BuildingSiteState : uint8_t
    {
        Planing,
        Building
    };
    friend constexpr auto maxEnumValue(BuildingSiteState) { return noBuildingSite::BuildingSiteState::Building; }
    friend class nofBuilder;

    /// Construction state
    BuildingSiteState state;
    /// Planner
    nofPlaner* planer;
    /// Builder assigned to this site
    nofBuilder* builder;
    /// Planks and stones currently stored here
    unsigned char boards, stones;
    /// Planks and stones already used
    unsigned char used_boards, used_stones;
    /// Tracks construction progress in eight steps per ware used
    unsigned char build_progress;
    /// Ordered planks and stones that are on the way
    std::list<Ware*> ordered_boards, ordered_stones;

public:
    unsigned char getOrderedBoards() const { return ordered_boards.size(); }
    unsigned char getOrderedStones() const { return ordered_stones.size(); }
    unsigned char getUsedBoards() const { return used_boards; }
    unsigned char getUsedStones() const { return used_stones; }
    unsigned char getBoards() const { return boards; }
    unsigned char getStones() const { return stones; }

    noBuildingSite(BuildingType type, MapPoint pos, unsigned char player);
    /// Constructor for harbor sites created from a ship
    noBuildingSite(MapPoint pos, unsigned char player);
    noBuildingSite(SerializedGameData& sgd, unsigned obj_id);

    ~noBuildingSite() override;

    void Destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::Buildingsite; }
    unsigned GetMilitaryRadius() const override;

    void Draw(DrawPoint drawPt) override;

    /// Creates a fog-of-war memory object for the site
    std::unique_ptr<FOWObject> CreateFOWObject() const override;

    void AddWare(std::unique_ptr<Ware> ware) override;
    void GotWorker(Job job, noFigure& worker) override;

    /// Requests building materials
    void OrderConstructionMaterial();
    /// Called when the builder resigns
    void Abrogate();
    /// A requested ware could not arrive
    void WareLost(Ware& ware) override;
    /// Return the construction progress
    unsigned char GetBuildProgress(bool percent = true) const;

    unsigned CalcDistributionPoints(GoodType goodtype);

    /// Called when a new ware is delivered to the site (not when it is merely ordered)
    void TakeWare(Ware* ware) override;
    /// Return whether the construction is complete
    bool IsBuildingComplete();

    /// Called when the leveling work finishes
    void PlaningFinished();
    /// Return whether the site was started from a ship
    bool IsHarborBuildingSiteFromSea() const;
};
