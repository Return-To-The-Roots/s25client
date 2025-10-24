// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/EnumArray.h"
#include "nobBaseWarehouse.h"
#include "gameData/MilitaryConsts.h"
#include <list>

class noShip;
class SerializedGameData;
class Ware;
class noBaseBuilding;
class noFigure;
class noRoadNode;
class nobMilitary;
class nofAttacker;
class nofDefender;
class GameEvent;

class nobHarborBuilding : public nobBaseWarehouse
{
    struct ExpeditionInfo
    {
        ExpeditionInfo() : boards(0), stones(0), active(false), builder(false) {}
        ExpeditionInfo(SerializedGameData& sgd);
        void Serialize(SerializedGameData& sgd) const;

        /// Number of planks and stones already gathered
        unsigned boards, stones;
        /// Is the expedition currently being prepared?
        bool active;
        /// Has the builder arrived?
        bool builder;
    } expedition;

    struct ExplorationExpeditionInfo
    {
        ExplorationExpeditionInfo() : active(false), scouts(0) {}
        ExplorationExpeditionInfo(SerializedGameData& sgd);
        void Serialize(SerializedGameData& sgd) const;

        /// Is the exploration expedition being prepared?
        bool active;
        /// Number of scouts already present
        unsigned scouts;
    } exploration_expedition;

    /// Event that orders additional wares
    const GameEvent* orderware_ev;
    /// Sea IDs of all adjacent waters (for the six surrounding shoreline points)
    helpers::EnumArray<uint16_t, Direction> seaIds;
    /// Wares queued for shipping
    std::list<std::unique_ptr<Ware>> wares_for_ships;
    /// Figures queued for shipping
    struct FigureForShip
    {
        std::unique_ptr<noFigure> fig;
        MapPoint dest;
    };
    std::list<FigureForShip> figures_for_ships;

    /// Attacking soldiers waiting to embark
    struct SoldierForShip
    {
        std::unique_ptr<nofAttacker> attacker;
        MapPoint dest;
    };
    std::list<SoldierForShip> soldiers_for_ships;

private:
    /// Order the additional wares required for an expedition
    void OrderExpeditionWares();
    /// Check whether the expedition has all wares and call a ship if ready
    void CheckExpeditionReady();
    /// Check whether the exploration expedition has all scouts and call a ship if ready
    void CheckExplorationExpeditionReady();
    /// Return whether the expedition is ready
    bool IsExpeditionReady() const;
    /// Return whether the exploration expedition is ready
    bool IsExplorationExpeditionReady() const;
    /// Allow derived classes to consume freshly created wares immediately (return true if consumed)
    bool UseWareAtOnce(std::unique_ptr<Ware>& ware, noBaseBuilding& goal) override;
    /// Same logic for figures
    bool UseFigureAtOnce(std::unique_ptr<noFigure>& fig, noRoadNode& goal) override;
    /// Cancel a queued figure that can no longer arrive
    void CancelFigure(noFigure* figure) override;
    /// Request a ship for the harbor if needed
    void OrderShip();

    /// Provide a defender for the harbor
    std::unique_ptr<nofDefender> ProvideDefender(nofAttacker& attacker) override;

    friend class SerializedGameData;
    friend class BuildingFactory;
    nobHarborBuilding(MapPoint pos, unsigned char player, Nation nation);
    nobHarborBuilding(SerializedGameData& sgd, unsigned obj_id);

protected:
    void DestroyBuilding() override;

public:
    unsigned GetMilitaryRadius() const override { return HARBOR_RADIUS; }

    /// Serialize harbor-specific data
    void Serialize(SerializedGameData& sgd) const override;
    GO_Type GetGOT() const final { return GO_Type::NobHarborbuilding; }
    void Draw(DrawPoint drawPt) override;
    void HandleEvent(unsigned id) override;

