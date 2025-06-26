// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "world/GameWorldView.h"
#include "CatapultStone.h"
#include "FOWObjects.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "MapGeometry.h"
#include "Settings.h"
#include "addons/AddonMaxWaterwayLength.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobUsual.h"
#include "drivers/VideoDriverWrapper.h"
#include "helpers/EnumArray.h"
#include "helpers/containerUtils.h"
#include "helpers/toString.h"
#include "ogl/FontStyle.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glFont.h"
#include "ogl/glSmartBitmap.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldViewer.h"
#include "gameTypes/RoadBuildState.h"
#include "gameData/BuildingConsts.h"
#include "gameData/GuiConsts.h"
#include "gameData/MapConsts.h"
#include "s25util/error.h"
#include "s25util/warningSuppression.h"
#ifdef __EMSCRIPTEN__
#include "SDL/SDL.h"
#include "SDL/SDL_opengl.h"
#else
#include <glad/glad.h>
#endif
#include <boost/format.hpp>
#include <cmath>

GameWorldView::GameWorldView(const GameWorldViewer& gwv, const Position& pos, const Extent& size)
    : selPt(0, 0), show_bq(SETTINGS.ingame.showBQ), show_names(SETTINGS.ingame.showNames),
      show_productivity(SETTINGS.ingame.showProductivity), offset(0, 0), lastOffset(0, 0), gwv(gwv), origin_(pos),
      size_(size), zoomFactor_(1.f), targetZoomFactor_(1.f), zoomSpeed_(0.f)
{
    updateEffectiveZoomFactor();
    MoveBy({0, 0});
}

const GameWorldBase& GameWorldView::GetWorld() const
{
    return gwv.GetWorld();
}

SoundManager& GameWorldView::GetSoundMgr()
{
    return const_cast<GameWorldViewer&>(gwv).GetSoundMgr();
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
    updateEffectiveZoomFactor();
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
    {
        zoomFactor_ = targetZoomFactor_;
        updateEffectiveZoomFactor();
        CalcFxLx();
    }
}

float GameWorldView::GetCurrentTargetZoomFactor() const
{
    return targetZoomFactor_;
}

struct ObjDraw
{
    noBase* obj;
    DrawPoint pos;
    ObjDraw(noBase* obj, const DrawPoint& pos) : obj(obj), pos(pos) {}
};

struct ObjFOW
{
    const FOWObject* obj;
    DrawPoint pos;
    ObjFOW(const FOWObject* obj, const DrawPoint& pos) : obj(obj), pos(pos) {}
};

struct ObjConstructionAid
{
    ITexture* obj;
    DrawPoint pos;
    ObjConstructionAid(ITexture* obj, const DrawPoint& pos) : obj(obj), pos(pos) {}
};

struct ObjBS
{
    glSmartBitmap& obj;
    DrawPoint pos;
    unsigned color;
    unsigned player_color;
    ObjBS(glSmartBitmap& obj, const DrawPoint& pos, unsigned color, unsigned player_color) : obj(obj), pos(pos), color(color), player_color(player_color) {}
};

