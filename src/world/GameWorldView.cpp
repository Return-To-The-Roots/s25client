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
#include "buildings/nobUsual.h"
#include "buildings/nobMilitary.h"
#include "buildings/noBuildingSite.h"
#include "CatapultStone.h"
#include "GameClient.h"
#include "SoundManager.h"
#include "Loader.h"
#include "gameData/MapConsts.h"
#include "FOWObjects.h"
#include "GameServer.h"
#include "ai/AIPlayerJH.h"
#include "ogl/glArchivItem_Font.h"
#include "ogl/glSmartBitmap.h"
#include "addons/AddonMaxWaterwayLength.h"
#include "gameTypes/RoadBuildState.h"
#include "helpers/converters.h"
#include <boost/format.hpp>
#include <stdexcept>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

GameWorldView::GameWorldView(GameWorldViewer& gwv, const Point<int>& pos, unsigned width, unsigned height):
	selPt(0, 0),
	debugNodePrinter(NULL),
	show_bq(false),
	show_names(false),
	show_productivity(false),
	offset(0, 0),
	lastOffset(0, 0),
	gwv(gwv),
	d_what(0),
	d_player(0),
	d_active(false),
	pos(pos),
	width(width), height(height),
    zoomFactor(1.f)
{
    MoveTo(0, 0);
}

GameWorldView::~GameWorldView()
{
}

void GameWorldView::SetZoomFactor(float zoomFactor)
{
    this->zoomFactor = zoomFactor;
    CalcFxLx();
}

struct ObjectBetweenLines
{
    noBase* obj;
    Point<int> pos; // Zeichenposition

    ObjectBetweenLines(noBase* obj, const Point<int>& pos) : obj(obj), pos(pos) {}
};

void GameWorldView::Draw(const RoadBuildState& rb, const bool draw_selected, const MapPoint selected, unsigned* water)
{
    int shortestDistToMouse = 100000;
    Point<int> mousePos(VIDEODRIVER.GetMouseX(), VIDEODRIVER.GetMouseY());
    mousePos -= Point<int>(pos);

    glScissor(pos.x, VIDEODRIVER.GetScreenHeight() - pos.y - height, width, height);
    if(zoomFactor != 1.f)
    {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glScalef(zoomFactor, zoomFactor, 1);
        // Offset to center view
        Point<float> diff(width - width / zoomFactor, height - height / zoomFactor);
        diff = diff / 2.f;
        glTranslatef(-diff.x, -diff.y, 0.f);
        // Also adjust mouse
        mousePos = Point<int>(Point<float>(mousePos) / zoomFactor + diff);
        glMatrixMode(GL_MODELVIEW);
    }

    glTranslatef(static_cast<GLfloat>(pos.x) / zoomFactor, static_cast<GLfloat>(pos.y) / zoomFactor, 0.0f);

    glTranslatef(static_cast<GLfloat>(-offset.x), static_cast<GLfloat>(-offset.y), 0.0f);
    TerrainRenderer& terrainRenderer = gwv.GetTerrainRenderer();
    terrainRenderer.Draw(*this, water);
    glTranslatef(static_cast<GLfloat>(offset.x), static_cast<GLfloat>(offset.y), 0.0f);

    for(int y = firstPt.y; y <= lastPt.y; ++y)
    {
        // Figuren speichern, die in dieser Zeile gemalt werden müssen
        // und sich zwischen zwei Zeilen befinden, da sie dazwischen laufen
        std::vector<ObjectBetweenLines> between_lines;

        for(int x = firstPt.x; x <= lastPt.x; ++x)
        {
            Point<int> curOffset;
            const MapPoint curPt = terrainRenderer.ConvertCoords(Point<int>(x, y), &curOffset);
            Point<int> curPos = Point<int>(gwv.GetNodePos(curPt)) - offset + curOffset;

            const Point<int> mouseDist = mousePos - curPos;
            if(std::abs(mouseDist.x) + std::abs(mouseDist.y) < shortestDistToMouse)
            {
                selPt = curPt;
                selPtOffset = curOffset;
                shortestDistToMouse = std::abs(mouseDist.x) + std::abs(mouseDist.y);
            }

            Visibility visibility = gwv.GetVisibility(curPt);

            DrawBoundaryStone(curPt, curPos, visibility);

            if(visibility == VIS_VISIBLE)
            {
                DrawObject(curPt, curPos);

                DrawFigures(curPt, curPos, between_lines);

                //Construction aid mode
                if(show_bq)
                    DrawConstructionAid(curPt, curPos);
            } else if(visibility == VIS_FOW)
            {
                const FOWObject* fowobj = gwv.GetYoungestFOWObject(MapPoint(curPt));
                if(fowobj)
                    fowobj->Draw(curPos.x, curPos.y);
            }

            if(debugNodePrinter)
                debugNodePrinter->print(curPt, curPos);

            if (d_active)
                DrawAIDebug(curPt, curPos);
        }

        // Figuren zwischen den Zeilen zeichnen
        for(unsigned i = 0; i < between_lines.size(); ++i)
            between_lines[i].obj->Draw(between_lines[i].pos.x, between_lines[i].pos.y);
    }

    if(show_names || show_productivity)
        DrawNameProductivityOverlay(terrainRenderer);

    DrawGUI(rb, terrainRenderer, draw_selected, selected);

    // Umherfliegende Katapultsteine zeichnen
    for(std::list<CatapultStone*>::iterator it = gwv.catapult_stones.begin(); it != gwv.catapult_stones.end(); ++it)
        (*it)->Draw(*this, offset.x, offset.y);

    if(zoomFactor != 1.f)
    {
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    }
    glTranslatef(-static_cast<GLfloat>(pos.x) / zoomFactor, -static_cast<GLfloat>(pos.y) / zoomFactor, 0.0f);

    glScissor(0, 0, VIDEODRIVER.GetScreenWidth(), VIDEODRIVER.GetScreenWidth());
}

