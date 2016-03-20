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

#include "defines.h" // IWYU pragma: keep
#include "world/GameWorldView.h"
#include "drivers/VideoDriverWrapper.h"
#include "nodeObjs/noTree.h"
#include "buildings/nobUsual.h"
#include "buildings/nobMilitary.h"
#include "buildings/noBuildingSite.h"
#include "CatapultStone.h"
#include "GameClient.h"
#include "SoundManager.h"
#include "Loader.h"
#include "gameData/MapConsts.h"
#include "desktops/dskGameInterface.h"
#include "FOWObjects.h"

#include "GameServer.h"
#include "ai/AIPlayerJH.h"
#include "ogl/glArchivItem_Font.h"
#include "ogl/glSmartBitmap.h"
#include "helpers/converters.h"
#include <boost/format.hpp>
#include <stdexcept>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

GameWorldView::GameWorldView(const MapPoint pos, unsigned short width, unsigned short height):
	selPt(0, 0),
	debugNodePrinter(NULL),
	show_bq(false),
	show_names(false),
	show_productivity(false),
	offset(0, 0),
	lastOffset(0, 0),
	gwv(NULL),
	d_what(0),
	d_player(0),
	d_active(false),
	pos(pos),
	width(width), height(height),
	terrain_list(0),
	terrainLastOffset(0, 0),
	terrain_last_global_animation(0),
	terrain_last_water(0)
{
}

GameWorldView::~GameWorldView()
{
    if (terrain_list != 0)
        glDeleteLists(terrain_list, 1);
}

void GameWorldView::SetGameWorldViewer(GameWorldViewer* viewer)
{
    if(gwv)
        throw::std::logic_error("Tried to set gwv multiple times!");
    gwv = viewer;
}

struct ObjectBetweenLines
{
    noBase* obj;
    Point<int> pos; // Zeichenposition

    ObjectBetweenLines(noBase* obj, const Point<int>& pos) : obj(obj), pos(pos) {}
};

