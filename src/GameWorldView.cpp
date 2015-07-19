// $Id: GameWorldView.cpp 7359 2011-08-10 10:21:18Z FloSoft $
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

#include "defines.h"
#include "GameWorld.h"
#include "drivers/VideoDriverWrapper.h"
#include "ogl/glArchivItem_Map.h"
#include "nodeObjs/noTree.h"
#include "buildings/nobUsual.h"
#include "buildings/nobMilitary.h"
#include "buildings/noBuildingSite.h"
#include "CatapultStone.h"
#include "GameClient.h"
#include "SoundManager.h"
#include "MapGeometry.h"
#include "gameData/MapConsts.h"
#include "desktops/dskGameInterface.h"
#include "FOWObjects.h"
#include "nodeObjs/noShip.h"

#include "Settings.h"

#include "GameServer.h"
#include "ai/AIPlayerJH.h"

#include "ogl/glSmartBitmap.h"

GameWorldView::GameWorldView(GameWorldViewer* gwv, unsigned short x, unsigned short y, unsigned short width, unsigned short height) : selx(0), sely(0), show_coordinates(false), show_bq(false), show_names(false), show_productivity(false), xoffset(0), yoffset(0), last_xoffset(0), last_yoffset(0), gwv(gwv), d_what(0), d_player(0), d_active(false), x(x), y(y), width(width), height(height), terrain_list(0), terrain_last_xoffset(0), terrain_last_yoffset(0), terrain_last_global_animation(0), terrain_last_water(0)
{
    CalcFxLx();
}

GameWorldView::~GameWorldView()
{
    if (terrain_list != 0)
        glDeleteLists(terrain_list, 1);
}

struct ObjectBetweenLines
{
    noBase* obj;
    Point<int> pos; // Zeichenposition

    ObjectBetweenLines(noBase* const obj, const Point<int> pos) : obj(obj), pos(pos) {}
};