void GameWorldView::DrawGUI(const RoadBuildState& rb, const TerrainRenderer& terrainRenderer, const bool draw_selected, const MapPoint& selectedPt)
{
    // Falls im Straßenbaumodus: Punkte um den aktuellen Straßenbaupunkt herum ermitteln
    MapPoint road_points[6];

    unsigned maxWaterWayLen = 0;
    if(rb.mode != RM_DISABLED)
    {
        for(unsigned i = 0; i < 6; ++i)
            road_points[i] = gwv.GetNeighbour(rb.point, i);

        const unsigned index = GAMECLIENT.GetGGS().getSelection(AddonId::MAX_WATERWAY_LENGTH);
        RTTR_Assert(index < waterwayLengths.size());
        maxWaterWayLen = waterwayLengths[index];
    }

    for(int x = firstPt.x; x <= lastPt.x; ++x)
    {
        for(int y = firstPt.y; y <= lastPt.y; ++y)
        {
            // Coordinates transform
            Point<int> curOffset;
            MapPoint curPt = terrainRenderer.ConvertCoords(Point<int>(x, y), &curOffset);
            Point<int> curPos = Point<int>(gwv.GetNodePos(curPt)) - offset + curOffset;

            /// Current point indicated by Mouse
            if(selPt == curPt)
            {
                // Mauszeiger am boten
                unsigned mid = 22;
                if(rb.mode == RM_DISABLED){
                    switch(gwv.GetNode(curPt).bq)
                    {
                    case BQ_FLAG: mid = 40; break;
                    case BQ_MINE: mid = 41; break;
                    case BQ_HUT: mid = 42; break;
                    case BQ_HOUSE: mid = 43; break;
                    case BQ_CASTLE: mid = 44; break;
                    case BQ_HARBOR: mid = 45; break;
                    default: break;
                    }
                }

                LOADER.GetMapImageN(mid)->Draw(curPos.x, curPos.y);
            }

            // Currently selected point
            if(draw_selected && selectedPt == curPt)
                LOADER.GetMapImageN(20)->Draw(curPos.x, curPos.y);

            // Wegbauzeug
            if(rb.mode == RM_DISABLED)
                continue;

            if(rb.point == curPt)
                LOADER.GetMapImageN(21)->Draw(curPos.x, curPos.y);

            int altitude = gwv.GetNode(rb.point).altitude;

            for(unsigned i = 0; i < 6; ++i)
            {
                if(road_points[i] != curPt)
                    continue;

                // test on maximal water way length
                if(rb.mode == RM_BOAT && maxWaterWayLen != 0 && rb.route.size() >= maxWaterWayLen)
                    continue;

                if(((gwv.RoadAvailable(rb.mode == RM_BOAT, curPt)
                    && gwv.GetNode(curPt).owner - 1 == (signed)GAMECLIENT.GetPlayerID())
                    || (gwv.GetNode(curPt).bq == BQ_FLAG))
                    && gwv.IsPlayerTerritory(curPt))
                {
                    unsigned id;
                    switch(int(gwv.GetNode(curPt).altitude) - altitude)
                    {
                    case 1: id = 61; break;
                    case 2: case 3: id = 62; break;
                    case 4: case 5: id = 63; break;
                    case -1: id = 64; break;
                    case -2: case -3: id = 65; break;
                    case -4: case -5: id = 66; break;
                    default: id = 60; break;
                    }

                    LOADER.GetMapImageN(id)->Draw(curPos.x, curPos.y);
                }

                // Flaggenanschluss? --> extra zeichnen
                if(gwv.GetNO(curPt)->GetType() == NOP_FLAG && curPt != rb.start)
                    LOADER.GetMapImageN(20)->Draw(curPos.x, curPos.y);

                if(!rb.route.empty() && unsigned(rb.route.back() + 3) % 6 == i)
                    LOADER.GetMapImageN(67)->Draw(curPos.x, curPos.y);
            }
        }
    }
}