void GameWorldView::Draw(const RoadBuildState& rb, const MapPoint selected, bool drawMouse, unsigned* water)
{
    SetNextZoomFactor();
    std::vector<ObjDraw> renderQueueObjects;
    std::vector<ObjFOW> renderQueueFOW;
    std::vector<ObjConstructionAid> renderQueueBQ;
    std::vector<ObjBS> renderQueueBS;

    const auto windowSize = VIDEODRIVER.GetWindowSize();
    const auto& guiScale = VIDEODRIVER.getGuiScale();
    const auto screenOrigin = guiScale.viewToScreen(origin_);
    const auto screenSize = guiScale.viewToScreen(size_);
    glScissor(screenOrigin.x, windowSize.height - (screenOrigin.y + screenSize.y), screenSize.x, screenSize.y);

    int shortestDistToMouse = 100000;
    Position mousePos = VIDEODRIVER.GetMousePos();
    mousePos -= Position(origin_);
    if(effectiveZoomFactor_ != 1.f) //-V550
    {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glScalef(effectiveZoomFactor_, effectiveZoomFactor_, 1);
        // Offset to center view
        PointF diff(size_.x - size_.x / effectiveZoomFactor_, size_.y - size_.y / effectiveZoomFactor_);
        diff = diff / 2.f;
        glTranslatef(-diff.x, -diff.y, 0.f);
        // Also adjust mouse
        mousePos = Position(PointF(mousePos) / effectiveZoomFactor_ + diff);
        glMatrixMode(GL_MODELVIEW);
    }

    glTranslatef(static_cast<GLfloat>(origin_.x) / effectiveZoomFactor_,
                 static_cast<GLfloat>(origin_.y) / effectiveZoomFactor_, 0.0f);

    glTranslatef(static_cast<GLfloat>(-offset.x), static_cast<GLfloat>(-offset.y), 0.0f);
    const TerrainRenderer& terrainRenderer = gwv.GetTerrainRenderer();
    terrainRenderer.Draw(GetFirstPt(), GetLastPt(), gwv, water);
    glTranslatef(static_cast<GLfloat>(offset.x), static_cast<GLfloat>(offset.y), 0.0f);

    for(int y = firstPt.y; y <= lastPt.y; ++y)
    {

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

            DrawBoundaryStone(curPt, curPos, visibility, renderQueueBS);

            if(visibility == Visibility::Visible)
            {
                renderQueueObjects.push_back(ObjDraw(GetWorld().GetNode(curPt).obj, curPos));

                DrawMovingFiguresFromBelow(Direction::NorthEast, terrainRenderer, Position(x, y), renderQueueObjects);
                DrawMovingFiguresFromBelow(Direction::NorthWest, terrainRenderer, Position(x, y), renderQueueObjects);

                //Draw figures
                for(noBase& figure : GetWorld().GetFigures(curPt))
                {
                    if(figure.IsMoving())
                    {
                        // Drawn from above
                        Direction curMoveDir = static_cast<noMovable&>(figure).GetCurMoveDir();
                        if(Direction::NorthEast == curMoveDir || curMoveDir == Direction::NorthWest)
                            continue;
                        // Draw later
                        renderQueueObjects.push_back(ObjDraw(&figure, curPos));
                    }
                    else
                    {
                        // Ansonsten jetzt schon zeichnen (not now, but in different layer)
                        renderQueueObjects.push_back(ObjDraw(&figure, curPos));
                    }
                }

                // Construction aid mode
                if(show_bq)
                {
                    DrawConstructionAid(curPt, curPos, renderQueueBQ);
                }
            } else if(visibility == Visibility::FogOfWar)
            {
                renderQueueFOW.push_back(ObjFOW(gwv.GetYoungestFOWObject(MapPoint(curPt)), curPos));
            }

            for(IDrawNodeCallback* callback : drawNodeCallbacks)
            {
                callback->onDraw(curPt, curPos);
            }
        }
    }
    // just group draw calls this gives +5fps in emscripten build
    for(auto& bs : renderQueueBS)
    {
        bs.obj.draw(bs.pos, bs.color, bs.player_color);
    }

    for(auto& renderQueueObject : renderQueueObjects)
    {
        if (!renderQueueObject.obj) continue;
        renderQueueObject.obj->Draw(renderQueueObject.pos);
    }

    for(auto& fow : renderQueueFOW)
    {
        if (!fow.obj) continue;
        fow.obj->Draw(fow.pos);
    }

    for(auto& bq : renderQueueBQ)
    {
        if (!bq.obj) continue;
        bq.obj->DrawFull(bq.pos);
    }

    if(show_names || show_productivity)
        DrawNameProductivityOverlay(terrainRenderer);

    DrawGUI(rb, terrainRenderer, selected, drawMouse);

    // Umherfliegende Katapultsteine zeichnen
    for(auto* catapult_stone : GetWorld().catapult_stones)
    {
        if(gwv.GetVisibility(catapult_stone->dest_building) == Visibility::Visible
           || gwv.GetVisibility(catapult_stone->dest_map) == Visibility::Visible)
            catapult_stone->Draw(offset);
    }

    if(effectiveZoomFactor_ != 1.f) //-V550
    {
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    }
    glTranslatef(-static_cast<GLfloat>(origin_.x) / effectiveZoomFactor_,
                 -static_cast<GLfloat>(origin_.y) / effectiveZoomFactor_, 0.0f);

    glScissor(0, 0, windowSize.width, windowSize.height);
}

