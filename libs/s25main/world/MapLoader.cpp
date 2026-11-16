// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "world/MapLoader.h"
#include "Game.h"
#include "GamePlayer.h"
#include "GameWorldBase.h"
#include "GlobalGameSettings.h"
#include "PointOutput.h"
#include "RttrForeachPt.h"
#include "addons/const_addons.h"
#include "buildings/nobHQ.h"
#include "factories/BuildingFactory.h"
#include "helpers/IdRange.h"
#include "helpers/Range.h"
#include "helpers/containerUtils.h"
#include "helpers/mathFuncs.h"
#include "lua/GameDataLoader.h"
#include "pathfinding/PathConditionShip.h"
#include "random/Random.h"
#include "world/World.h"
#include "nodeObjs/noAnimal.h"
#include "nodeObjs/noEnvObject.h"
#include "nodeObjs/noGranite.h"
#include "nodeObjs/noStaticObject.h"
#include "nodeObjs/noTree.h"
#include "gameTypes/ShipDirection.h"
#include "gameData/MaxPlayers.h"
#include "gameData/TerrainDesc.h"
#include "libsiedler2/Archiv.h"
#include "libsiedler2/ArchivItem_Map.h"
#include "libsiedler2/ArchivItem_Map_Header.h"
#include "libsiedler2/prototypen.h"
#include "s25util/Log.h"
#include <boost/filesystem/operations.hpp>
#include <algorithm>
#include <map>
#include <queue>

class noBase;

MapLoader::MapLoader(GameWorldBase& world) : world_(world) {}

bool MapLoader::Load(const libsiedler2::ArchivItem_Map& map, Exploration exploration)
{
    GameDataLoader gdLoader(world_.getDescriptionWriteable());
    if(!gdLoader.Load())
        return false;

    uint8_t gfxSet = map.getHeader().getGfxSet();
    DescIdx<LandscapeDesc> lt =
      world_.getDescription().landscapes.find([gfxSet](const LandscapeDesc& l) { return l.s2Id == gfxSet; });
    world_.Init(MapExtent(map.getHeader().getWidth(), map.getHeader().getHeight()), lt); //-V807

    if(!initNodes(map, exploration))
        return false;
    placeObjects(map);
    placeAnimals(map);
    if(!initSeasAndHarbors(world_))
        return false;

    /// Schatten
    initShadows(world_);

    // If we have explored FoW, create the FoW objects
    if(exploration == Exploration::FogOfWarExplored)
        setMapExplored(world_);

    return true;
}

bool MapLoader::Load(const boost::filesystem::path& mapFilePath)
{
    // Map laden
    libsiedler2::Archiv mapArchiv;

    // Karteninformationen laden
    if(libsiedler2::loader::LoadMAP(mapFilePath, mapArchiv) != 0)
        return false;

    const libsiedler2::ArchivItem_Map& map = *static_cast<libsiedler2::ArchivItem_Map*>(mapArchiv[0]);

    if(!Load(map, world_.getGGS().exploration))
        return false;
    if(!placeHQs())
        return false;

    return true;
}

bool MapLoader::loadLuaScript(Game& game, ILocalGameState& localgameState, const boost::filesystem::path& luaFilePath)
{
    if(!bfs::exists(luaFilePath))
        return false;
    auto lua = std::make_unique<LuaInterfaceGame>(game, localgameState);
    if(!lua->loadScript(luaFilePath) || !lua->checkScriptVersion())
        return false;
    game.setLua(std::move(lua));
    return true;
}

bool MapLoader::placeHQs(bool addStartWares)
{
    std::vector<MapPoint> hqPositions = hqPositions_;
    if(world_.getGGS().randomStartPosition)
        RANDOM_SHUFFLE2(hqPositions, 0);
    return placeHQs(world_, hqPositions, addStartWares);
}

