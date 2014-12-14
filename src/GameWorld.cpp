// $Id: GameWorld.cpp 9540 2014-12-14 11:32:47Z marcus $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#include "main.h"
#include "GameWorld.h"
#include "Loader.h"
#include "GameClient.h"
#include "noEnvObject.h"
#include "noStaticObject.h"
#include "noGranite.h"
#include "noTree.h"
#include "noFlag.h"
#include "nobHQ.h"
#include "noFire.h"
#include "nobUsual.h"
#include "noAnimal.h"
#include "CatapultStone.h"
#include "noBuildingSite.h"
#include "Random.h"
#include "TradeGraph.h"
#include <queue>
#include <algorithm>

#include "WindowManager.h"
#include "SoundManager.h"

/// Lädt eine Karte
bool GameWorld::LoadMap(const std::string& filename)
{
    // Map laden
    libsiedler2::ArchivInfo ai;

    // Karteninformationen laden
    if(libsiedler2::loader::LoadMAP(filename.c_str(), &ai) != 0)
        return false;

    glArchivItem_Map* map = static_cast<glArchivItem_Map*>(ai.get(0));

    std::string lua_file = filename.substr(0, filename.length() - 3);
    lua_file.append("lua");

    if (luaL_dofile(lua, lua_file.c_str()))
    {
        fprintf(stderr, "LUA ERROR: '%s'!\n", lua_tostring(lua, -1));
        lua_pop(lua, 1);
    }

    Scan(map);

    CreateTradeGraphs();

    tr.GenerateOpenGL(this);

    if(GetPlayer(GameClient::inst().GetPlayerID())->hqx != 0xFFFF)
        this->MoveToMapObject(GetPlayer(GameClient::inst().GetPlayerID())->hqx,
                              GetPlayer(GameClient::inst().GetPlayerID())->hqy);

    LUA_EventStart();

    return true;
}

// random function using RANDOM.Rand(...) for std::random_shuffle
ptrdiff_t GameWorld::myRandom(ptrdiff_t max)
{
    return(RANDOM.Rand(__FILE__, __LINE__, 0, max));
}

