// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU pragma: keep
#include "world/MapLoader.h"
#include "world/World.h"
#include "ogl/glArchivItem_Map.h"
#include "Random.h"
#include "nodeObjs/noEnvObject.h"
#include "nodeObjs/noStaticObject.h"
#include "nodeObjs/noGranite.h"
#include "nodeObjs/noTree.h"
#include "nodeObjs/noAnimal.h"
#include "buildings/nobHQ.h"
#include "gameTypes/ShipDirection.h"
#include "gameData/TerrainData.h"
#include "gameData/MaxPlayers.h"
#include "libsiedler2/src/ArchivItem_Map_Header.h"
#include "Log.h"
#include <queue>
#include <algorithm>

class noBase;
class nobBaseWarehouse;

MapLoader::MapLoader(World& world, const std::vector<Nation>& playerNations): world(world), playerNations(playerNations)
{}

bool MapLoader::Load(const glArchivItem_Map& map, bool randomStartPos, Exploration exploration)
{
    world.Init(map.getHeader().getWidth(), map.getHeader().getHeight(), LandscapeType(map.getHeader().getGfxSet())); //-V807

    InitNodes(map, exploration);
    PlaceObjects(map);
    PlaceAnimals(map);
    InitSeasAndHarbors(world);

    /// Schatten
    InitShadows(world);

    // If we have explored FoW, create the FoW objects
    if(exploration == EXP_FOGOFWARE_EXPLORED)
        SetMapExplored(world, playerNations.size());

    if(!PlaceHQs(world, hqPositions, playerNations, randomStartPos))
        return false;

    return true;
}

void MapLoader::InitShadows(World& world)
{
    RTTR_FOREACH_PT(MapPoint, world.GetWidth(), world.GetHeight())
        world.RecalcShadow(pt);
}