void GameWorldView::Draw(const unsigned char player, unsigned* water, const bool draw_selected, const MapCoord selected_x, const MapCoord selected_y, const RoadsBuilding& rb)
{

    int shortest_len = 100000;
    //if(//GUIResources::inst().action.IsActive())
    //  shortest_len = 0;

    glScissor(x, VideoDriverWrapper::inst().GetScreenHeight() - y - height, width, height);

    gwv->GetTerrainRenderer()->Draw(this, water);

    glTranslatef((GLfloat) this->x, (GLfloat) this->y, 0.0f);

    // Draw-Counter der Bäume zurücksetzen vor jedem Zeichnen
    noTree::ResetDrawCounter();

    for(int y = fy; y < ly; ++y)
    {
        // Figuren speichern, die in dieser Zeile gemalt werden müssen
        // und sich zwischen zwei Zeilen befinden, da sie dazwischen laufen
        std::vector<ObjectBetweenLines> between_lines;

        for(int x = fx; x < lx; ++x)
        {
            unsigned short tx, ty;
            int xo, yo;
            gwv->GetTerrainRenderer()->ConvertCoords(x, y, tx, ty, &xo, &yo);


            int xpos = int(gwv->GetTerrainRenderer()->GetTerrainX(tx, ty)) - xoffset + xo;
            int ypos = int(gwv->GetTerrainRenderer()->GetTerrainY(tx, ty)) - yoffset + yo;

            if(abs(VideoDriverWrapper::inst().GetMouseX() - static_cast<int>(xpos)) + abs(VideoDriverWrapper::inst().GetMouseY() - static_cast<int>(ypos)) < shortest_len)
            {
                selx = tx;
                sely = ty;
                selxo = xo;
                selyo = yo;
                shortest_len = abs(VideoDriverWrapper::inst().GetMouseX() - static_cast<int>(xpos)) + abs(VideoDriverWrapper::inst().GetMouseY() - static_cast<int>(ypos));
            }

            Visibility visibility = gwv->GetVisibility(tx, ty);

            DrawBoundaryStone(x, y, tx, ty, xpos, ypos, visibility);

            /// Nur bei Sichtbaren Stellen zeichnen
            // Visible terrain
            if(visibility == VIS_VISIBLE)
            {
                ////////////////////////////////////////////////

                ///////////////////////////////////////
                // Draw objects and people

                // Draw objects - buildings, trees, stones, decoration sprites, etc.
                MapNode& mn = gwv->GetNode(tx, ty);
                if(mn.obj)
                {
                    mn.obj->Draw(static_cast<int>(xpos), static_cast<int>(ypos));
                    if (false) //TODO: military aid - display icon overlay of attack possibility
                    {
                        noBuilding* building = gwv->GetSpecObj<noBuilding>(tx, ty);
                        if (mn.owner != GAMECLIENT.GetPlayerID() + 1 //not belonging to current player
                                && gwv->GetNO(tx, ty)->GetType() == NOP_BUILDING //is a building
                                && !GameClient::inst().GetLocalPlayer()->IsAlly(building->GetPlayer())) //not an ally
                        {
                            BuildingType bt = building->GetBuildingType();
                            if ((bt >= BLD_BARRACKS && bt <= BLD_FORTRESS)
                                    || bt == BLD_HEADQUARTERS
                                    || bt == BLD_HARBORBUILDING) //is it a military building?
                            {
                                if(gwv->GetAvailableSoldiersForAttack(GAMECLIENT.GetPlayerID(), tx, ty)) //soldiers available for attack?
                                    LOADER.GetImageN("map_new", 20000)->Draw(static_cast<int>(xpos) + 1, static_cast<int>(ypos) - 5, 0, 0, 0, 0, 0, 0);;
                            }
                        }
                    }
                }


                // People
                if(mn.figures.size())
                {
                    for(list<noBase*>::iterator it = mn.figures.begin(); it.valid(); ++it)
                    {
                        // Bewegt er sich oder ist es ein Schiff?
                        if((*it)->IsMoving() || (*it)->GetGOT() == GOT_SHIP)
                            // Dann nach der gesamten Zeile zeichnen
                            between_lines.push_back(ObjectBetweenLines(*it, Point<int>(static_cast<int>(xpos), static_cast<int>(ypos))));
                        else
                            // Ansonsten jetzt schon zeichnen
                            (*it)->Draw(static_cast<int>(xpos), static_cast<int>(ypos));
                    }
                }

                ////////////////////////////////////////////////

                //Construction aid mode
                if(show_bq && gwv->GetNode(tx, ty).bq && gwv->GetNode(tx, ty).bq < 7)
                {
                    BuildingQuality bq = gwv->GetNode(tx, ty).bq;
                    glArchivItem_Bitmap* bm = LOADER.GetMapImageN(49 + bq);
                    //Draw building quality icon
                    bm->Draw(static_cast<int>(xpos), static_cast<int>(ypos), 0, 0, 0, 0, 0, 0);
                    //Show ability to construct military buildings
                    if(GameClient::inst().GetGGS().isEnabled(ADDON_MILITARY_AID))
                    {
                        if(!gwv->IsMilitaryBuildingNearNode(tx, ty, GAMECLIENT.GetPlayerID()) && (bq == BQ_HUT || bq == BQ_HOUSE || bq == BQ_CASTLE || bq == BQ_HARBOR))
                            LOADER.GetImageN("map_new", 20000)->Draw(static_cast<int>(xpos) + (1), static_cast<int>(ypos) - bm->getHeight() - 5, 0, 0, 0, 0, 0, 0);
                    }
                }
            }
            // im Nebel die FOW-Objekte zeichnen
            // Fog of war
            else if(visibility == VIS_FOW)
            {
                const FOWObject* fowobj = gwv->GetYoungestFOWObject(Point<MapCoord>(tx, ty));
                if(fowobj)
                    fowobj->Draw(static_cast<int>(xpos), static_cast<int>(ypos));
            }


            if(show_coordinates)
            {
                char high[32];
                //*sprintf(high,"%d",unsigned(GetNode(tx,ty).altitude));*/
                sprintf(high, "%d;%d", tx, ty);
                //*sprintf(high,"%X",unsigned(GetNode(tx,ty))).resources;*/
                //sprintf(high,"%u",GetNode(tx,ty)).reserved;
                NormalFont->Draw(static_cast<int>(xpos), static_cast<int>(ypos), high, 0, 0xFFFFFF00);

                if(gwv->GetNode(tx, ty).reserved)
                {
                    NormalFont->Draw(static_cast<int>(xpos), static_cast<int>(ypos), "R", 0, 0xFFFF0000);
                }

            }

            if (d_active)
            {
                std::stringstream ss;
                AIPlayerJH* ai = dynamic_cast<AIPlayerJH*>(GameServer::inst().GetAIPlayer(d_player));
                if (ai)
                {
                    if (d_what == 1)
                    {
                        if(ai->GetAINode(tx, ty).bq && ai->GetAINode(tx, ty).bq  < 7)
                            LOADER.GetMapImageN(49 + ai->GetAINode(tx, ty).bq)->Draw(static_cast<int>(xpos), static_cast<int>(ypos), 0, 0, 0, 0, 0, 0);
                    }
                    else if (d_what == 2)
                    {
                        if (ai->GetAINode(tx, ty).reachable)
                            LOADER.GetImageN("io", 32)->Draw(static_cast<int>(xpos), static_cast<int>(ypos), 0, 0, 0, 0, 0, 0);
                        else
                            LOADER.GetImageN("io", 40)->Draw(static_cast<int>(xpos), static_cast<int>(ypos), 0, 0, 0, 0, 0, 0);
                    }
                    else if (d_what == 3)
                    {
                        if (ai->GetAINode(tx, ty).farmed)
                            LOADER.GetImageN("io", 32)->Draw(static_cast<int>(xpos), static_cast<int>(ypos), 0, 0, 0, 0, 0, 0);
                        else
                            LOADER.GetImageN("io", 40)->Draw(static_cast<int>(xpos), static_cast<int>(ypos), 0, 0, 0, 0, 0, 0);
                    }
                    else if (d_what > 3 && d_what < 13)
                    {
                        ss << ai->GetResMapValue(tx, ty, AIJH::Resource(d_what - 4));
                        NormalFont->Draw(static_cast<int>(xpos), static_cast<int>(ypos), ss.str().c_str(), 0, 0xFFFFFF00);
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
        for(int x = fx; x < lx; ++x)
        {
            for(int y = fy; y < ly; ++y)
            {
                // Coordinate transform
                unsigned short tx, ty;
                int xo, yo;
                gwv->GetTerrainRenderer()->ConvertCoords(x, y, tx, ty, &xo, &yo);

                int xpos = (int)(gwv->GetTerrainRenderer()->GetTerrainX(tx, ty) - xoffset + xo);
                int ypos = (int)(gwv->GetTerrainRenderer()->GetTerrainY(tx, ty) - yoffset + yo);

                // Name bzw Produktivität anzeigen
                GO_Type got = gwv->GetNO(tx, ty)->GetGOT();
                if(IsBaseBuilding(got))
                {
                    noBaseBuilding* no = gwv->GetSpecObj<noBaseBuilding>(tx, ty);

                    // Is object not belonging to local player?
                    if(no->GetPlayer() != GAMECLIENT.GetPlayerID())
                        continue;

                    int py = ypos - 10;

                    // Draw object name
                    if(show_names)
                    {
                        unsigned int color = (no->GetGOT() == GOT_BUILDINGSITE) ? COLOR_GREY : COLOR_YELLOW;
                        SmallFont->Draw(xpos, py, _(BUILDING_NAMES[no->GetBuildingType()]), glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER, color);
                        py += SmallFont->getHeight();
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
                                    SmallFont->Draw(xpos, py, text, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER, color);
                                }
                            }
                            break;

                            case GOT_NOB_USUAL: // Is it a normal building or shipyard?
                            case GOT_NOB_SHIPYARD:
                            {
                                nobUsual* n = dynamic_cast<nobUsual*>(no);
                                if(n)
                                {
                                    char text[256];
                                    unsigned int color = COLOR_RED;

                                    if(!n->HasWorker())
                                        snprintf(text, 256, "%s", _("(House unoccupied)"));
                                    else if(n->IsProductionDisabledVirtual())
                                        snprintf(text, 256, "%s", _("(stopped)"));
                                    else
                                    {
                                        // Catapult and Lookout tower doesn't have productivity!
                                        if(n->GetBuildingType() == BLD_CATAPULT || n->GetBuildingType() == BLD_LOOKOUTTOWER)
                                            text[0] = 0;
                                        else
                                        {
                                            unsigned short p = *n->GetProduktivityPointer();
                                            snprintf(text, 256, "(%d %%)", p);
                                            if(p >= 60)
                                                color = 0xFF00E000;
                                            else if(p >= 30)
                                                color = 0xFFFFFF00;
                                            else if(p >= 20)
                                                color = 0xFFFF8000;
                                        }
                                    }
                                    SmallFont->Draw(xpos, py, text, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER, color);
                                }
                            }
                            break;

                            case GOT_NOB_MILITARY: // Is it a military building?
                            {
                                // Display amount of soldiers
                                unsigned soldiers_count = static_cast<nobMilitary*>(no)->GetTroopsCount();
                                char str[64];
                                if(soldiers_count == 1)
                                    strcpy(str, _("(1 soldier)"));
                                else
                                    sprintf(str, _("(%d soldiers)"), soldiers_count);


                                SmallFont->Draw(xpos, py, str, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER,
                                                (soldiers_count > 0) ? COLOR_YELLOW : COLOR_RED);
                                py += SmallFont->getHeight();
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

    // Falls im StraÃenbaumodus: Punkte um den aktuellen StraÃenbaupunkt herum ermitteln
    unsigned short road_points[6 * 2];

    if(rb.mode)
    {
        for(unsigned i = 0; i < 6; ++i)
        {
            road_points[i * 2] = gwv->GetXA(rb.point_x, rb.point_y, i);
            road_points[i * 2 + 1] = gwv->GetYA(rb.point_x, rb.point_y, i);
        }
    }

    for(int x = fx; x < lx; ++x)
    {
        for(int y = fy; y < ly; ++y)
        {
            // Coordinates transform
            unsigned short tx, ty;
            int xo, yo;
            gwv->GetTerrainRenderer()->ConvertCoords(x, y, tx, ty, &xo, &yo);

            int xpos = (int)(gwv->GetTerrainRenderer()->GetTerrainX(tx, ty) - xoffset + xo);
            int ypos = (int)(gwv->GetTerrainRenderer()->GetTerrainY(tx, ty) - yoffset + yo);

            /// Current point indicated by Mouse
            if(selx == tx && sely == ty)
            {
                // Mauszeiger am boten
                unsigned mid = 22;
                switch(gwv->GetNode(tx, ty).bq)
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

                LOADER.GetMapImageN(mid)->Draw(xpos, ypos, 0, 0, 0, 0, 0, 0);
            }

            // Currently selected point
            if(draw_selected && selected_x == tx && selected_y == ty)
                LOADER.GetMapImageN(20)->Draw(xpos, ypos, 0, 0, 0, 0, 0, 0);

            // Wegbauzeug
            if(rb.mode)
            {
                if(rb.point_x == tx && rb.point_y == ty)
                    LOADER.GetMapImageN(21)->Draw(xpos, ypos, 0, 0, 0, 0, 0, 0);



                int altitude = gwv->GetNode(rb.point_x, rb.point_y).altitude;

                const unsigned char waterway_lengthes[] = {3, 5, 9, 13, 21, 0}; // these are written into dskGameInterface.cpp, too
                const unsigned char index = GameClient::inst().GetGGS().getSelection(ADDON_MAX_WATERWAY_LENGTH);
                assert(index <= sizeof(waterway_lengthes) - 1);
                const unsigned char max_length = waterway_lengthes[index];

                for(unsigned i = 0; i < 6; ++i)
                {
                    if(road_points[i * 2] == tx  && road_points[i * 2 + 1] == ty)
                    {
                        // test on maximal water way length
                        if(rb.mode != RM_BOAT || rb.route.size() < max_length || max_length == 0 )
                        {
                            if( ( (gwv->RoadAvailable(rb.mode == RM_BOAT, tx, ty, i)
                                    && gwv->GetNode(tx, ty).owner - 1 == (signed)GAMECLIENT.GetPlayerID())
                                    || (gwv->GetNode(tx, ty).bq == BQ_FLAG) )
                                    && gwv->IsPlayerTerritory(tx, ty) )
                            {
                                unsigned short id = 60;
                                switch(int(gwv->GetNode(tx, ty).altitude) - altitude)
                                {
                                    case 1: id = 61; break;
                                    case 2: case 3: id = 62; break;
                                    case 4: case 5: id = 63; break;
                                    case -1: id = 64; break;
                                    case -2: case -3: id = 65; break;
                                    case -4: case -5: id = 66; break;
                                }

                                LOADER.GetMapImageN(id)->Draw(xpos, ypos, 0, 0, 0, 0, 0, 0);
                            }

                            if(gwv->GetNO(tx, ty))
                            {
                                // Flaggenanschluss? --> extra zeichnen
                                if(gwv->GetNO(tx, ty)->GetType() == NOP_FLAG && !(tx == rb.start_x && ty == rb.start_y))
                                    LOADER.GetMapImageN(20)->Draw(xpos, ypos, 0, 0, 0, 0, 0, 0);
                            }
                        }

                        if(rb.route.size())
                        {
                            if(unsigned(rb.route.back() + 3) % 6 == i)
                                LOADER.GetMapImageN(67)->Draw(xpos, ypos, 0, 0, 0, 0, 0, 0);
                        }
                    }
                }
            }
        }
    }

    // Umherfliegende Katapultsteine zeichnen
    for(std::list<CatapultStone*>::iterator it = gwv->catapult_stones.begin(); it != gwv->catapult_stones.end(); ++it)
        (*it)->Draw(*this, xoffset, yoffset);

    glTranslatef((GLfloat) - this->x, (GLfloat) - this->y, 0.0f);

    glScissor(0, 0, VideoDriverWrapper::inst().GetScreenWidth(), VideoDriverWrapper::inst().GetScreenWidth());

    SoundManager::inst().PlayBirdSounds(noTree::QueryDrawCounter());
}

void GameWorldView::DrawBoundaryStone(const int x, const int y, const MapCoord tx, const MapCoord ty, const int xpos, const int ypos, Visibility vis)
{
    if(vis == VIS_INVISIBLE)
        // schwarz/unsichtbar, nichts zeichnen
        return;

    bool fow = !(vis == VIS_VISIBLE);

    unsigned char* boundary_stones = fow ? gwv->GetNode(tx, ty).fow[gwv->GetYoungestFOWNodePlayer(Point<MapCoord>(tx, ty))].boundary_stones : gwv->GetNode(tx, ty).boundary_stones;
    unsigned char owner = boundary_stones[0];

    if(owner)
    {
        unsigned nation = gwv->GetPlayer(owner - 1)->nation;
        unsigned player_color = COLORS[gwv->GetPlayer(owner - 1)->color];

        Loader::boundary_stone_cache[nation].draw(xpos, ypos, fow ? FOW_DRAW_COLOR : COLOR_WHITE, fow ? CalcPlayerFOWDrawColor(player_color) : player_color);

        for(unsigned i = 0; i < 3; ++i)
        {
            if(boundary_stones[i + 1])
            {
                Loader::boundary_stone_cache[nation].draw(
                    xpos - static_cast<int>((gwv->GetTerrainRenderer()->GetTerrainX(tx, ty) - gwv->GetTerrainRenderer()->GetTerrainXAround(tx, ty, 3 + i)) / 2.0f),
                    ypos - static_cast<int>((gwv->GetTerrainRenderer()->GetTerrainY(tx, ty) - gwv->GetTerrainRenderer()->GetTerrainYAround(tx, ty, 3 + i)) / 2.0f),
                    fow ? FOW_DRAW_COLOR : COLOR_WHITE,
                    fow ? CalcPlayerFOWDrawColor(player_color) : player_color);

                Loader::boundary_stone_cache[nation].draw(
                    xpos - static_cast<int>((gwv->GetTerrainRenderer()->GetTerrainX(tx, ty) - gwv->GetTerrainRenderer()->GetTerrainXAround(tx, ty, 3 + i)) / 2.0f),
                    ypos - static_cast<int>((gwv->GetTerrainRenderer()->GetTerrainY(tx, ty) - gwv->GetTerrainRenderer()->GetTerrainYAround(tx, ty, 3 + i)) / 2.0f),
                    fow ? FOW_DRAW_COLOR : COLOR_WHITE,
                    fow ? CalcPlayerFOWDrawColor(player_color) : player_color);
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
        xoffset = x;
        yoffset = y;
    }
    else
    {
        xoffset += x;
        yoffset += y;
    }

    CalcFxLx();
}

void GameWorldView::MoveToMapObject(const MapCoord x, const MapCoord y)
{
    last_xoffset = xoffset;
    last_yoffset = yoffset;

    MoveTo(static_cast<int>(gwv->GetTerrainX(x, y))
           - width  / 2, static_cast<int>(gwv->GetTerrainY(x, y))
           - height / 2, true);
}

/// Springt zur letzten Position, bevor man "weggesprungen" ist
void GameWorldView::MoveToLastPosition()
{
    int new_last_xoffset = xoffset;
    int new_last_yoffset = yoffset;

    MoveTo(last_xoffset, last_yoffset, true);

    last_xoffset = new_last_xoffset;
    last_yoffset = new_last_yoffset;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void GameWorldView::CalcFxLx()
{
    if(xoffset < 0)
        xoffset = gwv->GetWidth() * TR_W + xoffset;
    if(yoffset < 0)
        yoffset = gwv->GetHeight() * TR_H + yoffset;
    if(xoffset > gwv->GetWidth() * TR_W + width)
        xoffset -= (gwv->GetWidth() * TR_W);
    if(yoffset > gwv->GetHeight() * TR_H + height)
        yoffset -= (gwv->GetHeight() * TR_H);

    fx = xoffset / TR_W - 1;
    fy = (yoffset - 0x20 * HEIGHT_FACTOR) / TR_H;
    lx = (xoffset + width) / TR_W + 2;
    ly = (yoffset + height + 0x40 * HEIGHT_FACTOR) / TR_H;
}

void GameWorldView::Resize(unsigned short width, unsigned short height)
{
    this->width  = width;
    this->height = height;
    CalcFxLx();
}