void GameWorld::Scan(glArchivItem_Map* map)
{
    width = map->getHeader().getWidth();
    height = map->getHeader().getHeight();
    lt = LandscapeType(map->getHeader().getGfxSet());

    Init();

    // Dummy-Hafenpos für Index 0 einfügen // ask Oliverr why!
    // -> I just did, the dummy is so that the harbor "0" might be used for ships with no particular destination
    // poc: if you ever remove this dummy go to GameWorldBase::CalcDistanceToNearestHarbor and fix the loop to include the first harbor again (think Ive seen other instances of dummyadjusted loops as well...)
    GameWorldBase::HarborPos dummy = {0, 0};
    harbor_pos.push_back(dummy);

    // Andere Sachen setzen
    for(unsigned short y = 0; y < height; ++y)
    {
        for(unsigned short x = 0; x < width; ++x)
        {
            MapNode& node = nodes[y * width + x];

            node.roads[2] = node.roads[1] = node.roads[0] = 0;
            node.roads_real[2] = node.roads_real[1] = node.roads_real[0] = false;
            node.altitude = map->GetMapDataAt(MAP_ALTITUDE, x, y);
            // Aufpassen, dass die Terrainindizes im Rahmen liegen, ansonsten 0 nehmen, unbekanntes Terrain (Bsp.
            // Karte "Drachenebene")
            unsigned char t1 = map->GetMapDataAt(MAP_TERRAIN1, x, y), t2 = map->GetMapDataAt(MAP_TERRAIN2, x, y);

            // Hafenplatz?
            if(t1 >= 0x40 && t1 <= 0x54)
            {
                t1 -= 0x40;

                GameWorldBase::HarborPos p = {x, y};
                node.harbor_id = harbor_pos.size();
                harbor_pos.push_back(p);
            }
            else
                node.harbor_id = 0;



            node.t1 = (t1 < 20) ? TERRAIN_INDIZES[t1] : 0;
            node.t2 = (t2 < 20) ? TERRAIN_INDIZES[t2] : 0;

            node.resources = map->GetMapDataAt(MAP_RESOURCES, x, y);

            // Wasser?
            if(node.resources == 0x20 || node.resources == 0x21)
            {
                // TODO: Berge hatten komische Wasserbeeinflussung
                // ggf 0-4 Wasser setzen
                if( (node.t1 == TT_DESERT || node.t2 == TT_DESERT) ||
                        (node.t1 == TT_WATER || node.t2 == TT_WATER) )
                    node.resources = 0; // Kein Wasser, in der Wüste, da isses trocken!
                else if( (node.t1 == TT_STEPPE || node.t2 == TT_STEPPE) )
                    node.resources = 0x23; // 2 Wasser
                else if( (node.t1 == TT_SAVANNAH || node.t2 == TT_SAVANNAH) )
                    node.resources = 0x25; // 4 Wasser
                else
                    node.resources = 0x27; // 7 Wasser
            }

            node.reserved = false;
            node.owner = 0;
            for(unsigned i = 0; i < 4; ++i)
                node.boundary_stones[i] = 0;
            node.sea_id = 0;

            // FOW-Zeug initialisieren
            for(unsigned i = 0; i < GameClient::inst().GetPlayerCount(); ++i)
            {
                switch(GameClient::inst().GetGGS().exploration)
                {
                    case GlobalGameSettings::EXP_DISABLED:
                    {
                        node.fow[i].visibility = VIS_VISIBLE;
                    } break;
                    case GlobalGameSettings::EXP_CLASSIC:
                    {
                        node.fow[i].visibility = VIS_INVISIBLE;
                    } break;
                    case GlobalGameSettings::EXP_FOGOFWAR:
                    {
                        node.fow[i].visibility = VIS_INVISIBLE;
                    } break;
                    case GlobalGameSettings::EXP_FOGOFWARE_EXPLORED:
                    {
                        node.fow[i].visibility = VIS_FOW;
                    } break;
                }

                node.fow[i].last_update_time = 0;
                node.fow[i].object = NULL;
                node.fow[i].roads[0] = node.fow[i].roads[1] =
                                           node.fow[i].roads[2] = 0;
                node.fow[i].owner = 0;
                for(unsigned z = 0; z < 4; ++z)
                    node.fow[i].boundary_stones[z] = 0;
            }
        }
    }

    std::vector< Point<MapCoord> > headquarter_positions;

    // Objekte auslesen
    for(unsigned y = 0; y < height; ++y)
    {
        for(unsigned x = 0; x < width; ++x)
        {
            unsigned int pos = y * width + x;
            unsigned char lc = map->GetMapDataAt(MAP_LANDSCAPE, x, y);

            switch(map->GetMapDataAt(MAP_TYPE, x, y))
            {
                    // Player Startpos (provisorisch)
                case 0x80:
                {
                    headquarter_positions.push_back(Point<MapCoord>(x, y));

                    if(lc < GAMECLIENT.GetPlayerCount())
                    {
                        GetPlayer(lc)->hqx = x;
                        GetPlayer(lc)->hqy = y;
                        nodes[pos].obj = NULL;
                    }
                } break;

                // Baum 1-4
                case 0xC4:
                {
                    if(lc >= 0x30 && lc <= 0x3D)
                        nodes[pos].obj = new noTree(x, y, 0, 3);
                    else if(lc >= 0x70 && lc <= 0x7D)
                        nodes[pos].obj = new noTree(x, y, 1, 3);
                    else if(lc >= 0xB0 && lc <= 0xBD)
                        nodes[pos].obj = new noTree(x, y, 2, 3);
                    else if(lc >= 0xF0 && lc <= 0xFD)
                        nodes[pos].obj = new noTree(x, y, 3, 3);
                    else
                    {
                        LOG.lprintf("Unbekannter Baum1-4 auf x=%d, y=%d: id=%d (0x%0X)\n", x, y, lc, lc);
                        nodes[pos].obj = NULL;
                    }
                } break;

                // Baum 5-8
                case 0xC5:
                {
                    if(lc >= 0x30 && lc <= 0x3D)
                        nodes[pos].obj = new noTree(x, y, 4, 3);
                    else if(lc >= 0x70 && lc <= 0x7D)
                        nodes[pos].obj = new noTree(x, y, 5, 3);
                    else if(lc >= 0xB0 && lc <= 0xBD)
                        nodes[pos].obj = new noTree(x, y, 6, 3);
                    else if(lc >= 0xF0 && lc <= 0xFD)
                        nodes[pos].obj = new noTree(x, y, 7, 3);
                    else
                    {
                        LOG.lprintf("Unbekannter Baum5-8 auf x=%d, y=%d: id=%d (0x%0X)\n", x, y, lc, lc);
                        nodes[pos].obj = NULL;
                    }
                } break;

                // Baum 9
                case 0xC6:
                {
                    if(lc >= 0x30 && lc <= 0x3D)
                        nodes[pos].obj = new noTree(x, y, 8, 3);
                    else
                    {
                        LOG.lprintf("Unbekannter Baum9 auf x=%d, y=%d: id=%d (0x%0X)\n", x, y, lc, lc);
                        nodes[pos].obj = NULL;
                    }
                } break;

                // Sonstiges Naturzeug ohne Funktion, nur zur Dekoration
                case 0xC8:
                {
                    /// @todo mis0bobs unvollständig (dieses lagerzelt), 4 und 5 überhaupt nicht erwähnt
                    // mis1bobs, 2 und 3 sind vollständig eingebaut

                    // Objekte aus der map_?_z.lst
                    if(lc <= 0x0A)
                        nodes[pos].obj = new noEnvObject(x, y, 500 + lc);

                    // "wasserstein" aus der map_?_z.lst
                    else if(lc == 0x0B)
                        nodes[pos].obj = new noStaticObject(x, y, 500 + lc);

                    // Objekte aus der map_?_z.lst
                    else if(lc >= 0x0C && lc <= 0x0F)
                        nodes[pos].obj = new noEnvObject(x, y, 500 + lc);

                    // Objekte aus der map.lst
                    else if(lc >= 0x10 && lc <= 0x14)
                        nodes[pos].obj = new noEnvObject(x, y, 542 + lc - 0x10);

                    // gestrandetes Schiff (mis0bobs, unvollständig)
                    else if(lc == 0x15)
                        nodes[pos].obj = new noStaticObject(x, y, (lc - 0x15) * 2, 0, 1);

                    // das Tor aus der map_?_z.lst
                    else if(lc == 0x16)
                        nodes[pos].obj = new noStaticObject(x, y, 560, 0xFFFF, 2);

                    // das geöffnete Tor aus map_?_z.lst
                    else if(lc == 0x17)
                        nodes[pos].obj = new noStaticObject(x, y, 561, 0xFFFF, 2);

                    // Stalagmiten (mis1bobs)
                    else if(lc >= 0x18 && lc <= 0x1E)
                        nodes[pos].obj = new noStaticObject(x, y, (lc - 0x18) * 2, 1);

                    // toter Baum (mis1bobs)
                    else if(lc >= 0x1F && lc <= 0x20)
                        nodes[pos].obj = new noStaticObject(x, y, 20 + (lc - 0x1F) * 2, 1);

                    // Gerippe (mis1bobs)
                    else if(lc == 0x21)
                        nodes[pos].obj = new noEnvObject(x, y, 30, 1);

                    // Objekte aus der map.lst
                    else if(lc >= 0x22 && lc <= 0x27)
                        nodes[pos].obj = new noEnvObject(x, y, 550 + lc - 0x22);

                    // Objekte aus der map.lst
                    else if(lc >= 0x28 && lc <= 0x2B)
                        nodes[pos].obj = new noEnvObject(x, y, 556 + lc - 0x28);

                    // die "kaputten" Gebäuderuinen usw (mis2bobs)
                    else if(lc >= 0x2C && lc <= 0x2D)
                        nodes[pos].obj = new noStaticObject(x, y, (lc - 0x2C) * 2, 2);
                    else if(lc == 0x2E)
                        nodes[pos].obj = new noStaticObject(x, y, (lc - 0x2C) * 2, 2, 1);
                    else if(lc == 0x2F)
                        nodes[pos].obj = new noStaticObject(x, y, (lc - 0x2C) * 2, 2, 2);
                    else if(lc == 0x30)
                        nodes[pos].obj = new noEnvObject(x, y, (lc - 0x2C) * 2, 2);

                    // der Wikinger (mis3bobs)
                    else if(lc == 0x31)
                        nodes[pos].obj = new noStaticObject(x, y, 0, 2);

                    else
                    {
                        LOG.lprintf("Unbekanntes Naturzeug auf x=%d, y=%d: id=%d (0x%0X)\n", x, y, lc, lc);
                        nodes[pos].obj = NULL;
                    }

                } break;

                // Granit Typ 1
                case 0xCC:
                {
                    if(lc >= 0x01 && lc <= 0x06)
                        nodes[pos].obj = new noGranite(GT_1, lc - 1);
                    else
                    {
                        LOG.lprintf("Unbekannter Granit1 auf x=%d, y=%d: id=%d (0x%0X)\n", x, y, lc, lc);
                        nodes[pos].obj = NULL;
                    }
                } break;

                // Granit Typ 2
                case 0xCD:
                {
                    if(lc >= 0x01 && lc <= 0x06)
                        nodes[pos].obj = new noGranite(GT_2, lc - 1);
                    else
                    {
                        LOG.lprintf("Unbekannter Granit2 auf x=%d, y=%d: id=%d (0x%0X)\n", x, y, lc, lc);
                        nodes[pos].obj = NULL;
                    }
                } break;

                // Nichts
                case 0:
                {
                    nodes[pos].obj = NULL;
                } break;

                default:
                {
                    /*LOG.lprintf("Unbekanntes Objekt %d (0x%0X) auf x=%d, y=%d: id=%d (0x%0X)\n", map->map_type[y*width+x], map->map_type[y*width+x], x, y, lc, lc);
                    */         nodes[pos].obj = NULL;
                } break;
            }
        }
    }

    // BQ mit nichts erstmal inititalisieren (HQ-Setzen berechnet diese neu und braucht sie)
    for(unsigned y = 0; y < height; ++y)
    {
        for(unsigned x = 0; x < width; ++x)
        {
            SetBQ(x, y, BQ_NOTHING);
        }
    }

    //random locations? -> randomize them :)
    if (GameClient::inst().GetGGS().random_location)
    {
        ptrdiff_t (*p_myrandom)(ptrdiff_t) = myRandom;
        std::random_shuffle(headquarter_positions.begin(), headquarter_positions.end(), p_myrandom);

        for (unsigned i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
        {
            GetPlayer(i)->hqx = headquarter_positions.at(i).x;
            GetPlayer(i)->hqy = headquarter_positions.at(i).y;
        }
    }

    // HQ setzen
    for(unsigned i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
    {
        // Existiert überhaupt ein HQ?
        if(GetPlayer(i)->hqx != 0xFFFF)
        {
            if(GetPlayer(i)->ps == PS_OCCUPIED || GetPlayer(i)->ps == PS_KI)
            {
                nobHQ* hq = new nobHQ(GetPlayer(i)->hqx, GetPlayer(i)->hqy, i, GetPlayer(i)->nation);
                SetNO(hq, GetPlayer(i)->hqx, GetPlayer(i)->hqy);
                GetPlayer(i)->AddWarehouse(reinterpret_cast<nobBaseWarehouse*>(hq));
            }
            /*else
                GetNode(GetPlayer(i)->hqx,GetPlayer(i)->hqy).obj = 0;*/
        }
    }

    // Tiere auslesen
    for(unsigned y = 0; y < height; ++y)
    {
        for(unsigned x = 0; x < width; ++x)
        {
            // Tiere setzen
            Species species;
            switch(map->GetMapDataAt(MAP_ANIMALS, x, y))
            {
                    // TODO: Welche ID ist Polarbär?
                case 1: species = Species(SPEC_RABBITWHITE+RANDOM.Rand(__FILE__, __LINE__, 0, 2)); break; // zufällige Hasenart nehmen
                case 2: species = SPEC_FOX; break;
                case 3: species = SPEC_STAG; break;
                case 4: species = SPEC_DEER; break;
                case 5: species = SPEC_DUCK; break;
                case 6: species = SPEC_SHEEP; break;
                default: species = SPEC_NOTHING; break;
            }

            if(species != SPEC_NOTHING)
            {
                noAnimal* animal = new noAnimal(species, x, y);
                AddFigure(animal, x, y);
                // Loslaufen
                animal->StartLiving();
            }

            /// 4 Fische setzen
            if(map->GetMapDataAt(MAP_RESOURCES, y * width + x) > 0x80 && map->GetMapDataAt(MAP_RESOURCES, y * width + x)  < 0x90)
                GetNode(x, y).resources = 0x84;
        }
    }

    /// Weltmeere vermessen
    for(unsigned y = 0; y < height; ++y)
    {
        for(unsigned x = 0; x < width; ++x)
        {
            // Noch kein Meer an diesem Punkt?
            if(!GetNode(x, y).sea_id)
            {
                // Aber trotzdem Teil eines noch nicht vermessenen Meeres?
                if(IsSeaPoint(x, y))
                {
                    unsigned sea_size = MeasureSea(x, y, seas.size());
                    seas.push_back(Sea(sea_size));
                }
            }
        }
    }

    /// Die Meere herausfinden, an die die Hafenpunkte grenzen
    for(unsigned i = 0; i < harbor_pos.size(); ++i)
    {
        for(unsigned z = 0; z < 6; ++z)
            harbor_pos[i].cps[z].sea_id = IsCoastalPoint(GetXA(harbor_pos[i].x, harbor_pos[i].y, z),
                                          GetYA(harbor_pos[i].x, harbor_pos[i].y, z));
    }

    // Nachbarn der einzelnen Hafenplätze ermitteln
    CalcHarborPosNeighbors();

    /// Schatten und BQ berechnen
    for(unsigned y = 0; y < height; ++y)
    {
        for(unsigned x = 0; x < width; ++x)
        {
            RecalcShadow(x, y);
            SetBQ(x, y, GAMECLIENT.GetPlayerID());
        }
    }

    /// Bei FoW und aufgedeckt müssen auch die ersten FoW-Objekte erstellt werden
    if(GameClient::inst().GetGGS().exploration == GlobalGameSettings::EXP_FOGOFWARE_EXPLORED)
    {
        for(unsigned y = 0; y < height; ++y)
        {
            for(unsigned x = 0; x < width; ++x)
            {
                // Alle Spieler durchgehen
                for(unsigned i = 0; i < GameClient::inst().GetPlayerCount(); ++i)
                {
                    // An der Stelle FOW für diesen Spieler?
                    if(GetNode(x, y).fow[i].visibility == VIS_FOW)
                        SaveFOWNode(x, y, i);
                }
            }
        }
    }

}

void GameWorld::Serialize(SerializedGameData* sgd) const
{
    // Headinformationen
    sgd->PushUnsignedShort(width);
    sgd->PushUnsignedShort(height);
    sgd->PushUnsignedChar(static_cast<unsigned char>(lt));

    // Obj-ID-Counter reinschreiben
    sgd->PushUnsignedInt(GameObject::GetObjIDCounter());

    /// Serialize trade graphs first if they exist
    // Only if trade is enabled
    if(GameClient::inst().GetGGS().isEnabled(ADDON_TRADE))
    {
        sgd->PushUnsignedChar(static_cast<unsigned char>(tgs.size()));
        for(unsigned i = 0; i < tgs.size(); ++i)
            tgs[i]->Serialize(sgd);
    }


    // Alle Weltpunkte serialisieren
    for(unsigned i = 0; i < map_size; ++i)
    {
        for(unsigned z = 0; z < 3; ++z)
        {
            if(nodes[i].roads_real[z])
                sgd->PushUnsignedChar(nodes[i].roads[z]);
            else
                sgd->PushUnsignedChar(0);
        }

        sgd->PushUnsignedChar(nodes[i].altitude);
        sgd->PushUnsignedChar(nodes[i].shadow);
        sgd->PushUnsignedChar(nodes[i].t1);
        sgd->PushUnsignedChar(nodes[i].t2);
        sgd->PushUnsignedChar(nodes[i].resources);
        sgd->PushBool(nodes[i].reserved);
        sgd->PushUnsignedChar(nodes[i].owner);
        for(unsigned b = 0; b < 4; ++b)
            sgd->PushUnsignedChar(nodes[i].boundary_stones[b]);
        sgd->PushUnsignedChar(static_cast<unsigned char>(nodes[i].bq));
        for(unsigned z = 0; z < GameClient::inst().GetPlayerCount(); ++z)
        {
            sgd->PushUnsignedChar(static_cast<unsigned char>(nodes[i].fow[z].visibility));
            // Nur im FoW können FOW-Objekte stehen
            if(nodes[i].fow[z].visibility == VIS_FOW)
            {
                sgd->PushUnsignedInt(nodes[i].fow[z].last_update_time);
                sgd->PushFOWObject(nodes[i].fow[z].object);
                for(unsigned r = 0; r < 3; ++r)
                    sgd->PushUnsignedChar(nodes[i].fow[z].roads[r]);
                sgd->PushUnsignedChar(nodes[i].fow[z].owner);
                for(unsigned b = 0; b < 4; ++b)
                    sgd->PushUnsignedChar(nodes[i].fow[z].boundary_stones[b]);
            }
        }
        sgd->PushObject(nodes[i].obj, false);
        sgd->PushObjectList(nodes[i].figures, false);
        sgd->PushUnsignedShort(nodes[i].sea_id);
        sgd->PushUnsignedInt(nodes[i].harbor_id);
    }

    // Katapultsteine serialisieren
    sgd->PushObjectList<CatapultStone>(catapult_stones, true);
    // Meeresinformationen serialisieren
    sgd->PushUnsignedInt(seas.size());
    for(unsigned i = 0; i < seas.size(); ++i)
    {
        sgd->PushUnsignedInt(seas[i].nodes_count);
    }
    // Hafenpositionen serialisieren
    sgd->PushUnsignedInt(harbor_pos.size());
    for(unsigned i = 0; i < harbor_pos.size(); ++i)
    {
        sgd->PushUnsignedShort(harbor_pos[i].x);
        sgd->PushUnsignedShort(harbor_pos[i].y);
        for(unsigned z = 0; z < 6; ++z)
            sgd->PushUnsignedShort(harbor_pos[i].cps[z].sea_id);
        for(unsigned z = 0; z < 6; ++z)
        {
            sgd->PushUnsignedInt(harbor_pos[i].neighbors[z].size());

            for(unsigned c = 0; c < harbor_pos[i].neighbors[z].size(); ++c)
            {
                sgd->PushUnsignedInt(harbor_pos[i].neighbors[z][c].id);
                sgd->PushUnsignedInt(harbor_pos[i].neighbors[z][c].distance);
            }
        }
    }

    sgd->PushObjectList(harbor_building_sites_from_sea, true);
}

void GameWorld::Deserialize(SerializedGameData* sgd)
{
    // Headinformationen
    width = sgd->PopUnsignedShort();
    height = sgd->PopUnsignedShort();
    lt = LandscapeType(sgd->PopUnsignedChar());

    // Initialisierungen
    Init();

    // Obj-ID-Counter setzen
    GameObject::SetObjIDCounter(sgd->PopUnsignedInt());

    // Trade graphs
    // Only if trade is enabled
    if(GameClient::inst().GetGGS().isEnabled(ADDON_TRADE))
    {
        tgs.resize(sgd->PopUnsignedChar());
        for(unsigned i = 0; i < tgs.size(); ++i)
            tgs[i] = new TradeGraph(sgd, this);
    }

    // Alle Weltpunkte serialisieren
    for(unsigned i = 0; i < map_size; ++i)
    {
        for(unsigned z = 0; z < 3; ++z)
        {
            nodes[i].roads[z] = sgd->PopUnsignedChar();
            nodes[i].roads_real[z] = nodes[i].roads[z] ? true : false;
        }


        nodes[i].altitude = sgd->PopUnsignedChar();
        nodes[i].shadow = sgd->PopUnsignedChar();
        nodes[i].t1 = sgd->PopUnsignedChar();
        nodes[i].t2 = sgd->PopUnsignedChar();
        nodes[i].resources = sgd->PopUnsignedChar();
        nodes[i].reserved = sgd->PopBool();
        nodes[i].owner = sgd->PopUnsignedChar();
        for(unsigned b = 0; b < 4; ++b)
            nodes[i].boundary_stones[b] = sgd->PopUnsignedChar();
        nodes[i].bq = BuildingQuality(sgd->PopUnsignedChar());
        for(unsigned z = 0; z < GameClient::inst().GetPlayerCount(); ++z)
        {
            nodes[i].fow[z].visibility = Visibility(sgd->PopUnsignedChar());
            // Nur im FoW können FOW-Objekte stehen
            if(nodes[i].fow[z].visibility == VIS_FOW)
            {
                nodes[i].fow[z].last_update_time = sgd->PopUnsignedInt();
                nodes[i].fow[z].object = sgd->PopFOWObject();
                for(unsigned r = 0; r < 3; ++r)
                    nodes[i].fow[z].roads[r] = sgd->PopUnsignedChar();
                nodes[i].fow[z].owner = sgd->PopUnsignedChar();
                for(unsigned b = 0; b < 4; ++b)
                    nodes[i].fow[z].boundary_stones[b] = sgd->PopUnsignedChar();
            }
            else
            {
                nodes[i].fow[z].last_update_time = 0;
                nodes[i].fow[z].object = NULL;
                for(unsigned r = 0; r < 3; ++r)
                    nodes[i].fow[z].roads[r] = 0;
                nodes[i].fow[z].owner = 0;
                for(unsigned b = 0; b < 4; ++b)
                    nodes[i].fow[z].boundary_stones[b] = 0;
            }
        }
        nodes[i].obj = sgd->PopObject<noBase>(GOT_UNKNOWN);
        sgd->PopObjectList<noBase>(nodes[i].figures, GOT_UNKNOWN);
        nodes[i].sea_id = sgd->PopUnsignedShort();
        nodes[i].harbor_id = sgd->PopUnsignedInt();

        if (nodes[i].harbor_id)
        {
            GameWorldBase::HarborPos  p = {(MapCoord) (i % width), (MapCoord) (i / width)};
            harbor_pos.push_back(p);
        }
    }

    // Katapultsteine deserialisieren
    sgd->PopObjectList(catapult_stones, GOT_CATAPULTSTONE);

    // Meeresinformationen deserialisieren
    seas.resize(sgd->PopUnsignedInt());
    for(unsigned i = 0; i < seas.size(); ++i)
    {
        seas[i].nodes_count = sgd->PopUnsignedInt();
    }

    // Hafenpositionen serialisieren
    harbor_pos.resize(sgd->PopUnsignedInt());
    for(unsigned i = 0; i < harbor_pos.size(); ++i)
    {
        harbor_pos[i].x = sgd->PopUnsignedShort();
        harbor_pos[i].y = sgd->PopUnsignedShort();
        for(unsigned z = 0; z < 6; ++z)
            harbor_pos[i].cps[z].sea_id = sgd->PopUnsignedShort();
        for(unsigned z = 0; z < 6; ++z)
        {
            harbor_pos[i].neighbors[z].resize(sgd->PopUnsignedInt());
            for(unsigned c = 0; c < harbor_pos[i].neighbors[z].size(); ++c)
            {
                harbor_pos[i].neighbors[z][c].id = sgd->PopUnsignedInt();
                harbor_pos[i].neighbors[z][c].distance = sgd->PopUnsignedInt();
            }
        }
    }

    sgd->PopObjectList<noBuildingSite>(harbor_building_sites_from_sea, GOT_BUILDINGSITE);

    // BQ neu berechnen
    for(unsigned y = 0; y < height; ++y)
    {
        for(unsigned x = 0; x < width; ++x)
        {
            SetBQ(x, y, GAMECLIENT.GetPlayerID());
        }
    }

    tr.GenerateOpenGL(this);

    // Zum HQ am Anfang springen, falls dieses existiert
    if(GetPlayer(GameClient::inst().GetPlayerID())->hqx != 0xFFFF)
        this->MoveToMapObject(GetPlayer(GameClient::inst().GetPlayerID())->hqx,
                              GetPlayer(GameClient::inst().GetPlayerID())->hqy);
}


void GameWorld::ImportantObjectDestroyed(const unsigned short x, const MapCoord y)
{
    WindowManager::inst().Close(CreateGUIID(x, y));
}

void GameWorld::MilitaryBuildingCaptured(const unsigned short x, const MapCoord y, const unsigned char player)
{
    if(player == GAMECLIENT.GetPlayerID())
    {
        /*  int this_x,this_y = y-3*int(height);

            for(unsigned sy = 0;sy<6;++sy,this_y+=int(height))
            {
                this_x = x-3*int(width);
                for(unsigned sx = 0;sx<6;++sx,this_x+=int(width))
                {
                    if(this_x > GetFirstX() && this_x < GetLastX()
                        && this_y > GetFirstY() && this_y < GetLastY())*/
        LOADER.GetSoundN("sound", 110)->Play(255, false);
        /*  }
        }*/

    }
}

/// Vermisst ein neues Weltmeer von einem Punkt aus, indem es alle mit diesem Punkt verbundenen
/// Wasserpunkte mit der gleichen ID belegt und die Anzahl zurückgibt
unsigned GameWorld::MeasureSea(const MapCoord x, const MapCoord y, const unsigned short sea_id)
{
    // Breitensuche von diesem Punkt aus durchführen
    std::vector<bool> visited(width * height, false);
    std::queue< Point<MapCoord> > todo;

    Point<MapCoord> start(x, y);
    todo.push(start);

    // Knoten zählen (Startknoten schon mit inbegriffen)
    unsigned count = 0;

    while(!todo.empty())
    {
        Point<MapCoord> p = todo.front();
        todo.pop();

        if(visited[p.y * width + p.x])
            continue;

        GetNode(p.x, p.y).sea_id = sea_id;

        for(unsigned i = 0; i < 6; ++i)
        {
            MapCoord xa, ya;
            xa = GetXA(p.x, p.y, i);
            ya = GetYA(p.x, p.y, i);

            // Ist das dort auch ein Meerespunkt?
            if(!IsSeaPoint(xa, ya))
                continue;

            if(!visited[ya * width + xa])
            {
                Point<MapCoord> add(xa, ya);
                todo.push(add);
            }
        }

        visited[p.y * width + p.x] = true;
        ++count;
    }

    return count;

}

//void GameWorld::RecalcAllVisibilities()
//{
//  for(unsigned y = 0;y<height;++y)
//  {
//      for(unsigned x = 0;x<width;++x)
//      {
//          for(unsigned i = 0;i<GameClient::inst().GetPlayerCount();++i)
//              RecalcVisibility(x,y,i,false);
//      }
//  }
//}
