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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h"
#include "Loader.h"
#include "GameClient.h"
#include "nodeObjs/noEnvObject.h"
#include "nodeObjs/noStaticObject.h"
#include "nodeObjs/noGranite.h"
#include "nodeObjs/noTree.h"
#include "nodeObjs/noFlag.h"
#include "buildings/nobHQ.h"
#include "nodeObjs/noFire.h"
#include "buildings/nobUsual.h"
#include "nodeObjs/noAnimal.h"
#include "CatapultStone.h"
#include "buildings/noBuildingSite.h"
#include "Random.h"
#include "TradeGraph.h"
#include "gameData/MapConsts.h"
#include "SerializedGameData.h"
#include "Log.h"
#include "ogl/glArchivItem_Map.h"

#include "WindowManager.h"
#include "SoundManager.h"
#include "gameData/TerrainData.h"

#include "../libsiedler2/src/prototypen.h"
#include "luaIncludes.h"
#include <boost/filesystem.hpp>
#include <queue>
#include <algorithm>

/// Lädt eine Karte
bool GameWorld::LoadMap(const std::string& filename)
{
    // Map laden
    libsiedler2::ArchivInfo mapArchiv;

    // Karteninformationen laden
    if(libsiedler2::loader::LoadMAP(filename, mapArchiv) != 0)
        return false;

    const glArchivItem_Map& map = *dynamic_cast<glArchivItem_Map*>(mapArchiv.get(0));

    bfs::path luaPath(filename);
    luaPath.replace_extension("lua");
    std::string luaFilePath = luaPath.string();

    if (bfs::exists(luaPath) && luaL_dofile(lua, luaFilePath.c_str()))
    {
        fprintf(stderr, "LUA ERROR: '%s'!\n", lua_tostring(lua, -1));
        lua_pop(lua, 1);
    }

    Scan(map);

    CreateTradeGraphs();

    tr.GenerateOpenGL(*this);

    if(GetPlayer(GAMECLIENT.GetPlayerID()).hqPos.isValid())
        this->MoveToMapObject(GetPlayer(GAMECLIENT.GetPlayerID()).hqPos);

    LUA_EventStart();

    return true;
}

