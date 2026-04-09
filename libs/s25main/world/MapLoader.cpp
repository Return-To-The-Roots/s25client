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
#include "buildings/nobHQ.h"
#include "factories/BuildingFactory.h"
#include "helpers/IdRange.h"
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
    GameDataLoader gdLoader(world_.GetDescriptionWriteable());
    if(!gdLoader.Load())
        return false;

    uint8_t gfxSet = map.getHeader().getGfxSet();
    DescIdx<LandscapeDesc> lt =
      world_.GetDescription().landscapes.find([gfxSet](const LandscapeDesc& l) { return l.s2Id == gfxSet; });
    world_.Init(MapExtent(map.getHeader().getWidth(), map.getHeader().getHeight()), lt); //-V807

    if(!InitNodes(map, exploration))
        return false;
    PlaceObjects(map);
    PlaceAnimals(map);
    if(!InitSeasAndHarbors(world_))
        return false;

    /// Schatten
    InitShadows(world_);

    // If we have explored FoW, create the FoW objects
    if(exploration == Exploration::FogOfWarExplored)
        SetMapExplored(world_);

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

    if(!Load(map, world_.GetGGS().exploration))
        return false;
    if(!PlaceHQs())
        return false;

    world_.CreateTradeGraphs();

    return true;
}

bool MapLoader::LoadLuaScript(Game& game, ILocalGameState& localgameState, const boost::filesystem::path& luaFilePath)
{
    if(!bfs::exists(luaFilePath))
        return false;
    auto lua = std::make_unique<LuaInterfaceGame>(game, localgameState);
    if(!lua->loadScript(luaFilePath) || !lua->CheckScriptVersion())
        return false;
    game.SetLua(std::move(lua));
    return true;
}

bool MapLoader::PlaceHQs(bool addStartWares)
{
    std::vector<MapPoint> hqPositions = hqPositions_;
    if(world_.GetGGS().randomStartPosition)
        RANDOM_SHUFFLE2(hqPositions, 0);
    return PlaceHQs(world_, hqPositions, addStartWares);
}

void MapLoader::InitShadows(World& world)
{
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
        world.RecalcShadow(pt);
}

void MapLoader::SetMapExplored(World& world)
{
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        // For every player
        for(unsigned i = 0; i < MAX_PLAYERS; ++i)
        {
            // If we have FoW here, save it
            if(world.GetNode(pt).fow[i].visibility == Visibility::FogOfWar)
                world.SaveFOWNode(pt, i, 0);
        }
    }
}

DescIdx<TerrainDesc> MapLoader::getTerrainFromS2(uint8_t s2Id) const
{
    return world_.GetDescription().terrain.find([s2Id, landscape = world_.GetLandscapeType()](const TerrainDesc& t) {
        return t.s2Id == s2Id && t.landscape == landscape;
    });
}