void GameWorldView::DrawNameProductivityOverlay(const TerrainRenderer& terrainRenderer)
{
    for(int x = firstPt.x; x <= lastPt.x; ++x)
    {
        for(int y = firstPt.y; y <= lastPt.y; ++y)
        {
            // Coordinate transform
            Point<int> curOffset;
            MapPoint pt = terrainRenderer.ConvertCoords(Point<int>(x, y), &curOffset);

            noBaseBuilding* no = gwv.GetSpecObj<noBaseBuilding>(pt);
            if(!no)
                continue;

            // Is object not belonging to local player?
            if(no->GetPlayer() != GAMECLIENT.GetPlayerID())
                continue;

            Point<int> curPos = Point<int>(gwv.GetNodePos(pt)) - offset + curOffset;
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
                DrawProductivity(*no, curPos);
        }
    }
}

void GameWorldView::DrawProductivity(const noBaseBuilding& no, const Point<int>& curPos)
{
    const GO_Type got = no.GetGOT();
    if(got == GOT_BUILDINGSITE)
    {
        char text[256];
        unsigned int color = COLOR_GREY;

        unsigned short p = static_cast<const noBuildingSite&>(no).GetBuildProgress();
        snprintf(text, 256, "(%d %%)", p);
        SmallFont->Draw(curPos.x, curPos.y, text, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER, color);
    }else if(got == GOT_NOB_USUAL || got == GOT_NOB_SHIPYARD)
    {
        const nobUsual& n = static_cast<const nobUsual&>(no);
        std::string text;
        unsigned int color = COLOR_RED;

        if(!n.HasWorker())
            text = _("(House unoccupied)");
        else if(n.IsProductionDisabledVirtual())
            text = _("(stopped)");
        else
        {
            // Catapult and Lookout tower doesn't have productivity!
            if(n.GetBuildingType() == BLD_CATAPULT || n.GetBuildingType() == BLD_LOOKOUTTOWER)
                return;

            unsigned short p = n.GetProductivity();
            text = helpers::toString(p) + " %";
            if(p >= 60)
                color = 0xFF00E000;
            else if(p >= 30)
                color = 0xFFFFFF00;
            else if(p >= 20)
                color = 0xFFFF8000;
        }
        SmallFont->Draw(curPos.x, curPos.y, text, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER, color);
    }else if(got == GOT_NOB_MILITARY)
    {
        // Display amount of soldiers
        unsigned soldiers_count = static_cast<const nobMilitary&>(no).GetTroopsCount();
        std::string sSoldiers;
        if(soldiers_count == 1)
            sSoldiers = _("(1 soldier)");
        else
            sSoldiers = boost::str(
                boost::format(_("(%d soldiers)")) % soldiers_count
                );


        SmallFont->Draw(curPos.x, curPos.y, sSoldiers, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER,
            (soldiers_count > 0) ? COLOR_YELLOW : COLOR_RED);
    }
}