    /// Called when an ordered ware fails to arrive
    void WareLost(Ware& ware) override;
    /// Store a ware inside the harbor warehouse
    void AddWare(std::unique_ptr<Ware> ware) override;
    /// Add a figure to the harbor warehouse
    void AddFigure(std::unique_ptr<noFigure> figure, bool increase_visual_counts) override;
    /// Compute the priority of a ware for the harbor (expeditions consume specific goods)
    unsigned CalcDistributionPoints(GoodType type) const;

    /// Cancel the shipment of a specific ware that was slated for transport
    std::unique_ptr<Ware> CancelWareForShip(Ware* ware);

    /// Start gathering resources for an expedition
    void StartExpedition();
    void StopExpedition();

    /// Start gathering resources for an exploration expedition
    void StartExplorationExpedition();
    void StopExplorationExpedition();

    /// Is a regular expedition currently being prepared?
    bool IsExpeditionActive() const { return expedition.active; }
    /// Is an exploration expedition currently being prepared?
    bool IsExplorationExpeditionActive() const { return exploration_expedition.active; }
    /// Notify that a ship arrived
    void ShipArrived(noShip& ship);
    /// Notify that a ship failed to arrive
    void ShipLost(noShip* ship);

    /// Handle figures that can no longer reach the harbor (e.g., reorder a needed builder)
    void RemoveDependentFigure(noFigure& figure) override;

    /// Return the harbor spot ID occupied by this harbor
    unsigned GetHarborPosID() const;

    struct ShipConnection
    {
        /// Destination harbor
        noRoadNode* dest;
        /// Travel cost in carrier-distance units
        unsigned way_costs;
    };
    /// Return all viable ship connections
    std::vector<ShipConnection> GetShipConnections() const;

    /// Queue a figure for transport to a target destination
    void AddFigureForShip(std::unique_ptr<noFigure> fig, MapPoint dest);
    /// Queue a ware for transport to a target destination
    void AddWareForShip(std::unique_ptr<Ware> ware);

    /// A ware changed its route and doesn't want to use the ship anymore
    void WareDontWantToTravelByShip(Ware* ware);

    /// Return how many ships are still needed for pending tasks
    unsigned GetNumNeededShips() const;
    /// Rate how urgently an additional ship is required (0 = no demand)
    int GetNeedForShip(unsigned ships_coming) const;

    /// Receive goods and figures from a ship and store them
    void ReceiveGoodsFromShip(std::list<std::unique_ptr<noFigure>>& figures, std::list<std::unique_ptr<Ware>>& wares);

    nofAggressiveDefender* SendAggressiveDefender(nofAttacker& attacker) override;

    struct SeaAttackerBuilding
    {
        /// Comparator that compares only the building pointer
        struct CmpBuilding
        {
            const nobMilitary* const search;
            CmpBuilding(const nobMilitary* const search) : search(search) {}
            bool operator()(const SeaAttackerBuilding& other) const { return other.building == search; }
        };
        /// The military building itself
        nobMilitary* building;
        // Associated harbor where attackers should wait for the ship
        nobHarborBuilding* harbor;
        /// Distance between the source harbor and the target harbor
        unsigned distance;

        bool operator==(const nobMilitary* const building) const { return (this->building == building); };
    };

    /// Return the attackers this harbor can provide for a sea assault
    /// `defender_harbors` lists possible target harbors
    std::vector<SeaAttackerBuilding> GetAttackerBuildingsForSeaAttack(const std::vector<unsigned>& defender_harbors);
    /// Gibt verfügbare Angreifer zurück
    std::vector<SeaAttackerBuilding> GetAttackerBuildingsForSeaIdAttack();

    /// Fügt einen Schiffs-Angreifer zum Hafen hinzu
    void AddSeaAttacker(std::unique_ptr<nofAttacker> attacker);
    /// Attacker does not want to attack anymore
    void CancelSeaAttacker(nofAttacker* attacker);

    /// People waiting for a ship have to examine their route if a road was destroyed
    void ExamineShipRouteOfPeople();

    /// Is the harbor just being destroyed right now?
    bool IsBeingDestroyedNow() const;
};