void MapLoader::setupResources(GameWorldBase& world, const bool fixFish)
{
    ResourceType target;
    switch(world.getGGS().getSelection(AddonId::CHANGE_GOLD_DEPOSITS))
    {
        case 0:
        default: target = ResourceType::Gold; break;
        case 1: target = ResourceType::Nothing; break;
        case 2: target = ResourceType::Iron; break;
        case 3: target = ResourceType::Coal; break;
        case 4: target = ResourceType::Granite; break;
    }
    convertMineResourceTypes(world, ResourceType::Gold, target);
    placeAndFixWater(world);
    if(fixFish)
        removeUnusableFishResources(world);
}

void MapLoader::convertMineResourceTypes(GameWorldBase& world, ResourceType from, ResourceType to)
{
    // LOG.write(("Convert map resources from %i to %i\n", from, to);
    if(from == to)
        return;

    RTTR_FOREACH_PT(MapPoint, world.getSize())
    {
        Resource resources = world.getNode(pt).resources;
        // Gibt es Ressourcen dieses Typs?
        // Wenn ja, dann umwandeln bzw löschen
        if(resources.getType() == from)
        {
            resources.setType(to);
            world.setResource(pt, resources);
        }
    }
}

void MapLoader::placeAndFixWater(GameWorldBase& world)
{
    const bool waterEverywhere = world.getGGS().getSelection(AddonId::EXHAUSTIBLE_WATER) == 1;

    RTTR_FOREACH_PT(MapPoint, world.getSize())
    {
        Resource curNodeResource = world.getNode(pt).resources;

        if(curNodeResource.getType() == ResourceType::Nothing)
        {
            if(!waterEverywhere)
                continue;
        } else if(curNodeResource.getType() != ResourceType::Water)
            continue; // do not override maps resource.

        uint8_t minHumidity = 100;
        for(const DescIdx<TerrainDesc> tIdx : world.getTerrainsAround(pt))
        {
            const uint8_t curHumidity = world.getDescription().get(tIdx).humidity;
            if(curHumidity < minHumidity)
            {
                minHumidity = curHumidity;
                if(minHumidity == 0)
                    break;
            }
        }
        if(minHumidity)
        {
            curNodeResource =
              Resource(ResourceType::Water, waterEverywhere ? 7 : helpers::iround<uint8_t>(minHumidity * 7. / 100.));
        } else
            curNodeResource = Resource(ResourceType::Nothing, 0);

        world.setResource(pt, curNodeResource);
    }
}

void MapLoader::removeUnusableFishResources(GameWorldBase& world)
{
    const auto isWaterPoint = [&world](const MapPoint nb) { return world.isWaterPoint(nb); };
    for(const MapCoord y : helpers::range(world.getHeight()))
    {
        // Optimization: When there was fish on the previous node (in the same row)
        // we do not need to check for isolated water points, as there is at least that water point
        bool previousHasFish = false;
        for(const MapCoord x : helpers::range(world.getWidth()))
        {
            const MapPoint pt(x, y);
            bool hasFish = false;

            if(world.getNode(pt).resources.has(ResourceType::Fish))
            {
                if(isWaterPoint(pt) && (previousHasFish || helpers::contains_if(world.getNeighbours(pt), isWaterPoint)))
                    hasFish = true;
                else
                    world.setResource(pt, Resource(ResourceType::Nothing, 0));
            }
            previousHasFish = hasFish;
        }
    }
}

void MapLoader::initShadows(World& world)
{
    RTTR_FOREACH_PT(MapPoint, world.getSize())
        world.recalcShadow(pt);
}

void MapLoader::setMapExplored(World& world)
{
    RTTR_FOREACH_PT(MapPoint, world.getSize())
    {
        // For every player
        for(const auto i : helpers::range(MAX_PLAYERS))
        {
            // If we have FoW here, save it
            if(world.getNode(pt).fow[i].visibility == Visibility::FogOfWar)
                world.saveFOWNode(pt, i, 0);
        }
    }
}

DescIdx<TerrainDesc> MapLoader::getTerrainFromS2(uint8_t s2Id) const
{
    return world_.getDescription().terrain.find([s2Id, landscape = world_.GetLandscapeType()](const TerrainDesc& t) {
        return t.s2Id == s2Id && t.landscape == landscape;
    });
}

