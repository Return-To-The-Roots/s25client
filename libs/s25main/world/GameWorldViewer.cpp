// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "world/GameWorldViewer.h"
#include "Cheats.h"
#include "GameInterface.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "RttrForeachPt.h"
#include "buildings/nobMilitary.h"
#include "network/GameClient.h"
#include "notifications/NodeNote.h"
#include "notifications/PlayerNodeNote.h"
#include "notifications/RoadNote.h"
#include "world/BQCalculator.h"
#include "world/GameWorldBase.h"
#include "nodeObjs/noShip.h"
#include "gameTypes/MapCoordinates.h"
#include "gameData/BuildingProperties.h"

GameWorldViewer::GameWorldViewer(unsigned playerId, GameWorldBase& gwb) : playerId_(playerId), gwb(gwb)
{
    InitVisualData();
    maxNodeAltitude_ = 0;
    RTTR_FOREACH_PT(MapPoint, gwb.getSize())
    {
        maxNodeAltitude_ = std::max(maxNodeAltitude_, gwb.getNode(pt).altitude);
    }
    evAltitudeChanged = gwb.getNotifications().subscribe<NodeNote>([this](const NodeNote& note) {
        if(note.type == NodeNote::Altitude)
        {
            maxNodeAltitude_ = std::max(maxNodeAltitude_, this->gwb.getNode(note.pos).altitude);
        }
    });
}

void GameWorldViewer::InitVisualData()
{
    visualNodes.Resize(gwb.getSize());
    RTTR_FOREACH_PT(MapPoint, gwb.getSize())
    {
        VisualMapNode& vNode = visualNodes[pt];
        const MapNode& node = gwb.getNode(pt);
        vNode.bq = node.bq;
        // Roads are only overlays. At first we don't have any -> PointRoad::None=use real road
        std::fill(vNode.roads.begin(), vNode.roads.end(), PointRoad::None);
    }
    evRoadConstruction =
      gwb.getNotifications().subscribe<RoadNote>([this](const RoadNote& note) { RoadConstructionEnded(note); });
    evBQChanged = gwb.getNotifications().subscribe<NodeNote>([this](const NodeNote& note) {
        if(note.type == NodeNote::BQ)
            RecalcBQ(note.pos);
    });
}

void GameWorldViewer::InitTerrainRenderer()
{
    tr.GenerateOpenGL(*this);
    // Notify renderer about altitude changes
    evAltitudeChanged = gwb.getNotifications().subscribe<NodeNote>([this](const NodeNote& note) {
        if(note.type == NodeNote::Altitude)
        {
            maxNodeAltitude_ = std::max(maxNodeAltitude_, gwb.getNode(note.pos).altitude);
            tr.AltitudeChanged(note.pos, *this);
        }
    });
    // And visibility changes
    evVisibilityChanged = gwb.getNotifications().subscribe<PlayerNodeNote>([this](const PlayerNodeNote& note) {
        if(note.type == PlayerNodeNote::Visibility)
            VisibilityChanged(note.pt, note.player);
    });
}

SoundManager& GameWorldViewer::getSoundMgr()
{
    return gwb.getSoundMgr();
}

const GamePlayer& GameWorldViewer::getPlayer() const
{
    return getWorld().getPlayer(playerId_);
}

unsigned GameWorldViewer::getNumPlayers() const
{
    return getWorld().getNumPlayers();
}

unsigned GameWorldViewer::getNumSoldiersForAttack(const MapPoint pt) const
{
    const auto* attacked_building = getWorld().getSpecObj<nobBaseMilitary>(pt);
    // Can we actually attack this bld?
    if(!attacked_building || !attacked_building->isAttackable(playerId_))
        return 0;

    // Militärgebäude in der Nähe finden
    unsigned total_count = 0;

    sortedMilitaryBlds buildings = getWorld().LookForMilitaryBuildings(pt, 3);
    for(auto& building : buildings)
    {
        // Muss ein Gebäude von uns sein und darf nur ein "normales Militärgebäude" sein (kein HQ etc.)
        if(building->getPlayer() == playerId_ && BuildingProperties::isMilitary(building->getBuildingType()))
            total_count += static_cast<nobMilitary*>(building)->getNumSoldiersForAttack(pt);
    }

    return total_count;
}

