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
#include "world/GameWorldView.h"
#include "CatapultStone.h"
#include "FOWObjects.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "MapGeometry.h"
#include "SoundManager.h"
#include "addons/AddonMaxWaterwayLength.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobUsual.h"
#include "drivers/VideoDriverWrapper.h"
#include "helpers/containerUtils.h"
#include "helpers/strUtils.h"
#include "network/GameClient.h"
#include "ogl/FontStyle.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Font.h"
#include "ogl/glSmartBitmap.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldViewer.h"
#include "gameTypes/RoadBuildState.h"
#include "gameData/BuildingConsts.h"
#include "gameData/BuildingProperties.h"
#include "gameData/GuiConsts.h"
#include "gameData/MapConsts.h"
#include <boost/format.hpp>
#include <glad/glad.h>
#include <stdexcept>

GameWorldView::GameWorldView(const GameWorldViewer& gwv, const Position& pos, const Extent& size)
    : selPt(0, 0), show_bq(false), show_names(false), show_productivity(false), offset(0, 0), lastOffset(0, 0), gwv(gwv), origin_(pos),
      size_(size), zoomFactor_(1.f), targetZoomFactor_(1.f), zoomSpeed_(0.f)
{
    MoveTo(0, 0);
}

GameWorldView::~GameWorldView() {}

const GameWorldBase& GameWorldView::GetWorld() const
{
    return gwv.GetWorld();
}

void GameWorldView::SetNextZoomFactor()
{
    if(zoomFactor_ == targetZoomFactor_) // == with float is ok here, is explicitly set in last step //-V550
        return;

    float remainingZoomDiff = targetZoomFactor_ - zoomFactor_;

    if(std::abs(remainingZoomDiff) <= 0.5 * zoomSpeed_ * zoomSpeed_ / ZOOM_ACCELERATION)
    {
        // deceleration towards zero zoom speed
        if(zoomSpeed_ > 0)
            zoomSpeed_ -= ZOOM_ACCELERATION;
        else
            zoomSpeed_ += ZOOM_ACCELERATION;
    } else
    {
        // acceleration to unlimited speed
        if(remainingZoomDiff > 0)
            zoomSpeed_ += ZOOM_ACCELERATION;
        else
            zoomSpeed_ -= ZOOM_ACCELERATION;
    }

    if(std::abs(remainingZoomDiff) < std::abs(zoomSpeed_))
    {
        // last step
        zoomFactor_ = targetZoomFactor_;
        zoomSpeed_ = 0;
    }

    zoomFactor_ = zoomFactor_ + zoomSpeed_;
    CalcFxLx();
}

void GameWorldView::SetZoomFactor(float zoomFactor, bool smoothTransition /* = true*/)
{
    if(zoomFactor < ZOOM_FACTORS.front())
        targetZoomFactor_ = ZOOM_FACTORS.front();
    else if(zoomFactor > ZOOM_FACTORS.back())
        targetZoomFactor_ = ZOOM_FACTORS.back();
    else
        targetZoomFactor_ = zoomFactor;
    if(!smoothTransition)
        zoomFactor_ = targetZoomFactor_;
}

float GameWorldView::GetCurrentTargetZoomFactor() const
{
    return targetZoomFactor_;
}

struct ObjectBetweenLines
{
    noBase* obj;
    DrawPoint pos; // Zeichenposition

    ObjectBetweenLines(noBase* obj, const DrawPoint& pos) : obj(obj), pos(pos) {}
};