void MapLoader::SetMapExplored(World& world, unsigned numPlayers)
{
    RTTR_FOREACH_PT(MapPoint, world.GetWidth(), world.GetHeight())
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
    RTTR_FOREACH_PT(MapPoint, world.GetWidth(), world.GetHeight())
    {
        MapNode& node = world.GetNodeInt(pt);

        std::fill(node.roads.begin(), node.roads.end(), 0);
        node.altitude = map.GetMapDataAt(MAP_ALTITUDE, pt.x, pt.y);
        unsigned char t1 = map.GetMapDataAt(MAP_TERRAIN1, pt.x, pt.y), t2 = map.GetMapDataAt(MAP_TERRAIN2, pt.x, pt.y);

        // Hafenplatz?
        if(TerrainData::IsHarborSpot(t1))
            world.harbor_pos.push_back(pt);
            
        // Will be set later
        node.harborId = 0;

        node.t1 = TerrainData::MapIdx2Terrain(t1);
        node.t2 = TerrainData::MapIdx2Terrain(t2);

        unsigned char resource = map.GetMapDataAt(MAP_RESOURCES, pt.x, pt.y);
        // Wasser?
        if(resource == 0x20 || resource == 0x21)
        {
            // TODO: Berge hatten komische Wasserbeeinflussung
            // ggf 0-4 Wasser setzen
            if((node.t1 == TT_DESERT || node.t2 == TT_DESERT) || TerrainData::IsWater(node.t1) || TerrainData::IsWater(node.t2))
                resource = 0; // Kein Wasser, in der W�ste, da isses trocken!
            else if((node.t1 == TT_STEPPE || node.t2 == TT_STEPPE))
                resource = 0x23; // 2 Wasser
            else if((node.t1 == TT_SAVANNAH || node.t2 == TT_SAVANNAH))
                resource = 0x25; // 4 Wasser
            else
                resource = 0x27; // 7 Wasser
        } else if(resource > 0x80 && resource < 0x90) // fish
            resource = 0x84; // Use 4 fish
        node.resources = resource;

        node.reserved = false;
        node.owner = 0;
        std::fill(node.boundary_stones.begin(), node.boundary_stones.end(), 0);
        node.bq = BQ_NOTHING;
        node.seaId = 0;

        Visibility fowVisibility;
        switch(exploration)
        {
        case EXP_DISABLED:
            fowVisibility = VIS_VISIBLE;
            break;
        case EXP_CLASSIC:
        case EXP_FOGOFWAR:
            fowVisibility = VIS_INVISIBLE;
            break;
        case EXP_FOGOFWARE_EXPLORED:
            fowVisibility = VIS_FOW;
            break;
        default:
            throw std::invalid_argument("Visibility for FoW");
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
    hqPositions.clear();

    RTTR_FOREACH_PT(MapPoint, world.GetWidth(), world.GetHeight())
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
                while(hqPositions.size() <= lc)
                    hqPositions.push_back(MapPoint::Invalid());
                hqPositions[lc] = pt;
            }
        } break;

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
                LOG.write("Unbekannter Baum1-4 auf x=%d, y=%d: (0x%0X)\n", pt.x, pt.y, lc);
        } break;

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
                LOG.write("Unbekannter Baum5-8 auf x=%d, y=%d: (0x%0X)\n", pt.x, pt.y, lc);
        } break;

        // Baum 9
        case 0xC6:
        {
            if(lc >= 0x30 && lc <= 0x3D)
                obj = new noTree(pt, 8, 3);
            else
                LOG.write("Unbekannter Baum9 auf x=%d, y=%d: (0x%0X)\n", pt.x, pt.y, lc);
        } break;

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
            else if(lc >= 0x0C && lc <= 0x0F)
                obj = new noEnvObject(pt, 500 + lc);
            // Objekte aus der map.lst
            else if(lc >= 0x10 && lc <= 0x14)
                obj = new noEnvObject(pt, 542 + lc - 0x10);
            // exists in mis0bobs-mis5bobs -> take stranded ship
            else if(lc == 0x15)
                obj = new noStaticObject(pt, 0, 0, 1);
            // gate
            else if(lc == 0x16)
                obj = new noStaticObject(pt, 560, 0xFFFF, 2);
            // open gate
            else if(lc == 0x17)
                obj = new noStaticObject(pt, 561, 0xFFFF, 2);
            // Stalagmiten (mis1bobs)
            else if(lc >= 0x18 && lc <= 0x1E)
                obj = new noStaticObject(pt, (lc - 0x18) * 2, 1);
            // toter Baum (mis1bobs)
            else if(lc >= 0x1F && lc <= 0x20)
                obj = new noStaticObject(pt, 20 + (lc - 0x1F) * 2, 1);
            // Gerippe (mis1bobs)
            else if(lc == 0x21)
                obj = new noEnvObject(pt, 30, 1);
            // Objekte aus der map.lst
            else if(lc >= 0x22 && lc <= 0x27)
                obj = new noEnvObject(pt, 550 + lc - 0x22);
            // Objekte aus der map.lst
            else if(lc >= 0x28 && lc <= 0x2B)
                obj = new noEnvObject(pt, 556 + lc - 0x28);
            // tent and ruin of guardhouse
            else if(lc >= 0x2C && lc <= 0x2D)
                obj = new noStaticObject(pt, (lc - 0x2C) * 2, 2);
            // tower ruin
            else if(lc == 0x2E)
                obj = new noStaticObject(pt, (lc - 0x2C) * 2, 2, 1);
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
            // The next 2 are non standard and only for access in RTTR (replace in original though
            // whale skeleton (head left)
            else if(lc == 0x34)
                obj = new noStaticObject(pt, 2, 5);
            // Cave
            else if(lc == 0x35)
                obj = new noStaticObject(pt, 4, 5);
            else
                LOG.write("Unbekanntes Naturzeug auf x=%d, y=%d: (0x%0X)\n", pt.x, pt.y, lc);

        } break;

        // Granit Typ 1
        case 0xCC:
        {
            if(lc >= 0x01 && lc <= 0x06)
                obj = new noGranite(GT_1, lc - 1);
            else
                LOG.write("Unbekannter Granit1 auf x=%d, y=%d: (0x%0X)\n", pt.x, pt.y, lc);
        } break;

        // Granit Typ 2
        case 0xCD:
        {
            if(lc >= 0x01 && lc <= 0x06)
                obj = new noGranite(GT_2, lc - 1);
            else
                LOG.write("Unbekannter Granit2 auf x=%d, y=%d: (0x%0X)\n", pt.x, pt.y, lc);
        } break;

        // Nichts
        case 0:
            break;

        default:
#ifndef NDEBUG
            unsigned char unknownObj = map.GetMapDataAt(MAP_TYPE, pt.x, pt.y);
            LOG.write("Unbekanntes Objekt (0x%0X) auf x=%d, y=%d: (0x%0X)\n", unknownObj, pt.x, pt.y, lc);
#endif // !NDEBUG
            break;
        }

        world.GetNodeInt(pt).obj = obj;
    }
}

void MapLoader::PlaceAnimals(const glArchivItem_Map& map)
{
    // Tiere auslesen
    MapPoint pt;
    RTTR_FOREACH_PT(MapPoint, world.GetWidth(), world.GetHeight())
    {
        Species species;
        switch(map.GetMapDataAt(MAP_ANIMALS, pt.x, pt.y))
        {
            // TODO: Welche ID ist Polarb�r?
        case 1: species = Species(SPEC_RABBITWHITE + RANDOM.Rand(__FILE__, __LINE__, 0, 2)); break; // zuf�llige Hasenart nehmen
        case 2: species = SPEC_FOX; break;
        case 3: species = SPEC_STAG; break;
        case 4: species = SPEC_DEER; break;
        case 5: species = SPEC_DUCK; break;
        case 6: species = SPEC_SHEEP; break;
        case 0: species = SPEC_NOTHING; break;
        default:
#ifndef NDEBUG
            unsigned char unknownAnimal = map.GetMapDataAt(MAP_ANIMALS, pt.x, pt.y);
            LOG.write("Unknown animal species at x=%d, y=%d: (0x%0X)\n", pt.x, pt.y, unknownAnimal, unknownAnimal);
#endif // !NDEBUG
            species = SPEC_NOTHING;
            break;
        }

        if(species != SPEC_NOTHING)
        {
            noAnimal* animal = new noAnimal(species, pt);
            world.AddFigure(animal, pt);
            // Loslaufen
            animal->StartLiving();
        }
    }
}

// random function using RANDOM.Rand(...) for std::random_shuffle
struct RandomFunctor
{
    ptrdiff_t operator()(ptrdiff_t max) const
    {
        RTTR_Assert(max < std::numeric_limits<int>::max());
        return RANDOM.Rand(__FILE__, __LINE__, 0, static_cast<int>(max));
    }
};

bool MapLoader::PlaceHQs(World& world, std::vector<MapPoint> hqPositions, const std::vector<Nation>& playerNations, bool randomStartPos)
{
    //random locations? -> randomize them :)
    if(randomStartPos)
    {
        RandomFunctor random;
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
            LOG.write(_("Player %u does not have a valid start position!"), i);
            return false;
        }
        nobHQ* hq = new nobHQ(hqPositions[i], i, playerNations[i]);
        world.SetNO(hqPositions[i], hq);
    }
    return true;
}

void MapLoader::InitSeasAndHarbors(World& world, const std::vector<MapPoint>& additionalHarbors)
{
    world.harbor_pos.insert(world.harbor_pos.end(), additionalHarbors.begin(), additionalHarbors.end());

    /// Weltmeere vermessen
    RTTR_FOREACH_PT(MapPoint, world.GetWidth(), world.GetHeight())
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
        bool foundCoast = false;
        for(unsigned z = 0; z < 6; ++z)
        {
            const unsigned short seaId = world.GetSeaFromCoastalPoint(world.GetNeighbour(it->pos, z));
            it->cps[z].seaId = seaId;
            if(seaId)
                foundCoast = true;
        }
        if(!foundCoast)
        {
            LOG.write("Map Bug: Found harbor without coast at %u:%u. Removing!\n", it->pos.x, it->pos.y);
            it = world.harbor_pos.erase(it);
        } else
        {
            world.GetNodeInt(it->pos).harborId = curHarborId++;
            ++it;
        }
    }

    // Nachbarn der einzelnen Hafenpl�tze ermitteln
    CalcHarborPosNeighbors(world);
}


