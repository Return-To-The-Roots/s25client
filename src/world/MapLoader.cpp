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

#include "rttrDefines.h" // IWYU pragma: keep
#include "world/MapLoader.h"
#include "PointOutput.h"
#include "buildings/nobHQ.h"
#include "factories/BuildingFactory.h"
#include "ogl/glArchivItem_Map.h"
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
#include "gameData/TerrainData.h"
#include "libsiedler2/ArchivItem_Map_Header.h"
#include "libutil/Log.h"
#include <boost/foreach.hpp>
#include <boost/typeof/typeof.hpp>
#include <algorithm>
#include <map>
#include <queue>

class noBase;
class nobBaseWarehouse;

MapLoader::MapLoader(World& world, const std::vector<Nation>& playerNations) : world_(world), playerNations_(playerNations) {}

bool MapLoader::Load(const glArchivItem_Map& map, Exploration exploration)
{
    world_.Init(MapExtent(map.getHeader().getWidth(), map.getHeader().getHeight()), LandscapeType(map.getHeader().getGfxSet())); //-V807

    InitNodes(map, exploration);
    PlaceObjects(map);
    PlaceAnimals(map);
    if(!InitSeasAndHarbors(world_))
        return false;

    /// Schatten
    InitShadows(world_);

    // If we have explored FoW, create the FoW objects
    if(exploration == EXP_FOGOFWARE_EXPLORED)
        SetMapExplored(world_, playerNations_.size());

    return true;
}

bool MapLoader::PlaceHQs(GameWorldBase& world, bool randomStartPos)
{
    return PlaceHQs(world, hqPositions_, playerNations_, randomStartPos);
}

void MapLoader::InitShadows(World& world)
{
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
        world.RecalcShadow(pt);
}

void MapLoader::SetMapExplored(World& world, unsigned numPlayers)
{
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        // For every player
        for(unsigned i = 0; i < numPlayers; ++i)
        {
            // If we have FoW here, save it
            if(world.GetNode(pt).fow[i].visibility == VIS_FOW)
                world.SaveFOWNode(pt, i, 0);
        }
    }
}

void MapLoader::InitNodes(const glArchivItem_Map& map, Exploration exploration)
{
    // Init node data (everything except the objects and figures)
    RTTR_FOREACH_PT(MapPoint, world_.GetSize())
    {
        MapNode& node = world_.GetNodeInt(pt);

        std::fill(node.roads.begin(), node.roads.end(), 0);
        node.altitude = map.GetMapDataAt(MAP_ALTITUDE, pt.x, pt.y);
        unsigned char t1 = map.GetMapDataAt(MAP_TERRAIN1, pt.x, pt.y), t2 = map.GetMapDataAt(MAP_TERRAIN2, pt.x, pt.y);

        // Hafenplatz?
        if(TerrainData::IsHarborSpot(t1))
            world_.harbor_pos.push_back(pt);

        // Will be set later
        node.harborId = 0;

        node.t1 = TerrainData::MapIdx2Terrain(t1);
        node.t2 = TerrainData::MapIdx2Terrain(t2);

        unsigned char mapResource = map.GetMapDataAt(MAP_RESOURCES, pt.x, pt.y);
        Resource resource;
        // Wasser?
        if(mapResource == 0x20 || mapResource == 0x21)
            resource = Resource(Resource::Water, 7);
        else if(mapResource > 0x40 && mapResource < 0x48)
            resource = Resource(Resource::Coal, mapResource - 0x40);
        else if(mapResource > 0x48 && mapResource < 0x50)
            resource = Resource(Resource::Iron, mapResource - 0x48);
        else if(mapResource > 0x50 && mapResource < 0x58)
            resource = Resource(Resource::Gold, mapResource - 0x50);
        else if(mapResource > 0x58 && mapResource < 0x60)
            resource = Resource(Resource::Granite, mapResource - 0x58);
        else if(mapResource > 0x80 && mapResource < 0x90) // fish
            resource = Resource(Resource::Fish, 4);       // Use 4 fish
        node.resources = resource;

        node.reserved = false;
        node.owner = 0;
        std::fill(node.boundary_stones.begin(), node.boundary_stones.end(), 0);
        node.bq = BQ_NOTHING;
        node.seaId = 0;

        Visibility fowVisibility;
        switch(exploration)
        {
            case EXP_DISABLED: fowVisibility = VIS_VISIBLE; break;
            case EXP_CLASSIC:
            case EXP_FOGOFWAR: fowVisibility = VIS_INVISIBLE; break;
            case EXP_FOGOFWARE_EXPLORED: fowVisibility = VIS_FOW; break;
            default: throw std::invalid_argument("Visibility for FoW");
        }

        // FOW-Zeug initialisieren
        for(unsigned i = 0; i < node.fow.size(); ++i)
        {
            FoWNode& fow = node.fow[i];
            fow.last_update_time = 0;
            fow.visibility = fowVisibility;
            fow.object = NULL;
            std::fill(fow.roads.begin(), fow.roads.end(), 0);
            fow.owner = 0;
            std::fill(fow.boundary_stones.begin(), fow.boundary_stones.end(), 0);
        }

        node.obj = NULL; // Will be overwritten later...
        RTTR_Assert(node.figures.empty());
    }
}