void GameWorldView::DrawGUI(const RoadBuildState& rb, const TerrainRenderer& terrainRenderer,
                            const MapPoint& selectedPt, bool drawMouse)
{
    // Falls im Straßenbaumodus: Punkte um den aktuellen Straßenbaupunkt herum ermitteln
    helpers::EnumArray<MapPoint, Direction> road_points;

    unsigned maxWaterWayLen = 0;
    if(rb.mode != RoadBuildMode::Disabled)
    {
        for(const auto dir : helpers::EnumRange<Direction>{})
            road_points[dir] = GetWorld().GetNeighbour(rb.point, dir);

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
                if(rb.mode == RoadBuildMode::Disabled)
                {
                    switch(gwv.GetBQ(curPt))
                    {
                        case BuildingQuality::Flag: mid = 40; break;
                        case BuildingQuality::Mine: mid = 41; break;
                        case BuildingQuality::Hut: mid = 42; break;
                        case BuildingQuality::House: mid = 43; break;
                        case BuildingQuality::Castle: mid = 44; break;
                        case BuildingQuality::Harbor: mid = 45; break;
                        default: break;
                    }
                }
                LOADER.GetMapTexture(mid)->DrawFull(curPos);
            }

            // Currently selected point
            if(selectedPt == curPt)
                LOADER.GetMapTexture(20)->DrawFull(curPos);

            // not building roads, no further action needed
            if(rb.mode == RoadBuildMode::Disabled)
                continue;

            // we dont own curPt, no need for any rendering...
            if(!gwv.IsPlayerTerritory(curPt))
                continue;

            // we are in road build mode
            // highlight current route pt
            if(rb.point == curPt)
            {
                LOADER.GetMapTexture(21)->DrawFull(curPos);
                continue;
            }

            // ensure that curPt is a neighbour of rb.point
            if(!helpers::contains(road_points, curPt))
            {
                continue;
            }

            // test on maximal water way length
            if(rb.mode == RoadBuildMode::Boat && maxWaterWayLen != 0 && rb.route.size() >= maxWaterWayLen)
                continue;

            // render special icon for route revert
            if(!rb.route.empty() && road_points[rb.route.back() + 3u] == curPt)
            {
                LOADER.GetMapTexture(67)->DrawFull(curPos);
                continue;
            }

            // is a flag but not the route start flag, as it would revert the route.
            const bool targetFlag = GetWorld().GetNO(curPt)->GetType() == NodalObjectType::Flag && curPt != rb.start;
            int altitude = GetWorld().GetNode(rb.point).altitude;
            if(targetFlag || gwv.IsRoadAvailable(rb.mode == RoadBuildMode::Boat, curPt))
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
                    LOADER.GetMapTexture(id)->DrawFull(curPos);
                else
                {
                    DrawPoint lastPos = GetWorld().GetNodePos(rb.point) - offset + curOffset;
                    DrawPoint halfWayPos = (curPos + lastPos) / 2;
                    LOADER.GetMapTexture(id)->DrawFull(halfWayPos);
                    LOADER.GetMapTexture(20)->DrawFull(curPos);
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

            const auto* no = GetWorld().GetSpecObj<noBaseBuilding>(pt);
            if(!no)
                continue;

            Position curPos = GetWorld().GetNodePos(pt) - offset + curOffset;
            curPos.y -= 22;

            // Is object not belonging to local player?
            if(no->GetPlayer() != gwv.GetPlayerId())
            {
                if(GetWorld().GetGGS().getSelection(AddonId::MILITARY_AID) == 2 && gwv.GetNumSoldiersForAttack(pt) > 0)
                {
                    auto* attackAidImage = LOADER.GetImageN("map_new", 20000);
                    attackAidImage->DrawFull(curPos - DrawPoint(0, attackAidImage->getHeight()));
                }
                continue;
            }

            // Draw object name
            if(show_names)
            {
                unsigned color = (no->GetGOT() == GO_Type::Buildingsite) ? COLOR_GREY : COLOR_YELLOW;
                SmallFont->Draw(curPos, _(BUILDING_NAMES[no->GetBuildingType()]),
                                FontStyle::CENTER | FontStyle::VCENTER, color);
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
    if(got == GO_Type::Buildingsite)
    {
        unsigned color = COLOR_GREY;

        unsigned short p = static_cast<const noBuildingSite&>(no).GetBuildProgress();
        SmallFont->Draw(curPos, (boost::format("(%1% %%)") % p).str(), FontStyle::CENTER | FontStyle::VCENTER, color);
    } else if(got == GO_Type::NobUsual || got == GO_Type::NobShipyard || got == GO_Type::NobTemple)
    {
        const auto& n = static_cast<const nobUsual&>(no);
        std::string text;
        unsigned color = COLOR_0_PERCENT;

        if(!n.HasWorker())
            text = _("(House unoccupied)");
        else if(n.IsProductionDisabledVirtual())
            text = _("(stopped)");
        else
        {
            // Catapult and Lookout tower doesn't have productivity!
            if(n.GetBuildingType() == BuildingType::Catapult || n.GetBuildingType() == BuildingType::LookoutTower)
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
    } else if(got == GO_Type::NobMilitary)
    {
        // Display amount of soldiers
        unsigned soldiers_count = static_cast<const nobMilitary&>(no).GetNumTroops();
        std::string sSoldiers;
        if(soldiers_count == 1)
            sSoldiers = _("(1 soldier)");
        else
            sSoldiers = boost::str(boost::format(_("(%d soldiers)")) % soldiers_count);

        SmallFont->Draw(curPos, sSoldiers, FontStyle::CENTER | FontStyle::VCENTER,
                        (soldiers_count > 0) ? COLOR_YELLOW : COLOR_RED);
    }
}

void GameWorldView::DrawMovingFiguresFromBelow(Direction dir, const TerrainRenderer& terrainRenderer, const DrawPoint& curPos,
                                               std::vector<ObjDraw>& rq)
{
    // Get figures opposite the current dir and check if they are moving in this dir
    // Coordinates transform
    Position curOffset;
    MapPoint curPt = terrainRenderer.ConvertCoords(GetNeighbour(curPos, dir + 3u), &curOffset);
    Position figPos = GetWorld().GetNodePos(curPt) - offset + curOffset;

    for(noBase& figure : GetWorld().GetFigures(curPt))
    {
        if(figure.IsMoving() && static_cast<noMovable&>(figure).GetCurMoveDir() == dir)
            rq.push_back(ObjDraw(&figure, figPos));
    }
}

constexpr auto getBqImgs()
{
    helpers::EnumArray<unsigned, BuildingQuality> imgs{};
    imgs[BuildingQuality::Flag] = 50;
    imgs[BuildingQuality::Hut] = 51;
    imgs[BuildingQuality::House] = 52;
    imgs[BuildingQuality::Castle] = 53;
    imgs[BuildingQuality::Mine] = 54;
    imgs[BuildingQuality::Harbor] = 55;
    return imgs;
}

void GameWorldView::DrawConstructionAid(const MapPoint& pt, const DrawPoint& curPos, std::vector<ObjConstructionAid> &rq)
{
    BuildingQuality bq = gwv.GetBQ(pt);
    if(bq != BuildingQuality::Nothing)
    {
        constexpr auto bqImgs = getBqImgs();
        auto* bm = LOADER.GetMapTexture(bqImgs[bq]);
        // Draw building quality icon
        //bm->DrawFull(curPos);
        rq.push_back(ObjConstructionAid(bm, curPos));
        // Show ability to construct military buildings
        if(GetWorld().GetGGS().isEnabled(AddonId::MILITARY_AID))
        {
            if(!GetWorld().IsMilitaryBuildingNearNode(pt, gwv.GetPlayerId())
               && (bq == BuildingQuality::Hut
                   || bq == BuildingQuality::House
                   || bq == BuildingQuality::Castle
                   || bq == BuildingQuality::Harbor))
            {
                //LOADER.GetImageN("map_new", 20000)->DrawFull(curPos - DrawPoint(-1, bm->GetSize().y + 5));
                rq.push_back(ObjConstructionAid(LOADER.GetImageN("map_new", 20000), curPos - DrawPoint(-1, bm->GetSize().y + 5)));
            }
        }
    }
}

void GameWorldView::DrawBoundaryStone(const MapPoint& pt, const DrawPoint pos, Visibility vis, std::vector<ObjBS>& rq)
{
    if(vis == Visibility::Invisible) return;

    const bool isFoW = vis == Visibility::FogOfWar;

    const BoundaryStones& boundary_stones = isFoW
        ? gwv.GetYoungestFOWNode(pt).boundary_stones
        : GetWorld().GetNode(pt).boundary_stones;
    const unsigned char owner = boundary_stones[BorderStonePos::OnPoint];

    if(!owner) return;

    const Nation nation = GetWorld().GetPlayer(owner - 1).nation;
    unsigned player_color = GetWorld().GetPlayer(owner - 1).color;
    if(isFoW)
    {
        player_color = CalcPlayerFOWDrawColor(player_color);
    }

    const auto curVertexPos = gwv.GetTerrainRenderer().GetVertexPos(pt);
    for(const auto bPos : helpers::EnumRange<BorderStonePos>{})
    {
        DrawPoint curPos;
        if(bPos == BorderStonePos::OnPoint)
        {
            curPos = pos;
        }
        else if(boundary_stones[bPos])
        {
            curPos = pos - DrawPoint((curVertexPos - gwv.GetTerrainRenderer().GetNeighbourVertexPos(pt, toDirection(bPos))) / 2.0f);
        }
        else continue;
        rq.push_back(ObjBS(LOADER.boundary_stone_cache[nation], curPos, isFoW ? FOW_DRAW_COLOR : COLOR_WHITE, player_color));
        //LOADER.boundary_stone_cache[nation].draw(curPos, isFoW ? FOW_DRAW_COLOR : COLOR_WHITE, player_color);
    }
}

void GameWorldView::ToggleShowBQ()
{
    show_bq = !show_bq;
    SaveIngameSettingsValues();
    onHudSettingsChanged();
}

void GameWorldView::ToggleShowNames()
{
    show_names = !show_names;
    SaveIngameSettingsValues();
    onHudSettingsChanged();
}

void GameWorldView::ToggleShowProductivity()
{
    show_productivity = !show_productivity;
    SaveIngameSettingsValues();
    onHudSettingsChanged();
}

void GameWorldView::ToggleShowNamesAndProductivity()
{
    if(show_productivity && show_names)
        show_productivity = show_names = false;
    else
        show_productivity = show_names = true;
    SaveIngameSettingsValues();
    onHudSettingsChanged();
}

void GameWorldView::CopyHudSettingsTo(GameWorldView& other, bool copyBQ) const
{
    other.show_bq = (copyBQ ? show_bq : false);
    other.show_names = show_names;
    other.show_productivity = show_productivity;
}

void GameWorldView::MoveBy(const DrawPoint& numPixels)
{
    MoveTo(offset + numPixels);
}

void GameWorldView::MoveTo(const DrawPoint& newPos)
{
    offset = newPos;
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

    MoveTo(nodePos - GetSize() / 2u);
}

void GameWorldView::MoveToLastPosition()
{
    Position newLastOffset = offset;

    MoveTo(lastOffset);

    lastOffset = newLastOffset;
}

void GameWorldView::AddDrawNodeCallback(IDrawNodeCallback* newCallback)
{
    RTTR_Assert(newCallback);
    drawNodeCallbacks.push_back(newCallback);
}

void GameWorldView::RemoveDrawNodeCallback(IDrawNodeCallback* callbackToRemove)
{
    auto itPos = helpers::find(drawNodeCallbacks, callbackToRemove);
    RTTR_Assert(itPos != drawNodeCallbacks.end());
    drawNodeCallbacks.erase(itPos);
}

void GameWorldView::CalcFxLx()
{
    // Calc first and last point in map units (with 1 extra for incomplete triangles)
    firstPt.x = offset.x / TR_W - 1;
    firstPt.y = offset.y / TR_H - 1;
    lastPt.x = (offset.x + size_.x) / TR_W + 1;
    const auto maxAltitude = gwv.getMaxNodeAltitude();
    lastPt.y = (offset.y + size_.y + maxAltitude * HEIGHT_FACTOR) / TR_H + 1;

    if(effectiveZoomFactor_ != 1.f) //-V550
    {
        // Calc pixels we can remove from sides, as they are not drawn due to zoom
        PointF diff(size_.x - size_.x / effectiveZoomFactor_, size_.y - size_.y / effectiveZoomFactor_);
        // Stay centered by removing half the pixels from opposite sites
        diff = diff / 2.f;
        // Convert to map points
        diff.x /= TR_W;
        diff.y /= TR_H;
        // Don't remove to much
        diff.x = std::floor(diff.x);
        diff.y = std::floor(diff.y);
        firstPt = Position(PointF(firstPt) + diff);
        lastPt = Position(PointF(lastPt) - diff);
    }
}

void GameWorldView::Resize(const Extent& newSize)
{
    size_ = newSize;
    CalcFxLx();
}

void GameWorldView::SaveIngameSettingsValues() const
{
    auto& ingameSettings = SETTINGS.ingame;
    ingameSettings.showBQ = show_bq;
    ingameSettings.showNames = show_names;
    ingameSettings.showProductivity = show_productivity;
}

void GameWorldView::updateEffectiveZoomFactor()
{
    // partially "undo" GUI scale depending on DPI scale
    // => zoom levels should look the same on screens with different DPI regardless of GUI scale
    effectiveZoomFactor_ = VIDEODRIVER.getGuiScale().screenToView(zoomFactor_ * VIDEODRIVER.getDpiScale());
}