bool MapLoader::initNodes(const libsiedler2::ArchivItem_Map& map, Exploration exploration)
{
    using libsiedler2::MapLayer;
    // Init node data (everything except the objects, figures and BQ)
    RTTR_FOREACH_PT(MapPoint, world_.getSize())
    {
        MapNode& node = world_.getNodeInt(pt);

        std::fill(node.roads.begin(), node.roads.end(), PointRoad::None);
        node.altitude = map.getMapDataAt(MapLayer::Altitude, pt.x, pt.y);
        unsigned char t1 = map.getMapDataAt(MapLayer::Terrain1, pt.x, pt.y),
                      t2 = map.getMapDataAt(MapLayer::Terrain2, pt.x, pt.y);

        // Hafenplatz?
        if((t1 & libsiedler2::HARBOR_MASK) != 0)
            world_.harborData.push_back(HarborPos(pt));

        // Will be set later
        node.harborId.reset();

        node.t1 = getTerrainFromS2(t1 & 0x3F); // Only lower 6 bits
        node.t2 = getTerrainFromS2(t2 & 0x3F); // Only lower 6 bits
        if(!node.t1 || !node.t2)
            return false;

        unsigned char mapResource = map.getMapDataAt(MapLayer::Resources, pt.x, pt.y);
        Resource resource;
        // Wasser?
        if(mapResource == 0x20 || mapResource == 0x21)
            resource = Resource(ResourceType::Water, 7);
        else if(mapResource > 0x40 && mapResource < 0x48)
            resource = Resource(ResourceType::Coal, mapResource - 0x40);
        else if(mapResource > 0x48 && mapResource < 0x50)
            resource = Resource(ResourceType::Iron, mapResource - 0x48);
        else if(mapResource > 0x50 && mapResource < 0x58)
            resource = Resource(ResourceType::Gold, mapResource - 0x50);
        else if(mapResource > 0x58 && mapResource < 0x60)
            resource = Resource(ResourceType::Granite, mapResource - 0x58);
        else if(mapResource > 0x80 && mapResource < 0x90) // fish
            resource = Resource(ResourceType::Fish, 4);   // Use 4 fish
        node.resources = resource;

        node.reserved = false;
        node.owner = 0;
        std::fill(node.boundary_stones.begin(), node.boundary_stones.end(), 0);
        node.seaId.reset();

        Visibility fowVisibility;
        switch(exploration)
        {
            case Exploration::Disabled: fowVisibility = Visibility::Visible; break;
            case Exploration::Classic:
            case Exploration::FogOfWar: fowVisibility = Visibility::Invisible; break;
            case Exploration::FogOfWarExplored: fowVisibility = Visibility::FogOfWar; break;
            default: throw std::invalid_argument("Visibility for FoW");
        }

        // FOW-Zeug initialisieren
        for(auto& fow : node.fow)
        {
            fow = FoWNode();
            fow.visibility = fowVisibility;
        }

        RTTR_Assert(node.figures.empty());
    }
    return true;
}