BuildingQuality GameWorldViewer::GetBQ(const MapPoint& pt) const
{
    return getWorld().AdjustBQ(pt, playerId_, visualNodes[pt].bq);
}

Visibility GameWorldViewer::getVisibility(const MapPoint pt, const bool checkAllVisible) const
{
    if(checkAllVisible && isAllVisible())
        return Visibility::Visible;
    return getWorld().CalcVisiblityWithAllies(pt, playerId_);
}

bool GameWorldViewer::isAllVisible() const
{
    const auto* const gi = getWorld().getGameInterface();
    if(gi && gi->GI_GetCheats().isAllVisible())
        return true;

    /// Replaymodus und FoW aus? Dann alles sichtbar
    if(GAMECLIENT.isReplayModeOn() && GAMECLIENT.isReplayFOWDisabled())
        return true;

    // Spieler schon tot? Dann auch alles sichtbar?
    if(getPlayer().IsDefeated())
        return true;
    return false;
}

bool GameWorldViewer::isOwner(const MapPoint& pt) const
{
    return getWorld().getNode(pt).owner == playerId_ + 1;
}

bool GameWorldViewer::isPlayerTerritory(const MapPoint& pt) const
{
    return getWorld().isPlayerTerritory(pt, playerId_ + 1);
}

const MapNode& GameWorldViewer::getNode(const MapPoint& pt) const
{
    return getWorld().getNode(pt);
}

MapPoint GameWorldViewer::getNeighbour(const MapPoint pt, const Direction dir) const
{
    return getWorld().getNeighbour(pt, dir);
}

void GameWorldViewer::RecalcAllColors()
{
    tr.UpdateAllColors(*this);
}

/// liefert sichtbare Straße, im FoW entsprechend die FoW-Straße
PointRoad GameWorldViewer::getVisibleRoad(const MapPoint pt, RoadDir roadDir, const Visibility visibility) const
{
    if(visibility == Visibility::Visible)
        return getVisibleRoad(pt, roadDir);
    else if(visibility == Visibility::FogOfWar)
        return getYoungestFOWNode(pt).roads[roadDir];
    else
        return PointRoad::None;
}

PointRoad GameWorldViewer::getVisibleRoad(const MapPoint pt, RoadDir roadDir) const
{
    const PointRoad visualResult = visualNodes[pt].roads[roadDir];
    if(visualResult != PointRoad::None)
        return visualResult;
    else
        return getWorld().getRoad(pt, roadDir);
}

PointRoad GameWorldViewer::getVisiblePointRoad(MapPoint pt, Direction dir) const
{
    const RoadDir rDir = getWorld().toRoadDir(pt, dir);
    return getVisibleRoad(pt, rDir);
}

void GameWorldViewer::setVisiblePointRoad(MapPoint pt, Direction dir, PointRoad type)
{
    const RoadDir rDir = getWorld().toRoadDir(pt, dir);
    visualNodes[pt].roads[rDir] = type;
}

bool GameWorldViewer::isOnRoad(const MapPoint& pt) const
{
    // This must be fast for BQ calculation so don't use GetVisiblePointRoad
    for(const auto roadDir : helpers::EnumRange<RoadDir>{})
        if(getVisibleRoad(pt, roadDir) != PointRoad::None)
            return true;
    for(const auto roadDir : helpers::EnumRange<RoadDir>{})
    {
        const Direction oppositeDir = getOppositeDir(roadDir);
        if(getVisibleRoad(getNeighbour(pt, oppositeDir), roadDir) != PointRoad::None)
            return true;
    }
    return false;
}

/// Return a ship at this position owned by the given player. Prefers ships that need instructions.
const noShip* GameWorldViewer::getShip(const MapPoint pt) const
{
    const noShip* resultShip = nullptr;

    // Check if we want this ship and set resultShip. Return true if we found a ship which is waiting for instructions
    auto checkShip = [pt, &resultShip, playerId = playerId_](const noShip& ship) {
        if(ship.getPlayerId() == playerId && (ship.getPos() == pt || ship.getDestinationForCurrentMove() == pt))
        {
            resultShip = &ship;
            if(ship.isWaitingForExpeditionInstructions())
                return true;
        }
        return false;
    };
    const auto& world = getWorld();
    auto checkPointForShips = [&world, checkShip](const MapPoint curPt, auto /*radius*/) {
        for(const auto& figure : world.getFigures(curPt))
        {
            if(figure.getGOT() == GO_Type::Ship && checkShip(static_cast<const noShip&>(figure)))
                return true;
        }
        return false;
    };
    world.CheckPointsInRadius(pt, 1u, checkPointForShips, true);

    return resultShip;
}