bool MapLoader::InitNodes(const libsiedler2::ArchivItem_Map& map, Exploration exploration)
{
    using libsiedler2::MapLayer;
    // Init node data (everything except the objects, figures and BQ)
    RTTR_FOREACH_PT(MapPoint, world_.GetSize())
    {
        MapNode& node = world_.GetNodeInt(pt);

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

void MapLoader::PlaceObjects(const libsiedler2::ArchivItem_Map& map)
{
    hqPositions_.clear();

    RTTR_FOREACH_PT(MapPoint, world_.GetSize())
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
                        hqPositions_.push_back(MapPoint::Invalid());
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

        world_.GetNodeInt(pt).obj = obj;
    }
}

void MapLoader::PlaceAnimals(const libsiedler2::ArchivItem_Map& map)
{
    // Tiere auslesen
    RTTR_FOREACH_PT(MapPoint, world_.GetSize())
    {
        Species species;
        using libsiedler2::MapLayer;
        switch(map.getMapDataAt(MapLayer::Animals, pt.x, pt.y))
        {
            // TODO: Which id is the polar bear?
            case 1:
                species = (RANDOM.Rand(RANDOM_CONTEXT2(0), 2) == 0) ? Species::RabbitWhite : Species::RabbitGrey;
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

        world_.AddFigure(pt, std::make_unique<noAnimal>(species, pt)).StartLiving();
    }
}

bool MapLoader::PlaceHQs(GameWorldBase& world, const std::vector<MapPoint>& hqPositions, const bool addStartWares)
{
    for(unsigned i = 0; i < world.GetNumPlayers(); ++i)
    {
        // Skip unused slots
        if(!world.GetPlayer(i).isUsed())
            continue;

        // Does the HQ have a position?
        if(i >= hqPositions.size() || !hqPositions[i].isValid())
        {
            LOG.write(_("Player %u does not have a valid start position!\n")) % i;
            if(world.HasLua()) // HQ can be placed in the script, so don't signal error
                continue;
            else
                return false;
        }

        auto* hq = checkedCast<nobHQ*>(BuildingFactory::CreateBuilding(world, BuildingType::Headquarters,
                                                                       hqPositions[i], i, world.GetPlayer(i).nation));
        if(addStartWares)
            hq->addStartWares();
    }
    return true;
}

bool MapLoader::InitSeasAndHarbors(World& world, const std::vector<MapPoint>& additionalHarbors)
{
    for(MapPoint pt : additionalHarbors)
        world.harborData.push_back(HarborPos(pt));
    // Clear current harbors and seas
    RTTR_FOREACH_PT(MapPoint, world.GetSize()) //-V807
    {
        MapNode& node = world.GetNodeInt(pt);
        node.seaId.reset();
        node.harborId.reset();
    }

    /// Determine all seas
    world.seas.clear();
    SeaId curSeaId(1);
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        // Point is not yet assigned a sea but should be
        if(!world.GetNode(pt).seaId && world.IsSeaPoint(pt))
        {
            const auto seaSize = MeasureSea(world, pt, curSeaId);
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
                seaId = world.GetSeaFromCoastalPoint(world.GetNeighbour(it->pos, dir));
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
            world.GetNodeInt(it->pos).harborId = curHarborId;
            curHarborId = curHarborId.next();
            ++it;
        }
    }

    // Calculate the neighbors and distances
    CalcHarborPosNeighbors(world);

    // Validate
    for(const auto startHbId : helpers::idRange<HarborId>(world.harborData.size()))
    {
        const HarborPos& startHbPos = world.harborData[startHbId];
        for(const std::vector<HarborPos::Neighbor>& neighbors : startHbPos.neighbors)
        {
            for(const HarborPos::Neighbor& neighbor : neighbors)
            {
                if(world.CalcHarborDistance(neighbor.id, startHbId) != neighbor.distance)
                {
                    LOG.write("Bug: Harbor distance mismatch for harbors %1%->%2%: %3% != %4%\n") % startHbId
                      % neighbor.id % world.CalcHarborDistance(neighbor.id, startHbId) % neighbor.distance;
                    return false;
                }
            }
        }
    }
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
void MapLoader::CalcHarborPosNeighbors(World& world)
{
    for(HarborPos& harbor : world.harborData)
    {
        for(const auto dir : helpers::EnumRange<ShipDirection>{})
            harbor.neighbors[dir].clear();
    }
    PathConditionShip shipPathChecker(world);

    // pre-calculate sea-points, as IsSeaPoint is rather expensive
    std::vector<int8_t> ptIsSeaPt(world.nodes.size()); //-V656

    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        if(shipPathChecker.IsNodeOk(pt))
            ptIsSeaPt[world.GetIdx(pt)] = -1;
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
                SeaId seaId = world.GetSeaId(otherHbId, dir);
                // No sea? -> Next
                if(!seaId)
                    continue;
                const MapPoint coastPt = world.GetNeighbour(world.GetHarborPoint(otherHbId), dir);
                // This should not be marked for visit
                unsigned idx = world.GetIdx(coastPt);
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
            SeaId seaId = world.GetSeaFromCoastalPoint(ownCoastPt);
            auto const coastToHbs = coastToHarborPerSea[seaId].equal_range(world.GetIdx(ownCoastPt));
            for(auto it = coastToHbs.first; it != coastToHbs.second; ++it)
            {
                ShipDirection shipDir = world.GetShipDir(ownCoastPt, ownCoastPt);
                world.harborData[startHbId].neighbors[shipDir].push_back(HarborPos::Neighbor(it->second, 0));
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
                MapPoint curPt = world.GetNeighbour(curNode.pos, dir);
                unsigned idx = world.GetIdx(curPt);

                const int8_t ptValue = ptToVisitOrHb[idx];
                // Already visited
                if(ptValue == 0)
                    continue;
                // Not reachable
                if(!shipPathChecker.IsEdgeOk(curNode.pos, dir))
                    continue;

                if(ptValue > 0) // found harbor(s)
                {
                    ShipDirection shipDir = world.GetShipDir(world.harborData[startHbId].pos, curPt);
                    SeaId seaId = world.GetSeaFromCoastalPoint(curPt);
                    auto const coastToHbs = coastToHarborPerSea[seaId].equal_range(idx);
                    for(auto it = coastToHbs.first; it != coastToHbs.second; ++it)
                    {
                        const HarborId otherHbId = it->second;
                        if(hbFound[otherHbId])
                            continue;

                        hbFound[otherHbId] = true;
                        world.harborData[startHbId].neighbors[shipDir].push_back(
                          HarborPos::Neighbor(otherHbId, curNode.distance + 1));

                        // Make this the only coastal point of this harbor for this sea
                        HarborPos& otherHb = world.harborData[otherHbId];
                        RTTR_Assert(seaId);
                        for(const auto hbDir : helpers::EnumRange<Direction>{})
                        {
                            if(otherHb.seaIds[hbDir] == seaId && world.GetNeighbour(otherHb.pos, hbDir) != curPt)
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
unsigned MapLoader::MeasureSea(World& world, const MapPoint start, SeaId seaId)
{
    // Breitensuche von diesem Punkt aus durchf�hren
    std::vector<bool> visited(world.GetWidth() * world.GetHeight(), false);
    std::queue<MapPoint> todo;

    todo.push(start);
    visited[world.GetIdx(start)] = true;

    // Count of nodes (including start node)
    unsigned count = 0;

    while(!todo.empty())
    {
        MapPoint p = todo.front();
        todo.pop();

        RTTR_Assert(visited[world.GetIdx(p)]);
        world.GetNodeInt(p).seaId = seaId;

        for(const MapPoint neighbourPt : world.GetNeighbours(p))
        {
            if(visited[world.GetIdx(neighbourPt)])
                continue;
            visited[world.GetIdx(neighbourPt)] = true;

            // Ist das dort auch ein Meerespunkt?
            if(world.IsSeaPoint(neighbourPt))
                todo.push(neighbourPt);
        }

        ++count;
    }

    return count;
}