// class for finding harbor neighbors
class CalcHarborPosNeighborsNode
{
public:
    CalcHarborPosNeighborsNode() {} //-V730
    CalcHarborPosNeighborsNode(const MapPoint pt, unsigned distance): pos(pt), distance(distance) {}

    MapPoint pos;
    unsigned distance;
};

/// Calculate the distance from each harbor to the others
void MapLoader::CalcHarborPosNeighbors(World& world)
{
    // FIFO queue used for a BFS
    std::queue<CalcHarborPosNeighborsNode> todo_list;

    // pre-calculate sea-points, as IsSeaPoint is rather expensive
    std::vector<unsigned int> flags_init(world.nodes.size()); //-V656

    RTTR_FOREACH_PT(MapPoint, world.GetWidth(), world.GetHeight())
        flags_init[world.GetIdx(pt)] = world.IsSeaPoint(pt) ? 1 : 0;

    for(size_t startHbId = 1; startHbId < world.harbor_pos.size(); ++startHbId)
    {
        RTTR_Assert(todo_list.empty());

        // Copy sea points to working flags. Possible values are
        // 0 - visited or no sea point
        // 1 - sea point, not already visited
        // n - harbor_pos[n - 1]
        std::vector<unsigned int> flags(flags_init);

        // add another entry, so that we can use the value from 'flags' directly.
        std::vector<bool> found(world.harbor_pos.size() + 1, false);

        // mark points around harbors
        for(size_t otherHbId = 1; otherHbId < world.harbor_pos.size(); ++otherHbId)
        {
            /* Mark sea points belonging to harbor_pos[nr]:

            As sea points are only those fully surrounded by sea, we have to go two
            steps away from a harbor point to find them
            */
            for(size_t d = 0; d < 12; ++d)
            {
                MapPoint pa = world.GetNeighbour2(world.harbor_pos[otherHbId].pos, d);

                if(flags[world.GetIdx(pa)] == 1)
                {
                    if(otherHbId == startHbId)
                    {
                        // This is our start harbor. Add the sea points around it to our todo list.
                        todo_list.push(CalcHarborPosNeighborsNode(pa, 0));
                        flags[world.GetIdx(pa)] = 0; // Mark them as visited (flags = 0) to avoid finding a way to our start harbor.
                    } else
                    {
                        flags[world.GetIdx(pa)] = otherHbId + 1;
                    }
                }
            }
        }

        while(!todo_list.empty()) // as long as there are sea points on our todo list...
        {
            CalcHarborPosNeighborsNode curNode = todo_list.front();
            todo_list.pop();

            for(size_t d = 0; d < 6; ++d)
            {
                MapPoint curPt = world.GetNeighbour(curNode.pos, d);
                size_t idx = world.GetIdx(curPt);

                if((flags[idx] > 1) && !found[flags[idx]]) // found harbor we haven't already found
                {
                    ShipDirection shipDir = world.GetShipDir(world.harbor_pos[startHbId].pos, curPt);
                    world.harbor_pos[startHbId].neighbors[shipDir.toUInt()].push_back(HarborPos::Neighbor(flags[idx] - 1, curNode.distance + 1));

                    todo_list.push(CalcHarborPosNeighborsNode(curPt, curNode.distance + 1));

                    found[flags[idx]] = true;

                    flags[idx] = 0; // mark as visited, so we do not go here again
                } else if(flags[idx])    // this detects any sea point plus harbors we already visited
                {
                    todo_list.push(CalcHarborPosNeighborsNode(curPt, curNode.distance + 1));

                    flags[idx] = 0; // mark as visited, so we do not go here again
                }
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
    std::queue< MapPoint > todo;

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
            MapPoint neighbourPt = world.GetNeighbour(p, i);
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