void MapLoader::placeObjects(const libsiedler2::ArchivItem_Map& map)
{
    hqPositions_.clear();

    RTTR_FOREACH_PT(MapPoint, world_.getSize())
    {
        using libsiedler2::MapLayer;
        unsigned char lc = map.getMapDataAt(MapLayer::ObjectIndex, pt.x, pt.y);
        noBase* obj = nullptr;

        switch(map.getMapDataAt(MapLayer::ObjectType, pt.x, pt.y))
        {
            // Player Startpos (provisorisch)
            case 0x80:
            {
                if(lc < MAX_PLAYERS)
                {
                    while(hqPositions_.size() <= lc)
                        hqPositions_.push_back(MapPoint::invalid());
                    hqPositions_[lc] = pt;
                }
            }
            break;

            // Baum 1-4
            case 0xC4:
            {
                if(lc >= 0x30 && lc <= 0x3D)
                    obj = new noTree(pt, 0, 3);
                else if(lc >= 0x70 && lc <= 0x7D)
                    obj = new noTree(pt, 1, 3);
                else if(lc >= 0xB0 && lc <= 0xBD)
                    obj = new noTree(pt, 2, 3);
                else if(lc >= 0xF0 && lc <= 0xFD)
                    obj = new noTree(pt, 3, 3);
                else
                    LOG.write(_("Unknown tree1-4 at %1%: (0x%2$x)\n")) % pt % unsigned(lc);
            }
            break;

            // Baum 5-8
            case 0xC5:
            {
                if(lc >= 0x30 && lc <= 0x3D)
                    obj = new noTree(pt, 4, 3);
                else if(lc >= 0x70 && lc <= 0x7D)
                    obj = new noTree(pt, 5, 3);
                else if(lc >= 0xB0 && lc <= 0xBD)
                    obj = new noTree(pt, 6, 3);
                else if(lc >= 0xF0 && lc <= 0xFD)
                    obj = new noTree(pt, 7, 3);
                else
                    LOG.write(_("Unknown tree5-8 at %1%: (0x%2$x)\n")) % pt % unsigned(lc);
            }
            break;

            // Baum 9
            case 0xC6:
            {
                if(lc >= 0x30 && lc <= 0x3D)
                    obj = new noTree(pt, 8, 3);
                else
                    LOG.write(_("Unknown tree9 at %1%: (0x%2$x)\n")) % pt % unsigned(lc);
            }
            break;

            // Sonstiges Naturzeug ohne Funktion, nur zur Dekoration
            case 0xC8:
            case 0xC9: // Note: 0xC9 is actually a bug and should be 0xC8. But the random map generator produced that...
            {
                // "wasserstein" aus der map_?_z.lst
                if(lc == 0x0B)
                    obj = new noStaticObject(pt, 500 + lc);
                // Objekte aus der map_?_z.lst
                else if(lc <= 0x0F)
                    obj = new noEnvObject(pt, 500 + lc);
                // Objekte aus der map.lst
                else if(lc <= 0x14)
                    obj = new noEnvObject(pt, 542 + lc - 0x10);
                // exists in mis0bobs-mis5bobs -> take stranded ship
                else if(lc == 0x15)
                    obj = new noStaticObject(pt, 0, 0);
                // gate
                else if(lc == 0x16)
                    obj = new noStaticObject(pt, 560);
                // open gate
                else if(lc == 0x17)
                    obj = new noStaticObject(pt, 561);
                // Stalagmiten (mis1bobs)
                else if(lc <= 0x1E)
                    obj = new noStaticObject(pt, (lc - 0x18) * 2, 1);
                // toter Baum (mis1bobs)
                else if(lc <= 0x20)
                    obj = new noStaticObject(pt, 20 + (lc - 0x1F) * 2, 1);
                // Gerippe (mis1bobs)
                else if(lc == 0x21)
                    obj = new noStaticObject(pt, 30, 1);
                // Objekte aus der map.lst
                else if(lc <= 0x2B)
                    obj = new noEnvObject(pt, 550 + lc - 0x22);
                // tent, ruin of guardhouse, tower ruin, cross
                else if(lc <= 0x2E || lc == 0x30)
                    obj = new noStaticObject(pt, (lc - 0x2C) * 2, 2);
                // castle ruin
                else if(lc == 0x2F)
                    obj = new noStaticObject(pt, (lc - 0x2C) * 2, 2, 2);
                // small wiking with boat
                else if(lc == 0x31)
                    obj = new noStaticObject(pt, 0, 3);
                // Pile of wood
                else if(lc == 0x32)
                    obj = new noStaticObject(pt, 0, 4);
                // whale skeleton (head right)
                else if(lc == 0x33)
                    obj = new noStaticObject(pt, 0, 5);
                // The next 2 are non standard and only for access in RTTR (replaced in original by
                // whale skeleton (head left)
                else if(lc == 0x34)
                    obj = new noStaticObject(pt, 2, 5);
                // Cave
                else if(lc == 0x35)
                    obj = new noStaticObject(pt, 4, 5);
                else
                    LOG.write(_("Unknown nature object at %1%: (0x%2$x)\n")) % pt % unsigned(lc);
            }
            break;

            // Granit Typ 1
            case 0xCC:
            {
                if(lc >= 0x01 && lc <= 0x06)
                    obj = new noGranite(GraniteType::One, lc - 1);
                else
                    LOG.write(_("Unknown granite type2 at %1%: (0x%2$x)\n")) % pt % unsigned(lc);
            }
            break;

            // Granit Typ 2
            case 0xCD:
            {
                if(lc >= 0x01 && lc <= 0x06)
                    obj = new noGranite(GraniteType::Two, lc - 1);
                else
                    LOG.write(_("Unknown granite type2 at %1%: (0x%2$x)\n")) % pt % unsigned(lc);
            }
            break;

            // Nichts
            case 0: break;

            default:
#ifndef NDEBUG
                unsigned unknownObj = map.getMapDataAt(MapLayer::ObjectType, pt.x, pt.y);
                LOG.write(_("Unknown object at %1%: (0x%2$x: 0x%3$x)\n")) % pt % unknownObj % unsigned(lc);
#endif // !NDEBUG
                break;
        }

        world_.getNodeInt(pt).obj = obj;
    }
}