void GameWorldView::Draw(const unsigned char  /*player*/, unsigned* water, const bool draw_selected, const MapPoint selected, const RoadsBuilding& rb)
{

    int shortest_len = 100000;

    glScissor(pos.x, VIDEODRIVER.GetScreenHeight() - pos.y - height, width, height);

    TerrainRenderer& terrainRenderer = *gwv->GetTerrainRenderer();
    terrainRenderer.Draw(*this, water);

    glTranslatef((GLfloat) pos.x, (GLfloat) pos.y, 0.0f);

    // Draw-Counter der Bäume zurücksetzen vor jedem Zeichnen
    noTree::ResetDrawCounter();

    for(int y = firstPt.y; y < lastPt.y; ++y)
    {
        // Figuren speichern, die in dieser Zeile gemalt werden müssen
        // und sich zwischen zwei Zeilen befinden, da sie dazwischen laufen
        std::vector<ObjectBetweenLines> between_lines;

        for(int x = firstPt.x; x < lastPt.x; ++x)
        {
            Point<int> curOffset;
            MapPoint t = terrainRenderer.ConvertCoords(Point<int>(x, y), &curOffset);
            Point<int> curPos = Point<int>(gwv->GetNodePos(t)) - offset + curOffset;

            if(std::abs(VIDEODRIVER.GetMouseX() - curPos.x) + std::abs(VIDEODRIVER.GetMouseY() - curPos.y) < shortest_len)
            {
                selPt = t;
                selO = curOffset;
                shortest_len = std::abs(VIDEODRIVER.GetMouseX() - curPos.x) + std::abs(VIDEODRIVER.GetMouseY() - curPos.y);
            }

            Visibility visibility = gwv->GetVisibility(t);

            DrawBoundaryStone(x, y, t, curPos, visibility);

            /// Nur bei Sichtbaren Stellen zeichnen
            // Visible terrain
            if(visibility == VIS_VISIBLE)
            {
                ////////////////////////////////////////////////

                ///////////////////////////////////////
                // Draw objects and people

                // Draw objects - buildings, trees, stones, decoration sprites, etc.
                const MapNode& mapNode = gwv->GetNode(t);
                if(mapNode.obj)
                {
                    mapNode.obj->Draw(curPos.x, curPos.y);
                    if (false) //TODO: military aid - display icon overlay of attack possibility
                    {
                        noBuilding* building = gwv->GetSpecObj<noBuilding>(t);
                        if (mapNode.owner != GAMECLIENT.GetPlayerID() + 1 //not belonging to current player
                                && gwv->GetNO(t)->GetType() == NOP_BUILDING //is a building
                                && !GAMECLIENT.GetLocalPlayer().IsAlly(building->GetPlayer())) //not an ally
                        {
                            BuildingType bt = building->GetBuildingType();
                            if ((bt >= BLD_BARRACKS && bt <= BLD_FORTRESS)
                                    || bt == BLD_HEADQUARTERS
                                    || bt == BLD_HARBORBUILDING) //is it a military building?
                            {
                                if(gwv->GetAvailableSoldiersForAttack(GAMECLIENT.GetPlayerID(), t)) //soldiers available for attack?
                                    LOADER.GetImageN("map_new", 20000)->Draw(curPos.x + 1, curPos.y - 5, 0, 0, 0, 0, 0, 0);;
                            }
                        }
                    }
                }


                // People
                if(!mapNode.figures.empty())
                {
                    for(std::list<noBase*>::const_iterator it = mapNode.figures.begin(); it != mapNode.figures.end(); ++it)
                    {
                        // Bewegt er sich oder ist es ein Schiff?
                        if((*it)->IsMoving() || (*it)->GetGOT() == GOT_SHIP)
                            // Dann nach der gesamten Zeile zeichnen
                            between_lines.push_back(ObjectBetweenLines(*it, curPos));
                        else
                            // Ansonsten jetzt schon zeichnen
                            (*it)->Draw(curPos.x, curPos.y);
                    }
                }

                ////////////////////////////////////////////////

                //Construction aid mode
                if(show_bq && gwv->GetNode(t).bq && gwv->GetNode(t).bq < 7) //-V807
                {
                    BuildingQuality bq = gwv->GetNode(t).bq;
                    glArchivItem_Bitmap* bm = LOADER.GetMapImageN(49 + bq);
                    //Draw building quality icon
                    bm->Draw(curPos.x, curPos.y, 0, 0, 0, 0, 0, 0);
                    //Show ability to construct military buildings
                    if(GAMECLIENT.GetGGS().isEnabled(AddonId::MILITARY_AID))
                    {
                        if(!gwv->IsMilitaryBuildingNearNode(t, GAMECLIENT.GetPlayerID()) && (bq == BQ_HUT || bq == BQ_HOUSE || bq == BQ_CASTLE || bq == BQ_HARBOR))
                            LOADER.GetImageN("map_new", 20000)->Draw(curPos.x + 1, curPos.y - bm->getHeight() - 5, 0, 0, 0, 0, 0, 0);
                    }
                }
            }
            // im Nebel die FOW-Objekte zeichnen
            // Fog of war
            else if(visibility == VIS_FOW)
            {
                const FOWObject* fowobj = gwv->GetYoungestFOWObject(MapPoint(t));
                if(fowobj)
                    fowobj->Draw(curPos.x, curPos.y);
            }


            if(debugNodePrinter)
                debugNodePrinter->print(t, curPos);

            if (d_active)
            {
                std::stringstream ss;
                AIPlayerJH* ai = dynamic_cast<AIPlayerJH*>(GAMESERVER.GetAIPlayer(d_player));
                if (ai)
                {
                    if (d_what == 1)
                    {
                        if(ai->GetAINode(t).bq && ai->GetAINode(t).bq  < 7) //-V807
                            LOADER.GetMapImageN(49 + ai->GetAINode(t).bq)->Draw(curPos.x, curPos.y, 0, 0, 0, 0, 0, 0);
                    }
                    else if (d_what == 2)
                    {
                        if (ai->GetAINode(t).reachable)
                            LOADER.GetImageN("io", 32)->Draw(curPos.x, curPos.y, 0, 0, 0, 0, 0, 0);
                        else
                            LOADER.GetImageN("io", 40)->Draw(curPos.x, curPos.y, 0, 0, 0, 0, 0, 0);
                    }
                    else if (d_what == 3)
                    {
                        if (ai->GetAINode(t).farmed)
                            LOADER.GetImageN("io", 32)->Draw(curPos.x, curPos.y, 0, 0, 0, 0, 0, 0);
                        else
                            LOADER.GetImageN("io", 40)->Draw(curPos.x, curPos.y, 0, 0, 0, 0, 0, 0);
                    }
                    else if (d_what > 3 && d_what < 13)
                    {
                        ss << ai->GetResMapValue(t, AIJH::Resource(d_what - 4));
                        NormalFont->Draw(curPos.x, curPos.y, ss.str(), 0, 0xFFFFFF00);
                    }
                }
            }
        }

        // Figuren zwischen den Zeilen zeichnen
        for(unsigned i = 0; i < between_lines.size(); ++i)
            between_lines[i].obj->Draw(between_lines[i].pos.x, between_lines[i].pos.y);
    }

    // Names & Productivity overlay
    if(show_names || show_productivity)
    {
        for(int x = firstPt.x; x < lastPt.x; ++x)
        {
            for(int y = firstPt.y; y < lastPt.y; ++y)
            {
                // Coordinate transform
                Point<int> curOffset;
                MapPoint t = terrainRenderer.ConvertCoords(Point<int>(x, y), &curOffset);
                Point<int> curPos = Point<int>(gwv->GetNodePos(t)) - offset + curOffset;

                // Name bzw Produktivität anzeigen
                GO_Type got = gwv->GetNO(t)->GetGOT();
                if(IsBaseBuilding(got))
                {
                    noBaseBuilding* no = gwv->GetSpecObj<noBaseBuilding>(t);

                    // Is object not belonging to local player?
                    if(no->GetPlayer() != GAMECLIENT.GetPlayerID())
                        continue;

                    curPos.y -= 10;

                    // Draw object name
                    if(show_names)
                    {
                        unsigned int color = (no->GetGOT() == GOT_BUILDINGSITE) ? COLOR_GREY : COLOR_YELLOW;
                        SmallFont->Draw(curPos.x, curPos.y, _(BUILDING_NAMES[no->GetBuildingType()]), glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER, color);
                        curPos.y += SmallFont->getHeight();
                    }

                    //Draw productivity/soldiers
                    if(show_productivity)
                    {
                        switch(got)
                        {
                            case GOT_BUILDINGSITE: // Is object a building construction site?
                            {
                                noBuildingSite* n = static_cast<noBuildingSite*>(no);
                                if(n)
                                {
                                    char text[256];
                                    unsigned int color = COLOR_GREY;

                                    unsigned short p = n->GetBuildProgress();
                                    snprintf(text, 256, "(%d %%)", p);
                                    SmallFont->Draw(curPos.x, curPos.y, text, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER, color);
                                }
                            }
                            break;

                            case GOT_NOB_USUAL: // Is it a normal building or shipyard?
                            case GOT_NOB_SHIPYARD:
                            {
                                nobUsual* n = dynamic_cast<nobUsual*>(no);
                                if(n)
                                {
                                    std::string text;
                                    unsigned int color = COLOR_RED;

                                    if(!n->HasWorker())
                                        text = _("(House unoccupied)");
                                    else if(n->IsProductionDisabledVirtual())
                                        text = _("(stopped)");
                                    else
                                    {
                                        // Catapult and Lookout tower doesn't have productivity!
                                        if(n->GetBuildingType() == BLD_CATAPULT || n->GetBuildingType() == BLD_LOOKOUTTOWER)
                                            text[0] = 0;
                                        else
                                        {
                                            unsigned short p = n->GetProductivity();
                                            text = helpers::toString(p) + " %";
                                            if(p >= 60)
                                                color = 0xFF00E000;
                                            else if(p >= 30)
                                                color = 0xFFFFFF00;
                                            else if(p >= 20)
                                                color = 0xFFFF8000;
                                        }
                                    }
                                    SmallFont->Draw(curPos.x, curPos.y, text, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER, color);
                                }
                            }
                            break;

                            case GOT_NOB_MILITARY: // Is it a military building?
                            {
                                // Display amount of soldiers
                                unsigned soldiers_count = static_cast<nobMilitary*>(no)->GetTroopsCount();
                                std::string sSoldiers;
                                if(soldiers_count == 1)
                                    sSoldiers = _("(1 soldier)");
                                else
                                    sSoldiers = boost::str(
                                            boost::format(_("(%d soldiers)")) % soldiers_count
                                            );


                                SmallFont->Draw(curPos.x, curPos.y, sSoldiers, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER,
                                                (soldiers_count > 0) ? COLOR_YELLOW : COLOR_RED);
                                curPos.y += SmallFont->getHeight();
                            }
                            break;

                            default:
                                break;
                        }
                    }
                }
            }
        }
    }

    // GUI-Symbole auf der Map zeichnen

    // Falls im Straßenbaumodus: Punkte um den aktuellen Straßenbaupunkt herum ermitteln
    MapPoint road_points[6];

    if(rb.mode)
    {
        for(unsigned i = 0; i < 6; ++i)
        {
            road_points[i] = gwv->GetNeighbour(rb.point, i);
        }
    }

    for(int x = firstPt.x; x < lastPt.x; ++x)
    {
        for(int y = firstPt.y; y < lastPt.y; ++y)
        {
            // Coordinates transform
            Point<int> curOffset;
            MapPoint t = terrainRenderer.ConvertCoords(Point<int>(x, y), &curOffset);
            Point<int> curPos = Point<int>(gwv->GetNodePos(t)) - offset + curOffset;

            /// Current point indicated by Mouse
            if(selPt == t)
            {
                // Mauszeiger am boten
                unsigned mid = 22;
                switch(gwv->GetNode(t).bq)
                {
                    case BQ_FLAG: mid = 40; break;
                    case BQ_MINE: mid = 41; break;
                    case BQ_HUT: mid = 42; break;
                    case BQ_HOUSE: mid = 43; break;
                    case BQ_CASTLE: mid = 44; break;
                    case BQ_HARBOR: mid = 45; break;
                    default: break;
                }

                if(rb.mode)
                    mid = 22;

                LOADER.GetMapImageN(mid)->Draw(curPos.x, curPos.y, 0, 0, 0, 0, 0, 0);
            }

            // Currently selected point
            if(draw_selected && selected == t)
                LOADER.GetMapImageN(20)->Draw(curPos.x, curPos.y, 0, 0, 0, 0, 0, 0);

            // Wegbauzeug
            if(rb.mode)
            {
                if(rb.point == t)
                    LOADER.GetMapImageN(21)->Draw(curPos.x, curPos.y, 0, 0, 0, 0, 0, 0);



                int altitude = gwv->GetNode(rb.point).altitude;

                const unsigned char waterway_lengthes[] = {3, 5, 9, 13, 21, 0}; // these are written into dskGameInterface.cpp, too
                const unsigned char index = GAMECLIENT.GetGGS().getSelection(AddonId::MAX_WATERWAY_LENGTH);
                RTTR_Assert(index <= sizeof(waterway_lengthes) - 1);
                const unsigned char max_length = waterway_lengthes[index];

                for(unsigned i = 0; i < 6; ++i)
                {
                    if(road_points[i] != t)
                        continue;
                    // test on maximal water way length
                    if(rb.mode != RM_BOAT || rb.route.size() < max_length || max_length == 0 )
                    {
                        if( ( (gwv->RoadAvailable(rb.mode == RM_BOAT, t)
                                && gwv->GetNode(t).owner - 1 == (signed)GAMECLIENT.GetPlayerID())
                                || (gwv->GetNode(t).bq == BQ_FLAG) )
                                && gwv->IsPlayerTerritory(t) )
                        {
                            unsigned short id = 60;
                            switch(int(gwv->GetNode(t).altitude) - altitude)
                            {
                                case 1: id = 61; break;
                                case 2: case 3: id = 62; break;
                                case 4: case 5: id = 63; break;
                                case -1: id = 64; break;
                                case -2: case -3: id = 65; break;
                                case -4: case -5: id = 66; break;
                            }

                            LOADER.GetMapImageN(id)->Draw(curPos.x, curPos.y, 0, 0, 0, 0, 0, 0);
                        }

                        if(gwv->GetNO(t))
                        {
                            // Flaggenanschluss? --> extra zeichnen
                            if(gwv->GetNO(t)->GetType() == NOP_FLAG && t != rb.start)
                                LOADER.GetMapImageN(20)->Draw(curPos.x, curPos.y, 0, 0, 0, 0, 0, 0);
                        }
                    }

                    if(!rb.route.empty())
                    {
                        if(unsigned(rb.route.back() + 3) % 6 == i)
                            LOADER.GetMapImageN(67)->Draw(curPos.x, curPos.y, 0, 0, 0, 0, 0, 0);
                    }
                }
            }
        }
    }

    // Umherfliegende Katapultsteine zeichnen
    for(std::list<CatapultStone*>::iterator it = gwv->catapult_stones.begin(); it != gwv->catapult_stones.end(); ++it)
        (*it)->Draw(*this, offset.x, offset.y);

    glTranslatef((GLfloat) - pos.x, (GLfloat) - pos.y, 0.0f);

    glScissor(0, 0, VIDEODRIVER.GetScreenWidth(), VIDEODRIVER.GetScreenWidth());

    SOUNDMANAGER.PlayBirdSounds(noTree::QueryDrawCounter());
}