/// Gibt die verfügbar Anzahl der Angreifer für einen Seeangriff zurück
unsigned GameWorldViewer::getNumSoldiersForSeaAttack(const MapPoint pt) const
{
    return unsigned(getWorld().getSoldiersForSeaAttack(playerId_, pt).size());
}

void GameWorldViewer::ChangePlayer(unsigned player, bool updateVisualData /* = true*/)
{
    if(player == playerId_)
        return;
    playerId_ = player;
    if(updateVisualData)
    {
        RecalcAllColors();
        InitVisualData();
    }
}

helpers::EnumArray<MapPoint, Direction> GameWorldViewer::getNeighbours(const MapPoint pt) const
{
    return getWorld().getNeighbours(pt);
}

void GameWorldViewer::VisibilityChanged(const MapPoint& pt, unsigned player)
{
    // If visibility changed for us, or our team mate if shared view is on -> Update renderer
    if(player == playerId_ || (getWorld().getGGS().teamView && getWorld().getPlayer(playerId_).isAlly(player)))
        tr.VisibilityChanged(pt, *this);
}

void GameWorldViewer::RoadConstructionEnded(const RoadNote& note)
{
    if(note.player != playerId_
       || (note.type != RoadNote::Constructed && note.type != RoadNote::ConstructionFailed)) //-V560
        return;
    // Road construction command ended -> Remove visual overlay
    RemoveVisualRoad(note.pos, note.route);
}

void GameWorldViewer::RecalcBQ(const MapPoint& pt)
{
    BQCalculator calcBQ(getWorld());
    visualNodes[pt].bq = calcBQ(pt, [this](const MapPoint& pos) { return isOnRoad(pos); });
}

void GameWorldViewer::RecalcBQForRoad(const MapPoint& pt)
{
    RecalcBQ(pt);

    for(const Direction dir : {Direction::East, Direction::SouthEast, Direction::SouthWest})
        RecalcBQ(getNeighbour(pt, dir));
}

void GameWorldViewer::RemoveVisualRoad(const MapPoint& start, const std::vector<Direction>& route)
{
    MapPoint curPt = start;
    for(auto i : route)
    {
        setVisiblePointRoad(curPt, i, PointRoad::None);
        RecalcBQForRoad(curPt);
        curPt = getWorld().getNeighbour(curPt, i);
    }
    RecalcBQForRoad(curPt);
}

bool GameWorldViewer::isRoadAvailable(bool isWaterRoad, const MapPoint& pt) const
{
    return !isOnRoad(pt) && getWorld().isRoadAvailable(isWaterRoad, pt);
}

/// Get the "youngest" FOWObject of all players who share the view with the local player
const FOWObject* GameWorldViewer::getYoungestFOWObject(const MapPoint pos) const
{
    return getYoungestFOWNode(pos).object.get();
}

/// Gets the youngest fow node of all visible objects of all players who are connected
/// with the local player via team view
const FoWNode& GameWorldViewer::getYoungestFOWNode(const MapPoint pos) const
{
    const MapNode& node = getWorld().getNode(pos);
    const FoWNode* bestNode = &node.fow[playerId_];
    unsigned youngest_time = bestNode->last_update_time;

    // Shared team view enabled?
    if(getWorld().getGGS().teamView)
    {
        const GamePlayer& player = getWorld().getPlayer(playerId_);
        // Then check if team members have a better (="younger", see our economy) fow object
        for(unsigned i = 0; i < getWorld().getNumPlayers(); ++i)
        {
            if(!player.isAlly(i))
                continue;
            // Has the player FOW at this point at all?
            const FoWNode* curNode = &node.fow[i];
            if(curNode->visibility == Visibility::FogOfWar)
            {
                // Younger than the youngest or no object at all?
                if(curNode->last_update_time > youngest_time)
                {
                    // Then take it
                    youngest_time = curNode->last_update_time;
                    // And remember its owner
                    bestNode = curNode;
                }
            }
        }
    }

    return *bestNode;
}