void GameWorldView::Draw(const RoadBuildState& rb, const MapPoint selected, bool drawMouse, unsigned* water)
{
    SetNextZoomFactor();

    int shortestDistToMouse = 100000;
    Position mousePos(VIDEODRIVER.GetMouseX(), VIDEODRIVER.GetMouseY());
    mousePos -= Position(origin_);

    glScissor(origin_.x, VIDEODRIVER.GetScreenSize().y - origin_.y - size_.y, size_.x, size_.y);
    if(zoomFactor_ != 1.f) //-V550
    {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glScalef(zoomFactor_, zoomFactor_, 1);
        // Offset to center view
        Point<float> diff(size_.x - size_.x / zoomFactor_, size_.y - size_.y / zoomFactor_);
        diff = diff / 2.f;
        glTranslatef(-diff.x, -diff.y, 0.f);
        // Also adjust mouse
        mousePos = Position(Point<float>(mousePos) / zoomFactor_ + diff);
        glMatrixMode(GL_MODELVIEW);
    }

    glTranslatef(static_cast<GLfloat>(origin_.x) / zoomFactor_, static_cast<GLfloat>(origin_.y) / zoomFactor_, 0.0f);

    glTranslatef(static_cast<GLfloat>(-offset.x), static_cast<GLfloat>(-offset.y), 0.0f);
    const TerrainRenderer& terrainRenderer = gwv.GetTerrainRenderer();
    terrainRenderer.Draw(GetFirstPt(), GetLastPt(), gwv, water);
    glTranslatef(static_cast<GLfloat>(offset.x), static_cast<GLfloat>(offset.y), 0.0f);

    for(int y = firstPt.y; y <= lastPt.y; ++y)
    {
        // Figuren speichern, die in dieser Zeile gemalt werden müssen
        // und sich zwischen zwei Zeilen befinden, da sie dazwischen laufen
        std::vector<ObjectBetweenLines> between_lines;

        for(int x = firstPt.x; x <= lastPt.x; ++x)
        {
            Position curOffset;
            const MapPoint curPt = terrainRenderer.ConvertCoords(Position(x, y), &curOffset);
            DrawPoint curPos = GetWorld().GetNodePos(curPt) - offset + curOffset;

            Position mouseDist = mousePos - curPos;
            mouseDist *= mouseDist;
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
                DrawMovingFiguresFromBelow(terrainRenderer, Position(x, y), between_lines);
                DrawFigures(curPt, curPos, between_lines);

                // Construction aid mode
                if(show_bq)
                    DrawConstructionAid(curPt, curPos);
            } else if(visibility == VIS_FOW)
            {
                const FOWObject* fowobj = gwv.GetYoungestFOWObject(MapPoint(curPt));
                if(fowobj)
                    fowobj->Draw(curPos);
            }

            for(IDrawNodeCallback* callback : drawNodeCallbacks)
                callback->onDraw(curPt, curPos);
        }

        // Figuren zwischen den Zeilen zeichnen
        for(unsigned i = 0; i < between_lines.size(); ++i)
            between_lines[i].obj->Draw(between_lines[i].pos);
    }

    if(show_names || show_productivity)
        DrawNameProductivityOverlay(terrainRenderer);

    DrawGUI(rb, terrainRenderer, selected, drawMouse);

    // Umherfliegende Katapultsteine zeichnen
    for(std::list<CatapultStone*>::const_iterator it = GetWorld().catapult_stones.begin(); it != GetWorld().catapult_stones.end(); ++it)
    {
        if(gwv.GetVisibility((*it)->dest_building) == VIS_VISIBLE || gwv.GetVisibility((*it)->dest_map) == VIS_VISIBLE)
            (*it)->Draw(offset);
    }

    if(zoomFactor_ != 1.f) //-V550
    {
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    }
    glTranslatef(-static_cast<GLfloat>(origin_.x) / zoomFactor_, -static_cast<GLfloat>(origin_.y) / zoomFactor_, 0.0f);

    glScissor(0, 0, VIDEODRIVER.GetScreenSize().x, VIDEODRIVER.GetScreenSize().y);
}