void GameWorld::Scan(const glArchivItem_Map& map)
{
    width_ = map.getHeader().getWidth(); //-V807
    height_ = map.getHeader().getHeight();
    lt = LandscapeType(map.getHeader().getGfxSet());

    Init();

    // Dummy-Hafenpos für Index 0 einfügen
    // -> the dummy is so that the harbor "0" might be used for ships with no particular destination
    // poc: if you ever remove this dummy go to GameWorldBase::CalcDistanceToNearestHarbor and fix the loop to include the first harbor again (think Ive seen other instances of dummyadjusted loops as well...)
    harbor_pos.push_back(MapPoint(0, 0));
    
    InitNodes(map);
    std::vector<MapPoint> headquarter_positions = PlaceObjects(map);
    PlaceHQs(headquarter_positions);
    PlaceAnimals(map);
    InitSeasAndHarbors();

    /// Schatten und BQ berechnen
    MapPoint pt;
    for(pt.y = 0; pt.y < height_; ++pt.y)
    {
        for(pt.x = 0; pt.x < width_; ++pt.x)
        {
            RecalcShadow(pt);
            SetBQ(pt, GAMECLIENT.GetPlayerID());
        }
    }
	
    /// Bei FoW und aufgedeckt müssen auch die ersten FoW-Objekte erstellt werden
    if(GAMECLIENT.GetGGS().exploration == GlobalGameSettings::EXP_FOGOFWARE_EXPLORED)
    {
    for(pt.y = 0; pt.y < height_; ++pt.y)
    {
        for(pt.x = 0; pt.x < width_; ++pt.x)
        {
                // Alle Spieler durchgehen
                for(unsigned i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
                {
                    // An der Stelle FOW für diesen Spieler?
                    if(GetNode(pt).fow[i].visibility == VIS_FOW)
                        SaveFOWNode(pt, i);
                }
            }
        }
    }

}

void GameWorld::InitNodes(const glArchivItem_Map& map)
{
    // Init node data (everything except the objects and figures)
    MapPoint pt(0, 0);	
    for(pt.y = 0; pt.y < height_; ++pt.y)
    {
        for(pt.x = 0; pt.x < width_; ++pt.x)
        {
        	MapNode& node = GetNode(pt);

            std::fill(node.roads.begin(), node.roads.end(), 0);
            std::fill(node.roads_real.begin(), node.roads_real.end(), false);
            node.altitude = map.GetMapDataAt(MAP_ALTITUDE, pt.x, pt.y);
            unsigned char t1 = map.GetMapDataAt(MAP_TERRAIN1, pt.x, pt.y), t2 = map.GetMapDataAt(MAP_TERRAIN2, pt.x, pt.y);

            // Hafenplatz?
            if(TerrainData::IsHarborSpot(t1))
            {	
				HarborPos p(pt);
                node.harbor_id = harbor_pos.size();
                harbor_pos.push_back(p);
            }
            else
                node.harbor_id = 0;

            node.t1 = TerrainData::MapIdx2Terrain(t1);
            node.t2 = TerrainData::MapIdx2Terrain(t2);

            unsigned char resource = map.GetMapDataAt(MAP_RESOURCES, pt.x, pt.y);
            // Wasser?
            if(resource == 0x20 || resource == 0x21)
            {
                // TODO: Berge hatten komische Wasserbeeinflussung
                // ggf 0-4 Wasser setzen
                if( (node.t1 == TT_DESERT || node.t2 == TT_DESERT) || TerrainData::IsWater(node.t1) || TerrainData::IsWater(node.t2) )
                    resource = 0; // Kein Wasser, in der Wüste, da isses trocken!
                else if( (node.t1 == TT_STEPPE || node.t2 == TT_STEPPE) )
                    resource = 0x23; // 2 Wasser
                else if( (node.t1 == TT_SAVANNAH || node.t2 == TT_SAVANNAH) )
                    resource = 0x25; // 4 Wasser
                else
                    resource = 0x27; // 7 Wasser
            }else if(resource > 0x80 && resource < 0x90) // fish
                resource = 0x84; // Use 4 fish
            node.resources = resource;

            node.reserved = false;
            node.owner = 0;
            std::fill(node.boundary_stones.begin(), node.boundary_stones.end(), 0);
            node.bq = BQ_NOTHING;
            node.sea_id = 0;

            Visibility fowVisibility;
                switch(GAMECLIENT.GetGGS().exploration)
                {
                    case GlobalGameSettings::EXP_DISABLED:
                fowVisibility = VIS_VISIBLE;
                break;
                    case GlobalGameSettings::EXP_CLASSIC:
                    case GlobalGameSettings::EXP_FOGOFWAR:
                fowVisibility = VIS_INVISIBLE;
                break;
                    case GlobalGameSettings::EXP_FOGOFWARE_EXPLORED:
                fowVisibility = VIS_FOW;
                break;
            default:
                throw std::invalid_argument("Visibility for FoW");
                }

            // FOW-Zeug initialisieren
            for(unsigned i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
            {
                MapNode::FoWData& fow = node.fow[i];                
                fow.last_update_time = 0;
                fow.visibility = fowVisibility;
                fow.object = NULL;
                std::fill(fow.roads.begin(), fow.roads.end(), 0);
                fow.owner = 0;
                std::fill(fow.boundary_stones.begin(), fow.boundary_stones.end(), 0);
            }

            node.obj = NULL; // Will be overwritten later...
            assert(node.figures.empty());
        }
    }
}

std::vector<MapPoint> GameWorld::PlaceObjects(const glArchivItem_Map& map)
{
    std::vector< MapPoint > headquarter_positions;
    MapPoint pt;
    for(pt.y = 0; pt.y < height_; ++pt.y)
    {
        for(pt.x = 0; pt.x < width_; ++pt.x)
        {
            unsigned char lc = map.GetMapDataAt(MAP_LANDSCAPE, pt.x, pt.y);
            noBase* obj = NULL;

            switch(map.GetMapDataAt(MAP_TYPE, pt.x, pt.y))
            {
                    // Player Startpos (provisorisch)
                case 0x80:
                {
                    headquarter_positions.push_back(pt);
                    if(lc < GAMECLIENT.GetPlayerCount())
                        GetPlayer(lc).hqPos = pt;
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
                        LOG.lprintf("Unbekannter Baum1-4 auf x=%d, y=%d: (0x%0X)\n", pt.x, pt.y, lc, lc);
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
                        LOG.lprintf("Unbekannter Baum5-8 auf x=%d, y=%d: (0x%0X)\n", pt.x, pt.y, lc, lc);
                } break;

                // Baum 9
                case 0xC6:
                {
                    if(lc >= 0x30 && lc <= 0x3D)
                        obj = new noTree(pt, 8, 3);
                    else
                        LOG.lprintf("Unbekannter Baum9 auf x=%d, y=%d: (0x%0X)\n", pt.x, pt.y, lc, lc);
                } break;

                // Sonstiges Naturzeug ohne Funktion, nur zur Dekoration
                case 0xC8:
                {
                    /// @todo mis0bobs unvollständig (dieses lagerzelt), 4 und 5 überhaupt nicht erwähnt
                    // mis1bobs, 2 und 3 sind vollständig eingebaut

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
                    // gestrandetes Schiff (mis0bobs, unvollständig)
                    else if(lc == 0x15)
                        obj = new noStaticObject(pt, (lc - 0x15) * 2, 0, 1);
                    // das Tor aus der map_?_z.lst
                    else if(lc == 0x16)
                        obj = new noStaticObject(pt, 560, 0xFFFF, 2);
                    // das geöffnete Tor aus map_?_z.lst
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
                    // die "kaputten" Gebäuderuinen usw (mis2bobs)
                    else if(lc >= 0x2C && lc <= 0x2D)
                        obj = new noStaticObject(pt, (lc - 0x2C) * 2, 2);
                    else if(lc == 0x2E)
                        obj = new noStaticObject(pt, (lc - 0x2C) * 2, 2, 1);
                    else if(lc == 0x2F)
                        obj = new noStaticObject(pt, (lc - 0x2C) * 2, 2, 2);
                    else if(lc == 0x30)
                        obj = new noEnvObject(pt, (lc - 0x2C) * 2, 2);
                    // der Wikinger (mis3bobs)
                    else if(lc == 0x31)
                        obj = new noStaticObject(pt, 0, 2);
                    else
                        LOG.lprintf("Unbekanntes Naturzeug auf x=%d, y=%d: (0x%0X)\n", pt.x, pt.y, lc, lc);

                } break;

                // Granit Typ 1
                case 0xCC:
                {
                    if(lc >= 0x01 && lc <= 0x06)
                        obj = new noGranite(GT_1, lc - 1);
                    else
                        LOG.lprintf("Unbekannter Granit1 auf x=%d, y=%d: (0x%0X)\n", pt.x, pt.y, lc, lc);
                } break;

                // Granit Typ 2
                case 0xCD:
                {
                    if(lc >= 0x01 && lc <= 0x06)
                        obj = new noGranite(GT_2, lc - 1);
                    else
                        LOG.lprintf("Unbekannter Granit2 auf x=%d, y=%d: (0x%0X)\n", pt.x, pt.y, lc, lc);
                } break;

                // Nichts
                case 0:
                break;

                default:
#ifndef NDEBUG
                unsigned char unknownObj = map.GetMapDataAt(MAP_TYPE, pt.x, pt.y);
                LOG.lprintf("Unbekanntes Objekt (0x%0X) auf x=%d, y=%d: (0x%0X)\n", unknownObj, unknownObj, pt.x, pt.y, lc, lc); 
#endif // !NDEBUG
                break;
            }

            GetNode(pt).obj = obj;
        }
    }
    return headquarter_positions;
        }

void GameWorld::PlaceAnimals(const glArchivItem_Map& map)
    {
    // Tiere auslesen
    MapPoint pt;
    for(pt.y = 0; pt.y < height_; ++pt.y)
    {
        for(pt.x = 0; pt.x < width_; ++pt.x)
        {
            Species species;
            switch(map.GetMapDataAt(MAP_ANIMALS, pt.x, pt.y))
            {
                    // TODO: Welche ID ist Polarbär?
                case 1: species = Species(SPEC_RABBITWHITE+RANDOM.Rand(__FILE__, __LINE__, 0, 2)); break; // zufällige Hasenart nehmen
                case 2: species = SPEC_FOX; break;
                case 3: species = SPEC_STAG; break;
                case 4: species = SPEC_DEER; break;
                case 5: species = SPEC_DUCK; break;
                case 6: species = SPEC_SHEEP; break;
            case 0: species = SPEC_NOTHING; break;
            default:
#ifndef NDEBUG
                unsigned char unknownAnimal = map.GetMapDataAt(MAP_ANIMALS, pt.x, pt.y);
                LOG.lprintf("Unknown animal species at x=%d, y=%d: (0x%0X)\n", pt.x, pt.y, unknownAnimal, unknownAnimal);
#endif // !NDEBUG
                species = SPEC_NOTHING;
                break;
            }

            if(species != SPEC_NOTHING)
            {
                noAnimal* animal = new noAnimal(species, pt);
                AddFigure(animal, pt);
                // Loslaufen
                animal->StartLiving();
            }
        }
    }
}

// random function using RANDOM.Rand(...) for std::random_shuffle
struct RandomFunctor
    {
    ptrdiff_t operator()(ptrdiff_t max) const
        {
        return(RANDOM.Rand(__FILE__, __LINE__, 0, max));
    }
};

void GameWorld::PlaceHQs(std::vector<MapPoint>& headquarter_positions)
            {
    assert(headquarter_positions.size() >= GAMECLIENT.GetPlayerCount());

    //random locations? -> randomize them :)
    if (GAMECLIENT.GetGGS().random_location)
    {
        RandomFunctor random;
        std::random_shuffle(headquarter_positions.begin(), headquarter_positions.end(), random);

        for (unsigned i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
            GetPlayer(i).hqPos = headquarter_positions.at(i);
    }

    // HQ setzen
    for(unsigned i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
    {
        // Existiert überhaupt ein HQ?
        GameClientPlayer& player = GetPlayer(i);
        if(player.hqPos.isValid())
        {
            if(player.ps == PS_OCCUPIED || player.ps == PS_KI)
            {
                nobHQ* hq = new nobHQ(player.hqPos, i, player.nation);
                SetNO(hq, player.hqPos);
                player.AddWarehouse(reinterpret_cast<nobBaseWarehouse*>(hq));
        }
    }
    }
}

void GameWorld::InitSeasAndHarbors()
    {
    /// Weltmeere vermessen
    MapPoint pt;
        for(pt.y = 0; pt.y < height_; ++pt.y)
        {
            for(pt.x = 0; pt.x < width_; ++pt.x)
            {
            // Noch kein Meer an diesem Punkt  Aber trotzdem Teil eines noch nicht vermessenen Meeres?
            if(!GetNode(pt).sea_id && IsSeaPoint(pt))
                {
                unsigned sea_size = MeasureSea(pt, seas.size());
                seas.push_back(Sea(sea_size));
                }
            }
        }

    /// Die Meere herausfinden, an die die Hafenpunkte grenzen
    for(unsigned i = 0; i < harbor_pos.size(); ++i)
    {
        for(unsigned z = 0; z < 6; ++z)
            harbor_pos[i].cps[z].sea_id = IsCoastalPoint(GetNeighbour(harbor_pos[i].pos, z));
    }

    // Nachbarn der einzelnen Hafenplätze ermitteln
    CalcHarborPosNeighbors();
}

void GameWorld::Serialize(SerializedGameData& sgd) const
{
    // Headinformationen
    sgd.PushUnsignedShort(width_);
    sgd.PushUnsignedShort(height_);
    sgd.PushUnsignedChar(static_cast<unsigned char>(lt));

    // Obj-ID-Counter reinschreiben
    sgd.PushUnsignedInt(GameObject::GetObjIDCounter());

    /// Serialize trade graphs first if they exist
    // Only if trade is enabled
    if(GAMECLIENT.GetGGS().isEnabled(ADDON_TRADE))
    {
        sgd.PushUnsignedChar(static_cast<unsigned char>(tgs.size()));
        for(unsigned i = 0; i < tgs.size(); ++i)
            tgs[i]->Serialize(sgd);
    }


    // Alle Weltpunkte serialisieren
    for(std::vector<MapNode>::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
        it->Serialize(sgd);
        }

    // Katapultsteine serialisieren
    sgd.PushObjectContainer(catapult_stones, true);
    // Meeresinformationen serialisieren
    sgd.PushUnsignedInt(seas.size());
    for(unsigned i = 0; i < seas.size(); ++i)
    {
        sgd.PushUnsignedInt(seas[i].nodes_count);
    }
    // Hafenpositionen serialisieren
    sgd.PushUnsignedInt(harbor_pos.size());
    for(unsigned i = 0; i < harbor_pos.size(); ++i)
    {
        sgd.PushMapPoint(harbor_pos[i].pos);
        for(unsigned z = 0; z < 6; ++z)
            sgd.PushUnsignedShort(harbor_pos[i].cps[z].sea_id);
        for(unsigned z = 0; z < 6; ++z)
        {
            sgd.PushUnsignedInt(harbor_pos[i].neighbors[z].size());

            for(unsigned c = 0; c < harbor_pos[i].neighbors[z].size(); ++c)
            {
                sgd.PushUnsignedInt(harbor_pos[i].neighbors[z][c].id);
                sgd.PushUnsignedInt(harbor_pos[i].neighbors[z][c].distance);
            }
        }
    }

    sgd.PushObjectContainer(harbor_building_sites_from_sea, true);
}

void GameWorld::Deserialize(SerializedGameData& sgd)
{
    // Headinformationen
    width_ = sgd.PopUnsignedShort();
    height_ = sgd.PopUnsignedShort();
    lt = LandscapeType(sgd.PopUnsignedChar());

    // Initialisierungen
    Init();

    // Obj-ID-Counter setzen
    GameObject::SetObjIDCounter(sgd.PopUnsignedInt());

    // Trade graphs
    // Only if trade is enabled
    if(GAMECLIENT.GetGGS().isEnabled(ADDON_TRADE))
    {
        tgs.resize(sgd.PopUnsignedChar());
        for(unsigned i = 0; i < tgs.size(); ++i)
            tgs[i] = new TradeGraph(sgd, this);
    }

    // Alle Weltpunkte
    MapPoint curPos(0,0);
    for(std::vector<MapNode>::iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
        it->Deserialize(sgd);
        if (it->harbor_id)
        {
            HarborPos p(curPos);
            harbor_pos.push_back(p);
        }
        curPos.x++;
        if(curPos.x >= width_)
        {
            curPos.x = 0;
            curPos.y++;
            }
            }

    // Katapultsteine deserialisieren
    sgd.PopObjectContainer(catapult_stones, GOT_CATAPULTSTONE);

    // Meeresinformationen deserialisieren
    seas.resize(sgd.PopUnsignedInt());
    for(unsigned i = 0; i < seas.size(); ++i)
    {
        seas[i].nodes_count = sgd.PopUnsignedInt();
    }

    // Hafenpositionen serialisieren
    harbor_pos.resize(sgd.PopUnsignedInt());
    for(unsigned i = 0; i < harbor_pos.size(); ++i)
    {
        harbor_pos[i].pos = sgd.PopMapPoint();
        for(unsigned z = 0; z < 6; ++z)
            harbor_pos[i].cps[z].sea_id = sgd.PopUnsignedShort();
        for(unsigned z = 0; z < 6; ++z)
        {
            harbor_pos[i].neighbors[z].resize(sgd.PopUnsignedInt());
            for(unsigned c = 0; c < harbor_pos[i].neighbors[z].size(); ++c)
            {
                harbor_pos[i].neighbors[z][c].id = sgd.PopUnsignedInt();
                harbor_pos[i].neighbors[z][c].distance = sgd.PopUnsignedInt();
            }
        }
    }

    sgd.PopObjectContainer(harbor_building_sites_from_sea, GOT_BUILDINGSITE);

    // BQ neu berechnen
    for(unsigned y = 0; y < height_; ++y)
    {
        for(unsigned x = 0; x < width_; ++x)
        {
            SetBQ(MapPoint(x, y), GAMECLIENT.GetPlayerID());
        }
    }

    tr.GenerateOpenGL(*this);

    // Zum HQ am Anfang springen, falls dieses existiert
    if(GetPlayer(GAMECLIENT.GetPlayerID()).hqPos.x != 0xFFFF)
        this->MoveToMapObject(GetPlayer(GAMECLIENT.GetPlayerID()).hqPos);
}


void GameWorld::ImportantObjectDestroyed(const MapPoint pt)
{
    WINDOWMANAGER.Close(CreateGUIID(pt));
}

void GameWorld::MilitaryBuildingCaptured(const MapPoint pt, const unsigned char player)
{
    if(player == GAMECLIENT.GetPlayerID())
        LOADER.GetSoundN("sound", 110)->Play(255, false);
    }

/// Vermisst ein neues Weltmeer von einem Punkt aus, indem es alle mit diesem Punkt verbundenen
/// Wasserpunkte mit der gleichen ID belegt und die Anzahl zurückgibt
unsigned GameWorld::MeasureSea(const MapPoint start, const unsigned short sea_id)
{
    // Breitensuche von diesem Punkt aus durchführen
    std::vector<bool> visited(width_ * height_, false);
    std::queue< MapPoint > todo;

    todo.push(start);
    visited[GetIdx(start)] = true;

    // Knoten zählen (Startknoten schon mit inbegriffen)
    unsigned count = 0;

    while(!todo.empty())
    {
        MapPoint p = todo.front();
        todo.pop();

        assert(visited[GetIdx(p)]);
        GetNode(p).sea_id = sea_id;

        for(unsigned i = 0; i < 6; ++i)
        {
            MapPoint neighbourPt = GetNeighbour(p, i);
            if(visited[GetIdx(neighbourPt)])
                continue;
            visited[GetIdx(neighbourPt)] = true;

            // Ist das dort auch ein Meerespunkt?
            if(IsSeaPoint(neighbourPt))
                todo.push(neighbourPt);
            }

        ++count;
    }

    return count;
}