void MapLoader::placeAnimals(const libsiedler2::ArchivItem_Map& map)
{
    // Tiere auslesen
    RTTR_FOREACH_PT(MapPoint, world_.getSize())
    {
        Species species;
        using libsiedler2::MapLayer;
        switch(map.getMapDataAt(MapLayer::Animals, pt.x, pt.y))
        {
            // TODO: Which id is the polar bear?
            case 1:
                species = (RANDOM.rand(RANDOM_CONTEXT2(0), 2) == 0) ? Species::RabbitWhite : Species::RabbitGrey;
                break; // Random rabbit
            case 2: species = Species::Fox; break;
            case 3: species = Species::Stag; break;
            case 4: species = Species::Deer; break;
            case 5: species = Species::Duck; break;
            case 6: species = Species::Sheep; break;
            case 0:
            case 0xFF: // 0xFF is for (really) old S2 maps
                continue;
            default:
#ifndef NDEBUG
                unsigned unknownAnimal = map.getMapDataAt(MapLayer::Animals, pt.x, pt.y);
                LOG.write(_("Unknown animal species at %1%: (0x%2$x)\n")) % pt % unknownAnimal;
#endif // !NDEBUG
                continue;
        }

        world_.addFigure(pt, std::make_unique<noAnimal>(species, pt)).startLiving();
    }
}

bool MapLoader::placeHQs(GameWorldBase& world, const std::vector<MapPoint>& hqPositions, const bool addStartWares)
{
    for(unsigned i = 0; i < world.getNumPlayers(); ++i)
    {
        // Skip unused slots
        if(!world.getPlayer(i).isUsed())
            continue;

        // Does the HQ have a position?
        if(i >= hqPositions.size() || !hqPositions[i].isValid())
        {
            LOG.write(_("Player %u does not have a valid start position!")) % i;
            return false;
        }

        auto* hq = checkedCast<nobHQ*>(BuildingFactory::createBuilding(world, BuildingType::Headquarters,
                                                                       hqPositions[i], i, world.getPlayer(i).nation));
        if(addStartWares)
            hq->addStartWares();
    }
    return true;
}