void GameWorldView::DrawGUI(const RoadBuildState& rb, const TerrainRenderer& terrainRenderer, const MapPoint& selectedPt, bool drawMouse)
{
    // Falls im Straßenbaumodus: Punkte um den aktuellen Straßenbaupunkt herum ermitteln
    std::array<MapPoint, 6> road_points;

    unsigned maxWaterWayLen = 0;
    if(rb.mode != RM_DISABLED)
    {
        for(unsigned i = 0; i < Direction::COUNT; ++i)
            road_points[i] = GetWorld().GetNeighbour(rb.point, Direction::fromInt(i));

        const unsigned index = GetWorld().GetGGS().getSelection(AddonId::MAX_WATERWAY_LENGTH);
        RTTR_Assert(index < waterwayLengths.size());
        maxWaterWayLen = waterwayLengths[index];
    }

    for(int x = firstPt.x; x <= lastPt.x; ++x)
    {
        for(int y = firstPt.y; y <= lastPt.y; ++y)
        {
            // Coordinates transform
            Position curOffset;
            MapPoint curPt = terrainRenderer.ConvertCoords(Position(x, y), &curOffset);
            Position curPos = GetWorld().GetNodePos(curPt) - offset + curOffset;

            /// Current point indicated by Mouse
            if(drawMouse && selPt == curPt)
            {
                // Mauszeiger am boden
                unsigned mid = 22;
                if(rb.mode == RM_DISABLED)
                {
                    switch(gwv.GetBQ(curPt))
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
                LOADER.GetMapImageN(mid)->DrawFull(curPos);
            }

            // Currently selected point
            if(selectedPt == curPt)
                LOADER.GetMapImageN(20)->DrawFull(curPos);

            // not building roads, no further action needed
            if(rb.mode == RM_DISABLED)
                continue;

            // we dont own curPt, no need for any rendering...
            if(!gwv.IsPlayerTerritory(curPt))
                continue;

            // we are in road build mode
            // highlight current route pt
            if(rb.point == curPt)
            {
                LOADER.GetMapImageN(21)->DrawFull(curPos);
                continue;
            }

            // ensure that curPt is a neighbour of rb.point
            if(!helpers::contains(road_points, curPt))
            {
                continue;
            }

            // test on maximal water way length
            if(rb.mode == RM_BOAT && maxWaterWayLen != 0 && rb.route.size() >= maxWaterWayLen)
                continue;

            // render special icon for route revert
            if(!rb.route.empty() && road_points[(rb.route.back() + 3u).toUInt()] == curPt)
            {
                LOADER.GetMapImageN(67)->DrawFull(curPos);
                continue;
            }

            // is a flag but not the route start flag, as it would revert the route.
            const bool targetFlag = GetWorld().GetNO(curPt)->GetType() == NOP_FLAG && curPt != rb.start;
            int altitude = GetWorld().GetNode(rb.point).altitude;
            if(targetFlag || gwv.IsRoadAvailable(rb.mode == RM_BOAT, curPt))
            {
                unsigned id;
                switch(int(GetWorld().GetNode(curPt).altitude) - altitude)
                {
                    case 1: id = 61; break;
                    case 2:
                    case 3: id = 62; break;
                    case 4:
                    case 5: id = 63; break;
                    case -1: id = 64; break;
                    case -2:
                    case -3: id = 65; break;
                    case -4:
                    case -5: id = 66; break;
                    default: id = 60; break;
                }
                if(!targetFlag)
                    LOADER.GetMapImageN(id)->DrawFull(curPos);
                else
                {
                    DrawPoint lastPos = GetWorld().GetNodePos(rb.point) - offset + curOffset;
                    DrawPoint halfWayPos = (curPos + lastPos) / 2;
                    LOADER.GetMapImageN(id)->DrawFull(halfWayPos);
                    LOADER.GetMapImageN(20)->DrawFull(curPos);
                }
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
            Position curOffset;
            MapPoint pt = terrainRenderer.ConvertCoords(Position(x, y), &curOffset);

            const noBaseBuilding* no = GetWorld().GetSpecObj<noBaseBuilding>(pt);
            if(!no)
                continue;

            // Is object not belonging to local player?
            if(no->GetPlayer() != gwv.GetPlayerId())
                continue;

            Position curPos = GetWorld().GetNodePos(pt) - offset + curOffset;
            curPos.y -= 22;

            // Draw object name
            if(show_names)
            {
                unsigned color = (no->GetGOT() == GOT_BUILDINGSITE) ? COLOR_GREY : COLOR_YELLOW;
                SmallFont->Draw(curPos, _(BUILDING_NAMES[no->GetBuildingType()]), FontStyle::CENTER | FontStyle::VCENTER, color);
                curPos.y += SmallFont->getHeight();
            }

            // Draw productivity/soldiers
            if(show_productivity)
                DrawProductivity(*no, curPos);
        }
    }
}

void GameWorldView::DrawProductivity(const noBaseBuilding& no, const DrawPoint& curPos)
{
    const GO_Type got = no.GetGOT();
    if(got == GOT_BUILDINGSITE)
    {
        unsigned color = COLOR_GREY;

        unsigned short p = static_cast<const noBuildingSite&>(no).GetBuildProgress();
        SmallFont->Draw(curPos, (boost::format("(%1% %%)") % p).str(), FontStyle::CENTER | FontStyle::VCENTER, color);
    } else if(got == GOT_NOB_USUAL || got == GOT_NOB_SHIPYARD)
    {
        const nobUsual& n = static_cast<const nobUsual&>(no);
        std::string text;
        unsigned color = COLOR_0_PERCENT;

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
                color = COLOR_60_PERCENT;
            else if(p >= 30)
                color = COLOR_30_PERCENT;
            else if(p >= 20)
                color = COLOR_20_PERCENT;
        }
        SmallFont->Draw(curPos, text, FontStyle::CENTER | FontStyle::VCENTER, color);
    } else if(got == GOT_NOB_MILITARY)
    {
        // Display amount of soldiers
        unsigned soldiers_count = static_cast<const nobMilitary&>(no).GetNumTroops();
        std::string sSoldiers;
        if(soldiers_count == 1)
            sSoldiers = _("(1 soldier)");
        else
            sSoldiers = boost::str(boost::format(_("(%d soldiers)")) % soldiers_count);

        SmallFont->Draw(curPos, sSoldiers, FontStyle::CENTER | FontStyle::VCENTER, (soldiers_count > 0) ? COLOR_YELLOW : COLOR_RED);
    }
}

void GameWorldView::DrawFigures(const MapPoint& pt, const DrawPoint& curPos, std::vector<ObjectBetweenLines>& between_lines)
{
    const std::list<noBase*>& figures = GetWorld().GetFigures(pt);
    for(noBase* figure : figures)
    {
        if(figure->IsMoving())
        {
            // Drawn from above
            Direction curMoveDir = static_cast<noMovable*>(figure)->GetCurMoveDir();
            if(curMoveDir == Direction::NORTHEAST || curMoveDir == Direction::NORTHWEST)
                continue;
            // Draw later
            between_lines.push_back(ObjectBetweenLines(figure, curPos));
        } else if(figure->GetGOT() == GOT_SHIP)
            between_lines.push_back(ObjectBetweenLines(figure, curPos)); // TODO: Why special handling for ships?
        else
            // Ansonsten jetzt schon zeichnen
            figure->Draw(curPos);
    }
}

void GameWorldView::DrawMovingFiguresFromBelow(const TerrainRenderer& terrainRenderer, const DrawPoint& curPos,
                                               std::vector<ObjectBetweenLines>& between_lines)
{
    // First draw figures moving towards this point from below
    static const std::array<Direction, 2> aboveDirs = {{Direction::NORTHEAST, Direction::NORTHWEST}};
    for(Direction dir : aboveDirs)
    {
        // Get figures opposite the current dir and check if they are moving in this dir
        // Coordinates transform
        Position curOffset;
        MapPoint curPt = terrainRenderer.ConvertCoords(GetNeighbour(curPos, dir + 3u), &curOffset);
        Position figPos = GetWorld().GetNodePos(curPt) - offset + curOffset;

        const std::list<noBase*>& figures = GetWorld().GetFigures(curPt);
        for(noBase* figure : figures)
        {
            if(figure->IsMoving() && static_cast<noMovable*>(figure)->GetCurMoveDir() == dir)
                between_lines.push_back(ObjectBetweenLines(figure, figPos));
        }
    }
}

void GameWorldView::DrawConstructionAid(const MapPoint& pt, const DrawPoint& curPos)
{
    BuildingQuality bq = gwv.GetBQ(pt);
    if(bq != BQ_NOTHING)
    {
        glArchivItem_Bitmap* bm = LOADER.GetMapImageN(49 + bq);
        // Draw building quality icon
        bm->DrawFull(curPos);
        // Show ability to construct military buildings
        if(GetWorld().GetGGS().isEnabled(AddonId::MILITARY_AID))
        {
            if(!GetWorld().IsMilitaryBuildingNearNode(pt, gwv.GetPlayerId())
               && (bq == BQ_HUT || bq == BQ_HOUSE || bq == BQ_CASTLE || bq == BQ_HARBOR))
                LOADER.GetImageN("map_new", 20000)->DrawFull(curPos - DrawPoint(-1, bm->getHeight() + 5));
        }
    }
}

void GameWorldView::DrawObject(const MapPoint& pt, const DrawPoint& curPos)
{
    noBase* obj = GetWorld().GetNode(pt).obj;
    if(!obj)
        return;

    obj->Draw(curPos);

    return;
    // TODO: military aid - display icon overlay of attack possibility

    if(gwv.GetNumSoldiersForAttack(pt) > 0) // soldiers available for attack?
        LOADER.GetImageN("map_new", 20000)->DrawFull(curPos + DrawPoint(1, -5));
}

void GameWorldView::DrawBoundaryStone(const MapPoint& pt, const DrawPoint pos, Visibility vis)
{
    if(vis == VIS_INVISIBLE)
        return;

    bool isFoW = vis == VIS_FOW;

    const BoundaryStones& boundary_stones = isFoW ? gwv.GetYoungestFOWNode(pt).boundary_stones : GetWorld().GetNode(pt).boundary_stones;
    unsigned char owner = boundary_stones[0];

    if(!owner)
        return;

    unsigned nation = GetWorld().GetPlayer(owner - 1).nation;
    unsigned player_color = GetWorld().GetPlayer(owner - 1).color;
    if(isFoW)
        player_color = CalcPlayerFOWDrawColor(player_color);

    LOADER.boundary_stone_cache[nation].draw(pos, isFoW ? FOW_DRAW_COLOR : COLOR_WHITE, player_color);

    for(unsigned i = 0; i < 3; ++i)
    {
        if(boundary_stones[i + 1])
        {
            DrawPoint tmp =
              pos
              - DrawPoint((gwv.GetTerrainRenderer().GetVertexPos(pt) - gwv.GetTerrainRenderer().GetNeighbourVertexPos(pt, 3 + i)) / 2.0f);

            LOADER.boundary_stone_cache[nation].draw(tmp, isFoW ? FOW_DRAW_COLOR : COLOR_WHITE, player_color);
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

/**
 *  verschiebt das Bild zu einer bestimmten Stelle.
 */
void GameWorldView::MoveTo(int x, int y, bool absolute)
{
    MoveTo(DrawPoint(x, y), absolute);
}

void GameWorldView::MoveTo(const DrawPoint& newPos, bool absolute)
{
    if(absolute)
        offset = newPos;
    else
        offset += newPos;

    DrawPoint size(GetWorld().GetWidth() * TR_W, GetWorld().GetHeight() * TR_H);
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
    Position nodePos = GetWorld().GetNodePos(pt);

    MoveTo(nodePos - GetSize() / 2u, true);
}

/// Springt zur letzten Position, bevor man "weggesprungen" ist
void GameWorldView::MoveToLastPosition()
{
    Position newLastOffset = offset;

    MoveTo(lastOffset.x, lastOffset.y, true);

    lastOffset = newLastOffset;
}

void GameWorldView::AddDrawNodeCallback(IDrawNodeCallback* newCallback)
{
    RTTR_Assert(newCallback);
    drawNodeCallbacks.push_back(newCallback);
}

void GameWorldView::RemoveDrawNodeCallback(IDrawNodeCallback* callbackToRemove)
{
    std::vector<IDrawNodeCallback*>::iterator itPos = helpers::find(drawNodeCallbacks, callbackToRemove);
    RTTR_Assert(itPos != drawNodeCallbacks.end());
    drawNodeCallbacks.erase(itPos);
}

void GameWorldView::CalcFxLx()
{
    // Calc first and last point in map units (with 1 extra for incomplete triangles)
    firstPt.x = offset.x / TR_W - 1;
    firstPt.y = offset.y / TR_H - 1;
    lastPt.x = (offset.x + size_.x) / TR_W + 1;
    lastPt.y = (offset.y + size_.y + 60 * HEIGHT_FACTOR) / TR_H + 1; // max altitude = 60

    if(zoomFactor_ != 1.f) //-V550
    {
        // Calc pixels we can remove from sides, as they are not drawn due to zoom
        Point<float> diff(size_.x - size_.x / zoomFactor_, size_.y - size_.y / zoomFactor_);
        // Stay centered by removing half the pixels from opposite sites
        diff = diff / 2.f;
        // Convert to map points
        diff.x /= TR_W;
        diff.y /= TR_H;
        // Don't remove to much
        diff.x = std::floor(diff.x);
        diff.y = std::floor(diff.y);
        firstPt = Position(Point<float>(firstPt) + diff);
        lastPt = Position(Point<float>(lastPt) - diff);
    }
}

void GameWorldView::Resize(const Extent& newSize)
{
    size_ = newSize;
    CalcFxLx();
}