void MapLoader::PlaceObjects(const glArchivItem_Map& map)
{
    hqPositions_.clear();

    RTTR_FOREACH_PT(MapPoint, world_.GetSize())
    {
        unsigned char lc = map.GetMapDataAt(MAP_LANDSCAPE, pt.x, pt.y);
        noBase* obj = NULL;

        switch(map.GetMapDataAt(MAP_TYPE, pt.x, pt.y))
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
                // Objekte aus der map_?_z.lst
                if(lc <= 0x0A)
                    obj = new noEnvObject(pt, 500 + lc);
                // "wasserstein" aus der map_?_z.lst
                else if(lc == 0x0B)
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
                // tent and ruin of guardhouse
                else if(lc <= 0x2D)
                    obj = new noStaticObject(pt, (lc - 0x2C) * 2, 2);
                // tower ruin
                else if(lc == 0x2E)
                    obj = new noStaticObject(pt, (lc - 0x2C) * 2, 2);
                // castle ruin
                else if(lc == 0x2F)
                    obj = new noStaticObject(pt, (lc - 0x2C) * 2, 2, 2);
                // cross
                else if(lc == 0x30)
                    obj = new noEnvObject(pt, (lc - 0x2C) * 2, 2);
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
                    obj = new noGranite(GT_1, lc - 1);
                else
                    LOG.write(_("Unknown granite type2 at %1%: (0x%2$x)\n")) % pt % unsigned(lc);
            }
            break;

            // Granit Typ 2
            case 0xCD:
            {
                if(lc >= 0x01 && lc <= 0x06)
                    obj = new noGranite(GT_2, lc - 1);
                else
                    LOG.write(_("Unknown granite type2 at %1%: (0x%2$x)\n")) % pt % unsigned(lc);
            }
            break;

            // Nichts
            case 0: break;

            default:
#ifndef NDEBUG
                unsigned unknownObj = map.GetMapDataAt(MAP_TYPE, pt.x, pt.y);
                LOG.write(_("Unknown object at %1%: (0x%2$x: 0x%3$x)\n")) % pt % unknownObj % unsigned(lc);
#endif // !NDEBUG
                break;
        }

        world_.GetNodeInt(pt).obj = obj;
    }
}

void MapLoader::PlaceAnimals(const glArchivItem_Map& map)
{
    // Tiere auslesen
    MapPoint pt;
    RTTR_FOREACH_PT(MapPoint, world_.GetSize())
    {
        Species species;
        switch(map.GetMapDataAt(MAP_ANIMALS, pt.x, pt.y))
        {
            // TODO: Welche ID ist Polarb�r?
            case 1:
                species = Species(SPEC_RABBITWHITE + RANDOM.Rand(__FILE__, __LINE__, 0, 2));
                break; // zuf�llige Hasenart nehmen
            case 2: species = SPEC_FOX; break;
            case 3: species = SPEC_STAG; break;
            case 4: species = SPEC_DEER; break;
            case 5: species = SPEC_DUCK; break;
            case 6: species = SPEC_SHEEP; break;
            case 0:
            case 0xFF: // 0xFF is for (really) old S2 maps
                species = SPEC_NOTHING;
                break;
            default:
#ifndef NDEBUG
                unsigned unknownAnimal = map.GetMapDataAt(MAP_ANIMALS, pt.x, pt.y);
                LOG.write(_("Unknown animal species at %1%: (0x%2$x)\n")) % pt % unknownAnimal;
#endif // !NDEBUG
                species = SPEC_NOTHING;
                break;
        }

        if(species != SPEC_NOTHING)
        {
            noAnimal* animal = new noAnimal(species, pt);
            world_.AddFigure(pt, animal);
            // Loslaufen
            animal->StartLiving();
        }
    }
}