bool MapLoader::initSeasAndHarbors(World& world, const std::vector<MapPoint>& additionalHarbors)
{
    for(MapPoint pt : additionalHarbors)
        world.harborData.push_back(HarborPos(pt));
    // Clear current harbors and seas
    RTTR_FOREACH_PT(MapPoint, world.getSize()) //-V807
    {
        MapNode& node = world.getNodeInt(pt);
        node.seaId.reset();
        node.harborId.reset();
    }

    /// Determine all seas
    world.seas.clear();
    SeaId curSeaId(1);
    RTTR_FOREACH_PT(MapPoint, world.getSize())
    {
        // Point is not yet assigned a sea but should be
        if(!world.getNode(pt).seaId && world.isSeaPoint(pt))
        {
            const auto seaSize = measureSea(world, pt, curSeaId);
            world.seas.push_back(World::Sea(seaSize));
            curSeaId = curSeaId.next();
        }
    }

    /// Determine seas adjacent to the harbor places
    HarborId curHarborId(1);
    for(auto it = world.harborData.begin(); it != world.harborData.end();)
    {
        helpers::StrongIdVector<bool, SeaId> hasCoastAtSea(world.seas.size(), false);
        bool foundCoast = false;
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            SeaId seaId;
            // Skip point at NW as often there is no path from it if the harbor is north of an island
            if(dir != Direction::NorthWest)
            {
                seaId = world.getSeaFromCoastalPoint(world.getNeighbour(it->pos, dir));
                if(seaId)
                {
                    foundCoast = true;
                    // Only 1 coastal point per sea
                    if(hasCoastAtSea[seaId])
                        seaId.reset();
                    else
                        hasCoastAtSea[seaId] = true;
                }
            }
            it->seaIds[dir] = seaId;
        }
        if(!foundCoast)
        {
            LOG.write("Map Bug: Found harbor without coast at %1%. Removing!\n") % it->pos;
            it = world.harborData.erase(it);
        } else
        {
            world.getNodeInt(it->pos).harborId = curHarborId;
            curHarborId = curHarborId.next();
            ++it;
        }
    }

    // Calculate the neighbors and distances
    calcHarborPosNeighbors(world);
    return true;
}

// class for finding harbor neighbors
struct CalcHarborPosNeighborsNode
{
    CalcHarborPosNeighborsNode() = default; //-V730
    CalcHarborPosNeighborsNode(const MapPoint pt, unsigned distance) : pos(pt), distance(distance) {}

    MapPoint pos;
    unsigned distance;
};