void GameWorldView::DrawFigures(const MapPoint& pt, const Point<int>&curPos, std::vector<ObjectBetweenLines>& between_lines)
{
    const std::list<noBase*>& figures = gwv.GetNode(pt).figures;
    for(std::list<noBase*>::const_iterator it = figures.begin(); it != figures.end(); ++it)
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

void GameWorldView::DrawConstructionAid(const MapPoint& pt, const Point<int>& curPos)
{
    BuildingQuality bq = gwv.GetNode(pt).bq;
    if(bq != BQ_NOTHING && bq != BQ_DANGER) //-V807
    {
        glArchivItem_Bitmap* bm = LOADER.GetMapImageN(49 + bq);
        //Draw building quality icon
        bm->Draw(curPos.x, curPos.y);
        //Show ability to construct military buildings
        if(GAMECLIENT.GetGGS().isEnabled(AddonId::MILITARY_AID))
        {
            if(!gwv.IsMilitaryBuildingNearNode(pt, GAMECLIENT.GetPlayerID()) && (bq == BQ_HUT || bq == BQ_HOUSE || bq == BQ_CASTLE || bq == BQ_HARBOR))
                LOADER.GetImageN("map_new", 20000)->Draw(curPos.x + 1, curPos.y - bm->getHeight() - 5);
        }
    }
}

void GameWorldView::DrawObject(const MapPoint& pt, const Point<int>& curPos)
{
    noBase* obj = gwv.GetNode(pt).obj;
    if(!obj)
        return;

    obj->Draw(curPos.x, curPos.y);

    return;
    //TODO: military aid - display icon overlay of attack possibility

    noBuilding* building = dynamic_cast<noBuilding*>(obj);
    if(!building || GAMECLIENT.GetLocalPlayer().IsAlly(building->GetPlayer()))
        return;

    BuildingType bt = building->GetBuildingType();
    if((bt >= BLD_BARRACKS && bt <= BLD_FORTRESS)
        || bt == BLD_HEADQUARTERS
        || bt == BLD_HARBORBUILDING) //is it a military building?
    {
        if(gwv.GetAvailableSoldiersForAttack(GAMECLIENT.GetPlayerID(), building->GetPos())) //soldiers available for attack?
            LOADER.GetImageN("map_new", 20000)->Draw(curPos.x + 1, curPos.y - 5);
    }
}

void GameWorldView::DrawAIDebug(const MapPoint& pt, const Point<int>& curPos)
{
    AIPlayerJH* ai = dynamic_cast<AIPlayerJH*>(GAMESERVER.GetAIPlayer(d_player));
    if(!ai)
        return;

    if(d_what == 1)
    {
        if(ai->GetAINode(pt).bq && ai->GetAINode(pt).bq < 7) //-V807
            LOADER.GetMapImageN(49 + ai->GetAINode(pt).bq)->Draw(curPos.x, curPos.y);
    } else if(d_what == 2)
    {
        if(ai->GetAINode(pt).reachable)
            LOADER.GetImageN("io", 32)->Draw(curPos.x, curPos.y);
        else
            LOADER.GetImageN("io", 40)->Draw(curPos.x, curPos.y);
    } else if(d_what == 3)
    {
        if(ai->GetAINode(pt).farmed)
            LOADER.GetImageN("io", 32)->Draw(curPos.x, curPos.y);
        else
            LOADER.GetImageN("io", 40)->Draw(curPos.x, curPos.y);
    } else if(d_what > 3 && d_what < 13)
    {
        std::stringstream ss;
        ss << ai->GetResMapValue(pt, AIJH::Resource(d_what - 4));
        NormalFont->Draw(curPos.x, curPos.y, ss.str(), 0, 0xFFFFFF00);
    }
}

void GameWorldView::DrawBoundaryStone(const MapPoint& pt, const Point<int> pos, Visibility vis)
{
    if(vis == VIS_INVISIBLE)
        return;

    bool isFoW = vis == VIS_FOW;

    const MapNode::BoundaryStones& boundary_stones = isFoW ? gwv.GetNode(pt).fow[gwv.GetYoungestFOWNodePlayer(pt)].boundary_stones : gwv.GetNode(pt).boundary_stones;
    unsigned char owner = boundary_stones[0];

    if(!owner)
        return;

    unsigned nation = gwv.GetPlayer(owner - 1).nation;
    unsigned player_color = gwv.GetPlayer(owner - 1).color;
    if(isFoW)
        player_color = CalcPlayerFOWDrawColor(player_color);

    LOADER.boundary_stone_cache[nation].draw(pos.x, pos.y, isFoW ? FOW_DRAW_COLOR : COLOR_WHITE, player_color);

    for(unsigned i = 0; i < 3; ++i)
    {
        if(boundary_stones[i + 1])
        {
            Point<int> tmp = pos - Point<int>((gwv.GetNodePos(pt) - gwv.GetTerrainRenderer().GetTerrainAround(pt, 3 + i)) / 2.0f);

            LOADER.boundary_stone_cache[nation].draw(
                tmp.x, tmp.y,
                isFoW ? FOW_DRAW_COLOR : COLOR_WHITE,
                player_color);
        }
    }
}

/// Schaltet Produktivitäten/Namen komplett aus oder an
void GameWorldView::ToggleShowNamesAndProductivity()
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

    Point<int> size(gwv.GetWidth() * TR_W, gwv.GetHeight() * TR_H);
    if(size.x && size.y)
    {
        offset.x %= size.x;
        offset.y %= size.y;

        if(offset.x < 0)
            offset.x += size.x;
        if(offset.y < 0)
            offset.y += size.y;
    }

    CalcFxLx();
}