void GameWorldView::DrawBoundaryStone(const int  /*x*/, const int  /*y*/, const MapPoint t, const Point<int> curPos, Visibility vis)
{
    if(vis == VIS_INVISIBLE)
        // schwarz/unsichtbar, nichts zeichnen
        return;

    bool fow = vis != VIS_VISIBLE;

    const MapNode::BoundaryStones& boundary_stones = fow ? gwv->GetNode(t).fow[gwv->GetYoungestFOWNodePlayer(MapPoint(t))].boundary_stones : gwv->GetNode(t).boundary_stones;
    unsigned char owner = boundary_stones[0];

    if(owner)
    {
        unsigned nation = gwv->GetPlayer(owner - 1).nation;
        unsigned player_color = COLORS[gwv->GetPlayer(owner - 1).color];

        LOADER.boundary_stone_cache[nation].draw(curPos.x, curPos.y, fow ? FOW_DRAW_COLOR : COLOR_WHITE, fow ? CalcPlayerFOWDrawColor(player_color) : player_color);

        for(unsigned i = 0; i < 3; ++i)
        {
            if(boundary_stones[i + 1])
            {
                Point<int> tmp = curPos - Point<int>( (gwv->GetNodePos(t) - gwv->GetTerrainRenderer()->GetTerrainAround(t, 3 + i)) / 2.0f );

                LOADER.boundary_stone_cache[nation].draw(
                    tmp.x, tmp.y,
                    fow ? FOW_DRAW_COLOR : COLOR_WHITE,
                    fow ? CalcPlayerFOWDrawColor(player_color) : player_color);

                /* Why twice?
                LOADER.boundary_stone_cache[nation].draw(
                    tmp.x, tmp.y,
                    fow ? FOW_DRAW_COLOR : COLOR_WHITE,
                    fow ? CalcPlayerFOWDrawColor(player_color) : player_color);*/
            }
        }
    }
}