/// Calculate the distance from each harbor to the others
void MapLoader::calcHarborPosNeighbors(World& world)
{
    for(HarborPos& harbor : world.harborData)
    {
        for(const auto dir : helpers::EnumRange<ShipDirection>{})
            harbor.neighbors[dir].clear();
    }
    PathConditionShip shipPathChecker(world);

    // pre-calculate sea-points, as IsSeaPoint is rather expensive
    std::vector<int8_t> ptIsSeaPt(world.nodes.size()); //-V656

    RTTR_FOREACH_PT(MapPoint, world.getSize())
    {
        if(shipPathChecker.isNodeOk(pt))
            ptIsSeaPt[world.getIdx(pt)] = -1;
    }

    // FIFO queue used for a BFS
    std::queue<CalcHarborPosNeighborsNode> todo_list;

    for(const auto startHbId : helpers::idRange<HarborId>(world.harborData.size()))
    {
        RTTR_Assert(todo_list.empty());

        // Copy sea points to working flags. Possible values are
        // -1 - sea point, not already visited
        // 0 - visited or no sea point
        // 1 - Coast to a harbor
        std::vector<int8_t> ptToVisitOrHb(ptIsSeaPt);

        helpers::StrongIdVector<bool, HarborId> hbFound(world.harborData.size(), false);
        // For each sea, store the coastal point indices and their harbor
        helpers::StrongIdVector<std::multimap<unsigned, HarborId>, SeaId> coastToHarborPerSea(world.seas.size());
        std::vector<MapPoint> ownCoastalPoints;

        // mark coastal points around harbors
        for(const auto otherHbId : helpers::idRange<HarborId>(world.harborData.size()))
        {
            for(const auto dir : helpers::EnumRange<Direction>{})
            {
                SeaId seaId = world.getSeaId(otherHbId, dir);
                // No sea? -> Next
                if(!seaId)
                    continue;
                const MapPoint coastPt = world.getNeighbour(world.getHarborPoint(otherHbId), dir);
                // This should not be marked for visit
                unsigned idx = world.getIdx(coastPt);
                RTTR_Assert(ptToVisitOrHb[idx] != -1);
                if(otherHbId == startHbId)
                {
                    // This is our start harbor. Add the coast points around it to our todo list.
                    ownCoastalPoints.push_back(coastPt);
                } else
                {
                    ptToVisitOrHb[idx] = 1;
                    coastToHarborPerSea[seaId].insert(std::make_pair(idx, otherHbId));
                }
            }
        }

        for(const MapPoint& ownCoastPt : ownCoastalPoints)
        {
            // Special case: Get all harbors that share the coast point with us
            const SeaId seaId = world.getSeaFromCoastalPoint(ownCoastPt);
            auto const coastToHbs = coastToHarborPerSea[seaId].equal_range(world.getIdx(ownCoastPt));
            for(auto it = coastToHbs.first; it != coastToHbs.second; ++it)
            {
                ShipDirection shipDir = world.getShipDir(ownCoastPt, ownCoastPt);
                world.harborData[startHbId].neighbors[shipDir].push_back(HarborPos::Neighbor(it->second, seaId, 0));
                hbFound[it->second] = true;
            }
            todo_list.push(CalcHarborPosNeighborsNode(ownCoastPt, 0));
        }

        while(!todo_list.empty()) // as long as there are sea points on our todo list...
        {
            CalcHarborPosNeighborsNode curNode = todo_list.front();
            todo_list.pop();

            for(const auto dir : helpers::EnumRange<Direction>{})
            {
                MapPoint curPt = world.getNeighbour(curNode.pos, dir);
                unsigned idx = world.getIdx(curPt);

                const int8_t ptValue = ptToVisitOrHb[idx];
                // Already visited
                if(ptValue == 0)
                    continue;
                // Not reachable
                if(!shipPathChecker.isEdgeOk(curNode.pos, dir))
                    continue;

                if(ptValue > 0) // found harbor(s)
                {
                    const ShipDirection shipDir = world.getShipDir(world.harborData[startHbId].pos, curPt);
                    const SeaId seaId = world.getSeaFromCoastalPoint(curPt);
                    RTTR_Assert(seaId);
                    auto const coastToHbs = coastToHarborPerSea[seaId].equal_range(idx);
                    for(auto it = coastToHbs.first; it != coastToHbs.second; ++it)
                    {
                        const HarborId otherHbId = it->second;
                        if(hbFound[otherHbId])
                            continue;

                        hbFound[otherHbId] = true;
                        world.harborData[startHbId].neighbors[shipDir].push_back(
                          HarborPos::Neighbor(otherHbId, seaId, curNode.distance + 1));

                        // Make this the only coastal point of this harbor for this sea
                        HarborPos& otherHb = world.harborData[otherHbId];
                        for(const auto hbDir : helpers::EnumRange<Direction>{})
                        {
                            if(otherHb.seaIds[hbDir] == seaId && world.getNeighbour(otherHb.pos, hbDir) != curPt)
                                otherHb.seaIds[hbDir].reset();
                        }
                    }
                }
                todo_list.push(CalcHarborPosNeighborsNode(curPt, curNode.distance + 1));
                ptToVisitOrHb[idx] = 0; // mark as visited, so we do not go here again
            }
        }
    }
}

/// Vermisst ein neues Weltmeer von einem Punkt aus, indem es alle mit diesem Punkt verbundenen
/// Wasserpunkte mit der gleichen ID belegt und die Anzahl zur�ckgibt
unsigned MapLoader::measureSea(World& world, const MapPoint start, SeaId seaId)
{
    // Breitensuche von diesem Punkt aus durchf�hren
    std::vector<bool> visited(world.getWidth() * world.getHeight(), false);
    std::queue<MapPoint> todo;

    todo.push(start);
    visited[world.getIdx(start)] = true;

    // Count of nodes (including start node)
    unsigned count = 0;

    while(!todo.empty())
    {
        MapPoint p = todo.front();
        todo.pop();

        RTTR_Assert(visited[world.getIdx(p)]);
        world.getNodeInt(p).seaId = seaId;

        for(const MapPoint neighbourPt : world.getNeighbours(p))
        {
            if(visited[world.getIdx(neighbourPt)])
                continue;
            visited[world.getIdx(neighbourPt)] = true;

            // Ist das dort auch ein Meerespunkt?
            if(world.isSeaPoint(neighbourPt))
                todo.push(neighbourPt);
        }

        ++count;
    }

    return count;
}