void GameWorldView::MoveToMapPt(const MapPoint pt)
{
    if(!pt.isValid())
        return;

    lastOffset = offset;
    Point<int> nodePos = static_cast<Point<int> >(gwv.GetNodePos(pt));

    MoveTo(nodePos.x - width / 2, nodePos.y - height / 2, true);
}

/// Springt zur letzten Position, bevor man "weggesprungen" ist
void GameWorldView::MoveToLastPosition()
{
    Point<int> newLastOffset = offset;

    MoveTo(lastOffset.x, lastOffset.y, true);

    lastOffset = newLastOffset;
}

void GameWorldView::CalcFxLx()
{
    // Calc first and last point in map units (with 1 extra for incomplete triangles)
    firstPt.x = offset.x / TR_W - 1;
    firstPt.y = (offset.y - 10 * HEIGHT_FACTOR) / TR_H - 1; // base altitude = 10
    lastPt.x = (offset.x + width) / TR_W + 1;
    lastPt.y = (offset.y + height + (60 - 10) * HEIGHT_FACTOR) / TR_H + 1; // max altitude = 60, base = 10

    if(zoomFactor != 1.f)
    {
        // Calc pixels we can remove from sides, as they are not drawn due to zoom
        Point<float> diff(width - width / zoomFactor, height - height / zoomFactor);
        // Stay centered by removing half the pixels from opposite sites
        diff = diff / 2.f;
        // Convert to map points
        diff.x /= TR_W;
        diff.y /= TR_H;
        // Don't remove to much
        diff.x = std::floor(diff.x);
        diff.y = std::floor(diff.y);
        firstPt = Point<int>(Point<float>(firstPt) + diff);
        lastPt = Point<int>(Point<float>(lastPt) - diff);
    }
}

void GameWorldView::Resize(unsigned width, unsigned height)
{
    this->width  = width;
    this->height = height;
    CalcFxLx();
}