/// Schaltet Produktivitäten/Namen komplett aus oder an
void GameWorldView::ShowNamesAndProductivity()
{
    if(show_productivity && show_names)
        show_productivity = show_names = false;
    else
        show_productivity = show_names = true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  verschiebt das Bild zu einer bestimmten Stelle.
 *
 *  @author FloSoft
 */
void GameWorldView::MoveTo(int x, int y, bool absolute)
{
    if(absolute)
    {
        offset = Point<int>(x, y);
    }
    else
    {
    	offset.x += x;
    	offset.y += y;
    }

    CalcFxLx();
}

void GameWorldView::MoveToMapObject(const MapPoint pt)
{
    lastOffset = offset;
    Point<int> nodePos = static_cast<Point<int> >(gwv->GetNodePos(pt));

    MoveTo(nodePos.x - width / 2, nodePos.y - height / 2, true);
}

/// Springt zur letzten Position, bevor man "weggesprungen" ist
void GameWorldView::MoveToLastPosition()
{
    Point<int> newLastOffset = offset;

    MoveTo(lastOffset.x, lastOffset.y, true);

    lastOffset = newLastOffset;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void GameWorldView::CalcFxLx()
{
    if(offset.x < 0)
    	offset.x = gwv->GetWidth() * TR_W + offset.x;
    if(offset.y < 0)
    	offset.y = gwv->GetHeight() * TR_H + offset.y;
    if(offset.x > gwv->GetWidth() * TR_W + width)
    	offset.x -= (gwv->GetWidth() * TR_W);
    if(offset.y > gwv->GetHeight() * TR_H + height)
    	offset.y -= (gwv->GetHeight() * TR_H);

    firstPt.x = offset.x / TR_W - 1;
    firstPt.y = (offset.y - 0x20 * HEIGHT_FACTOR) / TR_H;
    lastPt.x = (offset.x + width) / TR_W + 2;
    lastPt.y = (offset.y + height + 0x40 * HEIGHT_FACTOR) / TR_H;
}

void GameWorldView::Resize(unsigned short width, unsigned short height)
{
    this->width  = width;
    this->height = height;
    CalcFxLx();
}