bool MapLoader::PlaceHQs(GameWorldBase& world, std::vector<MapPoint> hqPositions, const std::vector<Nation>& playerNations,
                         bool randomStartPos)
{
    // random locations? -> randomize them :)
    if(randomStartPos)
    {
        RANDOM_FUNCTOR(random);
        std::random_shuffle(hqPositions.begin(), hqPositions.end(), random);
    }

    for(unsigned i = 0; i < playerNations.size(); ++i)
    {
        // Skip unused slots
        if(playerNations[i] == NAT_INVALID)
            continue;

        // Does the HQ have a position?
        if(i >= hqPositions.size() || !hqPositions[i].isValid())
        {
            LOG.write(_("Player %u does not have a valid start position!")) % i;
            return false;
        }

        BuildingFactory::CreateBuilding(world, BLD_HEADQUARTERS, hqPositions[i], i, playerNations[i]);
    }
    return true;
}

bool MapLoader::InitSeasAndHarbors(World& world, const std::vector<MapPoint>& additionalHarbors)
{
    world.harbor_pos.insert(world.harbor_pos.end(), additionalHarbors.begin(), additionalHarbors.end());
    // Clear current harbors and seas
    RTTR_FOREACH_PT(MapPoint, world.GetSize()) //-V807
    {
        MapNode& node = world.GetNodeInt(pt);
        node.seaId = 0u;
        node.harborId = 0;
    }

    /// Weltmeere vermessen
    world.seas.clear();
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        // Noch kein Meer an diesem Punkt  Aber trotzdem Teil eines noch nicht vermessenen Meeres?
        if(!world.GetNode(pt).seaId && world.IsSeaPoint(pt))
        {
            unsigned sea_size = MeasureSea(world, pt, world.seas.size() + 1);
            world.seas.push_back(World::Sea(sea_size));
        }
    }

    /// Die Meere herausfinden, an die die Hafenpunkte grenzen
    unsigned curHarborId = 1;
    for(std::vector<HarborPos>::iterator it = world.harbor_pos.begin() + 1; it != world.harbor_pos.end();)
    {
        std::vector<bool> hasCoastAtSea(world.seas.size() + 1, false);
        bool foundCoast = false;
        for(unsigned z = 0; z < 6; ++z)
        {
            // Skip point at NW as often there is no path from it if the harbor is north of an island
            unsigned short seaId =
              (z == Direction::NORTHWEST) ? 0 : world.GetSeaFromCoastalPoint(world.GetNeighbour(it->pos, Direction::fromInt(z)));
            // Only 1 coastal point per sea
            if(hasCoastAtSea[seaId])
                seaId = 0;
            else
                hasCoastAtSea[seaId] = true;

            it->cps[z].seaId = seaId;
            if(seaId)
                foundCoast = true;
        }
        if(!foundCoast)
        {
            LOG.write("Map Bug: Found harbor without coast at %1%. Removing!\n") % it->pos;
            it = world.harbor_pos.erase(it);
        } else
        {
            world.GetNodeInt(it->pos).harborId = curHarborId++;
            ++it;
        }
    }

    // Calculate the neighbors and distances
    CalcHarborPosNeighbors(world);

    // Validate
    for(unsigned startHbId = 1; startHbId < world.harbor_pos.size(); ++startHbId)
    {
        const HarborPos& startHbPos = world.harbor_pos[startHbId];
        BOOST_FOREACH(const std::vector<HarborPos::Neighbor>& neighbors, startHbPos.neighbors)
        {
            BOOST_FOREACH(const HarborPos::Neighbor& neighbor, neighbors)
            {
                if(world.CalcHarborDistance(neighbor.id, startHbId) != neighbor.distance)
                {
                    LOG.write("Bug: Harbor distance mismatch for harbors %1%->%2%: %3% != %4%\n") % startHbId % neighbor.id
                      % world.CalcHarborDistance(neighbor.id, startHbId) % neighbor.distance;
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
    CalcHarborPosNeighborsNode() {} //-V730
    CalcHarborPosNeighborsNode(const MapPoint pt, unsigned distance) : pos(pt), distance(distance) {}

    MapPoint pos;
    unsigned distance;
};

/// Calculate the distance from each harbor to the others
void MapLoader::CalcHarborPosNeighbors(World& world)
{
    BOOST_FOREACH(HarborPos& harbor, world.harbor_pos)
    {
        for(unsigned z = 0; z < 6; ++z)
            harbor.neighbors[z].clear();
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

    for(unsigned startHbId = 1; startHbId < world.harbor_pos.size(); ++startHbId)
    {
        RTTR_Assert(todo_list.empty());

        // Copy sea points to working flags. Possible values are
        // -1 - sea point, not already visited
        // 0 - visited or no sea point
        // 1 - Coast to a harbor
        std::vector<int8_t> ptToVisitOrHb(ptIsSeaPt);

        std::vector<bool> hbFound(world.harbor_pos.size(), false);
        // For each sea, store the coastal point indices and their harbor
        std::vector<std::multimap<unsigned, unsigned> > coastToHarborPerSea(world.seas.size() + 1);
        std::vector<MapPoint> ownCoastalPoints;

        // mark coastal points around harbors
        for(unsigned otherHbId = 1; otherHbId < world.harbor_pos.size(); ++otherHbId)
        {
            for(unsigned d = 0; d < Direction::COUNT; d++)
            {
                unsigned seaId = world.GetSeaId(otherHbId, Direction::fromInt(d));
                // No sea? -> Next
                if(!seaId)
                    continue;
                const MapPoint coastPt = world.GetNeighbour(world.GetHarborPoint(otherHbId), Direction::fromInt(d));
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

        BOOST_FOREACH(const MapPoint& ownCoastPt, ownCoastalPoints)
        {
            // Special case: Get all harbors that share the coast point with us
            unsigned short seaId = world.GetSeaFromCoastalPoint(ownCoastPt);
            BOOST_AUTO(coastToHbs, coastToHarborPerSea[seaId].equal_range(world.GetIdx(ownCoastPt)));
            for(BOOST_AUTO(it, coastToHbs.first); it != coastToHbs.second; ++it)
            {
                ShipDirection shipDir = world.GetShipDir(ownCoastPt, ownCoastPt);
                world.harbor_pos[startHbId].neighbors[shipDir.toUInt()].push_back(HarborPos::Neighbor(it->second, 0));
                hbFound[it->second] = true;
            }
            todo_list.push(CalcHarborPosNeighborsNode(ownCoastPt, 0));
        }

        while(!todo_list.empty()) // as long as there are sea points on our todo list...
        {
            CalcHarborPosNeighborsNode curNode = todo_list.front();
            todo_list.pop();

            for(unsigned dir = 0; dir < Direction::COUNT; ++dir)
            {
                MapPoint curPt = world.GetNeighbour(curNode.pos, Direction::fromInt(dir));
                unsigned idx = world.GetIdx(curPt);

                int ptValue = ptToVisitOrHb[idx];
                // Already visited
                if(ptValue == 0)
                    continue;
                // Not reachable
                if(!shipPathChecker.IsEdgeOk(curNode.pos, Direction::fromInt(dir)))
                    continue;

                if(ptValue > 0) // found harbor(s)
                {
                    ShipDirection shipDir = world.GetShipDir(world.harbor_pos[startHbId].pos, curPt);
                    unsigned seaId = world.GetSeaFromCoastalPoint(curPt);
                    BOOST_AUTO(coastToHbs, coastToHarborPerSea[seaId].equal_range(idx));
                    for(BOOST_AUTO(it, coastToHbs.first); it != coastToHbs.second; ++it)
                    {
                        unsigned otherHbId = it->second;
                        if(hbFound[otherHbId])
                            continue;

                        hbFound[otherHbId] = true;
                        world.harbor_pos[startHbId].neighbors[shipDir.toUInt()].push_back(
                          HarborPos::Neighbor(otherHbId, curNode.distance + 1));

                        // Make this the only coastal point of this harbor for this sea
                        HarborPos& otherHb = world.harbor_pos[otherHbId];
                        RTTR_Assert(seaId);
                        for(unsigned hbDir = 0; hbDir < Direction::COUNT; ++hbDir)
                        {
                            if(otherHb.cps[hbDir].seaId == seaId && world.GetNeighbour(otherHb.pos, Direction::fromInt(hbDir)) != curPt)
                                otherHb.cps[hbDir].seaId = 0;
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
unsigned MapLoader::MeasureSea(World& world, const MapPoint start, unsigned short seaId)
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

        for(unsigned i = 0; i < 6; ++i)
        {
            MapPoint neighbourPt = world.GetNeighbour(p, Direction::fromInt(i));
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
